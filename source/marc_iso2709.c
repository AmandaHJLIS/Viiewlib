#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define MARC_RECORD_TERMINATOR  0x1D
#define MARC_FIELD_TERMINATOR   0x1E
#define MARC_SUBFIELD_DELIMITER 0x1F

#define MARC_MAX_RECORD_LENGTH 99999
#define MARC_MAX_FIELD_LENGTH  9999
#define MARC_MAX_FIELD_POSITION 99999


/*
 * --------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------
 */

static int is_control_field(
    const MARC_Field *field
)
{
    if (field == NULL)
    {
        return 0;
    }

    return marc_field_is_control_field(
        field
    );
}


/*
 * Calculate the number of bytes occupied by
 * a field in the ISO 2709 variable-field area.
 */
static int field_length(
    const MARC_Field *field
)
{
    size_t length;
    size_t i;

    if (field == NULL)
    {
        return -1;
    }

    /*
     * Control field:
     *
     * value
     * field terminator
     */
    if (is_control_field(field))
    {
        const char *value;

        value = marc_field_get_control_value(
            field
        );

        if (value == NULL)
        {
            return -1;
        }

        length = strlen(value) + 1;

        if (length > MARC_MAX_FIELD_LENGTH)
        {
            return -1;
        }

        return (int)length;
    }

    /*
     * Data field:
     *
     * indicator 1
     * indicator 2
     *
     * for each subfield:
     *     delimiter
     *     code
     *     value
     *
     * field terminator
     */
    length = 2;

    for (i = 0;
         i < marc_field_get_subfield_count(field);
         i++)
    {
        MARC_Subfield *subfield;
        const char *value;

        subfield = marc_field_get_subfield(
            field,
            i
        );

        if (subfield == NULL)
        {
            return -1;
        }

        value = marc_subfield_get_value(
            subfield
        );

        if (value == NULL)
        {
            return -1;
        }

        /*
         * Delimiter + subfield code.
         */
        length += 2;

        length += strlen(value);

        if (length > MARC_MAX_FIELD_LENGTH)
        {
            return -1;
        }
    }

    /*
     * Field terminator.
     */
    length += 1;

    if (length > MARC_MAX_FIELD_LENGTH)
    {
        return -1;
    }

    return (int)length;
}


/*
 * Control fields must precede data fields.
 *
 * Within each group, fields are sorted by tag.
 */
static int compare_fields(
    const void *a,
    const void *b
)
{
    const MARC_Field *field_a;
    const MARC_Field *field_b;

    field_a =
        *(const MARC_Field * const *)a;

    field_b =
        *(const MARC_Field * const *)b;

    if (is_control_field(field_a) &&
        !is_control_field(field_b))
    {
        return -1;
    }

    if (!is_control_field(field_a) &&
        is_control_field(field_b))
    {
        return 1;
    }

    return strcmp(
        marc_field_get_tag(field_a),
        marc_field_get_tag(field_b)
    );
}


/*
 * Write a zero-padded decimal number using
 * exactly 'width' bytes.
 *
 * IMPORTANT:
 *
 * This function does NOT append a NUL terminator.
 *
 * ISO 2709 uses fixed-width numeric fields inside
 * the leader and directory, so writing a C-string
 * terminator here would corrupt the record.
 */
static int write_ascii_number(
    char *destination,
    size_t width,
    size_t value
)
{
    size_t limit;
    size_t i;

    if (destination == NULL ||
        width == 0)
    {
        return -1;
    }

    /*
     * Calculate 10^width.
     */
    limit = 1;

    for (i = 0; i < width; i++)
    {
        if (limit > ((size_t)-1) / 10)
        {
            return -1;
        }

        limit *= 10;
    }

    /*
     * Value must fit inside the requested
     * number of decimal digits.
     */
    if (value >= limit)
    {
        return -1;
    }

    /*
     * Fill from right to left.
     */
    for (i = width; i > 0; i--)
    {
        destination[i - 1] =
            (char)('0' + (value % 10));

        value /= 10;
    }

    /*
     * DO NOT write destination[width] = '\0'.
     *
     * The destination may be inside the
     * fixed 24-byte MARC leader.
     */

    return 0;
}


static int append_byte(
    unsigned char *buffer,
    size_t capacity,
    size_t *position,
    unsigned char value
)
{
    if (buffer == NULL ||
        position == NULL)
    {
        return -1;
    }

    if (*position >= capacity)
    {
        return -1;
    }

    buffer[*position] = value;

    (*position)++;

    return 0;
}


static int append_bytes(
    unsigned char *buffer,
    size_t capacity,
    size_t *position,
    const void *data,
    size_t length
)
{
    if (buffer == NULL ||
        position == NULL)
    {
        return -1;
    }

    if (length > 0 &&
        data == NULL)
    {
        return -1;
    }

    if (*position > capacity ||
        length > capacity - *position)
    {
        return -1;
    }

    if (length > 0)
    {
        memcpy(
            buffer + *position,
            data,
            length
        );
    }

    *position += length;

    return 0;
}


/*
 * Parse a fixed-width ASCII decimal number.
 */
static int parse_ascii_number(
    const unsigned char *data,
    size_t width,
    size_t *value
)
{
    size_t result;
    size_t i;

    if (data == NULL ||
        value == NULL ||
        width == 0)
    {
        return -1;
    }

    result = 0;

    for (i = 0; i < width; i++)
    {
        unsigned char character;

        character = data[i];

        if (character < '0' ||
            character > '9')
        {
            return -1;
        }

        /*
         * Prevent overflow.
         */
        if (result >
            (((size_t)-1) -
             (size_t)(character - '0')) / 10)
        {
            return -1;
        }

        result =
            result * 10 +
            (size_t)(character - '0');
    }

    *value = result;

    return 0;
}


/*
 * --------------------------------------------------------------------------
 * ISO 2709 writer
 * --------------------------------------------------------------------------
 */

int marc_record_write(
    const MARC_Record *record,
    FILE *stream
)
{
    size_t field_count;
    MARC_Field **fields;

    size_t directory_length;
    size_t data_length;
    size_t base_address;
    size_t record_length;

    size_t i;
    size_t position;

    unsigned char *output;

    char leader[25];


    /*
     * Validate arguments.
     */

    if (record == NULL)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: record is NULL\n"
        );

        return -1;
    }

    if (stream == NULL)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: stream is NULL\n"
        );

        return -1;
    }


    /*
     * Validate leader.
     */

    if (marc_record_get_leader(record) == NULL)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: leader is NULL\n"
        );

        return -1;
    }

    if (strlen(
        marc_record_get_leader(record)
    ) != 24)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: leader is not 24 bytes\n"
        );

        return -1;
    }


    /*
     * Get fields.
     */

    field_count =
        marc_record_get_field_count(record);

    if (field_count == 0)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: record contains no fields\n"
        );

        return -1;
    }


    /*
     * Copy field pointers so that sorting
     * does not modify the user's record.
     */

    fields = malloc(
        sizeof(MARC_Field *) *
        field_count
    );

    if (fields == NULL)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: failed allocating field list\n"
        );

        return -1;
    }


    for (i = 0; i < field_count; i++)
    {
        fields[i] =
            marc_record_get_field(
                record,
                i
            );

        if (fields[i] == NULL)
        {
            fprintf(
                stderr,
                "ISO2709 WRITE ERROR: field %zu is NULL\n",
                i
            );

            free(fields);

            return -1;
        }
    }


    qsort(
        fields,
        field_count,
        sizeof(MARC_Field *),
        compare_fields
    );


    /*
     * Directory:
     *
     * 12 bytes per field
     * + 1 directory terminator
     */

    directory_length =
        (field_count * 12) + 1;


    /*
     * Calculate variable-field data length.
     */

    data_length = 0;

    for (i = 0; i < field_count; i++)
    {
        int length;

        length =
            field_length(fields[i]);

        if (length < 0)
        {
            fprintf(
                stderr,
                "ISO2709 WRITE ERROR: failed calculating field %zu (%s) length\n",
                i,
                marc_field_get_tag(fields[i])
            );

            free(fields);

            return -1;
        }

        data_length +=
            (size_t)length;
    }


    /*
     * Base address:
     *
     * 24-byte leader
     * + directory
     */

    base_address =
        24 +
        directory_length;


    /*
     * Complete record:
     *
     * leader
     * + directory
     * + variable fields
     * + record terminator
     */

    record_length =
        base_address +
        data_length +
        1;


    fprintf(
        stderr,
        "ISO2709 WRITE DEBUG:\n"
        "  fields          = %zu\n"
        "  directory       = %zu\n"
        "  base address     = %zu\n"
        "  variable data    = %zu\n"
        "  record length    = %zu\n",
        field_count,
        directory_length,
        base_address,
        data_length,
        record_length
    );


    /*
     * MARC21 supports record lengths up to
     * five decimal digits.
     */

    if (record_length >
        MARC_MAX_RECORD_LENGTH)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: record is too large\n"
        );

        free(fields);

        return -1;
    }


    if (base_address >
        MARC_MAX_FIELD_POSITION)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: base address is too large\n"
        );

        free(fields);

        return -1;
    }


    /*
     * Allocate complete record.
     */

    output = malloc(
        record_length
    );

    if (output == NULL)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: failed allocating record buffer\n"
        );

        free(fields);

        return -1;
    }


    /*
     * Copy original leader.
     */

    memcpy(
        leader,
        marc_record_get_leader(record),
        24
    );

    leader[24] = '\0';


    /*
     * Update leader.
     *
     * 00-04 = record length
     * 10    = indicator count
     * 11    = subfield code length
     * 12-16 = base address
     * 20-23 = entry map
     */

    if (write_ascii_number(
        leader + 0,
        5,
        record_length
    ) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: failed encoding record length\n"
        );

        free(output);
        free(fields);

        return -1;
    }


    leader[10] = '2';
    leader[11] = '2';


    if (write_ascii_number(
        leader + 12,
        5,
        base_address
    ) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: failed encoding base address\n"
        );

        free(output);
        free(fields);

        return -1;
    }


    leader[20] = '4';
    leader[21] = '5';
    leader[22] = '0';
    leader[23] = '0';


    /*
     * Write leader.
     */

    position = 0;

    if (append_bytes(
        output,
        record_length,
        &position,
        leader,
        24
    ) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: failed writing leader\n"
        );

        free(output);
        free(fields);

        return -1;
    }


    /*
     * ----------------------------------------------------------------------
     * Directory
     * ----------------------------------------------------------------------
     */

    {
        size_t field_position;

        field_position = 0;

        for (i = 0; i < field_count; i++)
        {
            MARC_Field *field;
            const char *tag;
            int length;

            char entry[12];

            field = fields[i];

            tag =
                marc_field_get_tag(field);

            length =
                field_length(field);


            if (tag == NULL ||
                length < 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: invalid directory field\n"
                );

                free(output);
                free(fields);

                return -1;
            }


            /*
             * Directory entry:
             *
             * 3 bytes tag
             * 4 bytes field length
             * 5 bytes starting position
             */

            memcpy(
                entry,
                tag,
                3
            );


            if (write_ascii_number(
                entry + 3,
                4,
                (size_t)length
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed encoding field %s length\n",
                    tag
                );

                free(output);
                free(fields);

                return -1;
            }


            if (write_ascii_number(
                entry + 7,
                5,
                field_position
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed encoding field %s position\n",
                    tag
                );

                free(output);
                free(fields);

                return -1;
            }


            if (append_bytes(
                output,
                record_length,
                &position,
                entry,
                12
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed writing directory entry for %s\n",
                    tag
                );

                free(output);
                free(fields);

                return -1;
            }


            field_position +=
                (size_t)length;
        }


        /*
         * Directory terminator.
         */

        if (append_byte(
            output,
            record_length,
            &position,
            MARC_FIELD_TERMINATOR
        ) != 0)
        {
            fprintf(
                stderr,
                "ISO2709 WRITE ERROR: failed writing directory terminator\n"
            );

            free(output);
            free(fields);

            return -1;
        }
    }


    /*
     * ----------------------------------------------------------------------
     * Variable fields
     * ----------------------------------------------------------------------
     */

    for (i = 0; i < field_count; i++)
    {
        MARC_Field *field;

        field = fields[i];


        /*
         * Control field.
         */

        if (is_control_field(field))
        {
            const char *value;

            value =
                marc_field_get_control_value(
                    field
                );

            if (value == NULL)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: control field %s has no value\n",
                    marc_field_get_tag(field)
                );

                free(output);
                free(fields);

                return -1;
            }


            if (append_bytes(
                output,
                record_length,
                &position,
                value,
                strlen(value)
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed writing control field %s\n",
                    marc_field_get_tag(field)
                );

                free(output);
                free(fields);

                return -1;
            }


            if (append_byte(
                output,
                record_length,
                &position,
                MARC_FIELD_TERMINATOR
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed writing control field terminator\n"
                );

                free(output);
                free(fields);

                return -1;
            }
        }


        /*
         * Data field.
         */

        else
        {
            size_t subfield_count;
            size_t j;

            char indicator1;
            char indicator2;


            indicator1 =
                marc_field_get_indicator1(field);

            indicator2 =
                marc_field_get_indicator2(field);


            /*
             * Indicator 1.
             */

            if (append_byte(
                output,
                record_length,
                &position,
                (unsigned char)indicator1
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed writing indicator 1 for %s\n",
                    marc_field_get_tag(field)
                );

                free(output);
                free(fields);

                return -1;
            }


            /*
             * Indicator 2.
             */

            if (append_byte(
                output,
                record_length,
                &position,
                (unsigned char)indicator2
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed writing indicator 2 for %s\n",
                    marc_field_get_tag(field)
                );

                free(output);
                free(fields);

                return -1;
            }


            /*
             * Subfields.
             */

            subfield_count =
                marc_field_get_subfield_count(field);


            for (j = 0;
                 j < subfield_count;
                 j++)
            {
                MARC_Subfield *subfield;
                char code;
                const char *value;


                subfield =
                    marc_field_get_subfield(
                        field,
                        j
                    );

                if (subfield == NULL)
                {
                    fprintf(
                        stderr,
                        "ISO2709 WRITE ERROR: NULL subfield %zu in %s\n",
                        j,
                        marc_field_get_tag(field)
                    );

                    free(output);
                    free(fields);

                    return -1;
                }


                code =
                    marc_subfield_get_code(
                        subfield
                    );

                value =
                    marc_subfield_get_value(
                        subfield
                    );


                if (code == '\0' ||
                    value == NULL)
                {
                    fprintf(
                        stderr,
                        "ISO2709 WRITE ERROR: invalid subfield %zu in %s\n",
                        j,
                        marc_field_get_tag(field)
                    );

                    free(output);
                    free(fields);

                    return -1;
                }


                /*
                 * Subfield delimiter.
                 */

                if (append_byte(
                    output,
                    record_length,
                    &position,
                    MARC_SUBFIELD_DELIMITER
                ) != 0)
                {
                    fprintf(
                        stderr,
                        "ISO2709 WRITE ERROR: failed writing subfield delimiter\n"
                    );

                    free(output);
                    free(fields);

                    return -1;
                }


                /*
                 * Subfield code.
                 */

                if (append_byte(
                    output,
                    record_length,
                    &position,
                    (unsigned char)code
                ) != 0)
                {
                    fprintf(
                        stderr,
                        "ISO2709 WRITE ERROR: failed writing subfield code\n"
                    );

                    free(output);
                    free(fields);

                    return -1;
                }


                /*
                 * Subfield value.
                 */

                if (append_bytes(
                    output,
                    record_length,
                    &position,
                    value,
                    strlen(value)
                ) != 0)
                {
                    fprintf(
                        stderr,
                        "ISO2709 WRITE ERROR: failed writing subfield value\n"
                    );

                    free(output);
                    free(fields);

                    return -1;
                }
            }


            /*
             * Field terminator.
             */

            if (append_byte(
                output,
                record_length,
                &position,
                MARC_FIELD_TERMINATOR
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 WRITE ERROR: failed writing data field terminator\n"
                );

                free(output);
                free(fields);

                return -1;
            }
        }
    }


    /*
     * Record terminator.
     */

    if (append_byte(
        output,
        record_length,
        &position,
        MARC_RECORD_TERMINATOR
    ) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: failed writing record terminator\n"
        );

        free(output);
        free(fields);

        return -1;
    }


    /*
     * Verify final size.
     */

    if (position != record_length)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: generated %zu bytes, expected %zu\n",
            position,
            record_length
        );

        free(output);
        free(fields);

        return -1;
    }


    /*
     * Write complete record.
     */

    if (fwrite(
        output,
        1,
        record_length,
        stream
    ) != record_length)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: fwrite failed\n"
        );

        free(output);
        free(fields);

        return -1;
    }


    if (fflush(stream) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 WRITE ERROR: fflush failed\n"
        );

        free(output);
        free(fields);

        return -1;
    }


    fprintf(
        stderr,
        "ISO2709 WRITE SUCCESS: wrote %zu bytes\n",
        record_length
    );


    free(output);
    free(fields);

    return 0;
}


/*
 * --------------------------------------------------------------------------
 * ISO 2709 reader
 * --------------------------------------------------------------------------
 */

int marc_record_read(
    MARC_Record *record,
    FILE *stream
)
{
    unsigned char leader[24];

    size_t record_length;
    size_t base_address;

    unsigned char *buffer;

    size_t directory_start;
    size_t directory_end;
    size_t directory_position;


    /*
     * Validate arguments.
     */

    if (record == NULL ||
        stream == NULL)
    {
        return -1;
    }


    /*
     * Read leader.
     */

    if (fread(
        leader,
        1,
        24,
        stream
    ) != 24)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: failed reading leader\n"
        );

        return -1;
    }


    /*
     * Parse record length from leader
     * positions 00-04.
     */

    if (parse_ascii_number(
        leader,
        5,
        &record_length
    ) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: invalid record length\n"
        );

        return -1;
    }


    if (record_length < 25 ||
        record_length > MARC_MAX_RECORD_LENGTH)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: invalid record length %zu\n",
            record_length
        );

        return -1;
    }


    /*
     * Allocate complete record.
     */

    buffer =
        malloc(record_length);

    if (buffer == NULL)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: failed allocating record buffer\n"
        );

        return -1;
    }


    memcpy(
        buffer,
        leader,
        24
    );


    /*
     * Read remaining bytes.
     */

    if (fread(
        buffer + 24,
        1,
        record_length - 24,
        stream
    ) != record_length - 24)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: failed reading complete record\n"
        );

        free(buffer);

        return -1;
    }


    /*
     * Verify record terminator.
     */

    if (buffer[record_length - 1] !=
        MARC_RECORD_TERMINATOR)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: missing record terminator\n"
        );

        free(buffer);

        return -1;
    }


    /*
     * Parse base address from leader positions 12-16.
     */

    if (parse_ascii_number(
        buffer + 12,
        5,
        &base_address
    ) != 0)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: invalid base address\n"
        );

        free(buffer);

        return -1;
    }


    if (base_address < 25 ||
        base_address >= record_length)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: base address %zu is invalid\n",
            base_address
        );

        free(buffer);

        return -1;
    }


    /*
     * Restore leader.
     */

    {
        char leader_string[25];

        memcpy(
            leader_string,
            buffer,
            24
        );

        leader_string[24] = '\0';

        if (marc_record_set_leader(
            record,
            leader_string
        ) != 0)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: failed restoring leader\n"
            );

            free(buffer);

            return -1;
        }
    }


    /*
     * Directory.
     *
     * Directory begins immediately after the leader.
     *
     * The final byte before base_address is
     * the directory terminator.
     */

    directory_start = 24;

    directory_end =
        base_address - 1;


    if (buffer[directory_end] !=
        MARC_FIELD_TERMINATOR)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: missing directory terminator\n"
        );

        free(buffer);

        return -1;
    }


    directory_position =
        directory_start;


    while (directory_position <
           directory_end)
    {
        char tag[4];

        size_t length;
        size_t field_position;

        MARC_Field *field;


        /*
         * Every directory entry is exactly
         * 12 bytes.
         */

        if (directory_position + 12 >
            directory_end)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: incomplete directory entry\n"
            );

            free(buffer);

            return -1;
        }


        /*
         * Tag.
         */

        memcpy(
            tag,
            buffer + directory_position,
            3
        );

        tag[3] = '\0';


        /*
         * Field length.
         */

        if (parse_ascii_number(
            buffer + directory_position + 3,
            4,
            &length
        ) != 0)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: invalid length for field %s\n",
                tag
            );

            free(buffer);

            return -1;
        }


        /*
         * Field starting position.
         */

        if (parse_ascii_number(
            buffer + directory_position + 7,
            5,
            &field_position
        ) != 0)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: invalid position for field %s\n",
                tag
            );

            free(buffer);

            return -1;
        }


        if (length == 0)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: field %s has zero length\n",
                tag
            );

            free(buffer);

            return -1;
        }


        /*
         * Verify that the complete field lies
         * inside the variable-field area.
         */

        if (field_position >=
            record_length - base_address)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: field %s position is out of bounds\n",
                tag
            );

            free(buffer);

            return -1;
        }


        if (length >
            (record_length - 1) -
            (base_address + field_position))
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: field %s length is out of bounds\n",
                tag
            );

            free(buffer);

            return -1;
        }


        /*
         * Create field.
         */

        if (tag[0] == '0' &&
            tag[1] == '0' &&
            tag[2] >= '1' &&
            tag[2] <= '9')
        {
            field =
                marc_field_create(
                    tag,
                    '\0',
                    '\0'
                );
        }
        else
        {
            char indicator1;
            char indicator2;

            /*
             * A data field must contain at least
             * two indicator bytes plus terminator.
             */

            if (length < 3)
            {
                fprintf(
                    stderr,
                    "ISO2709 READ ERROR: data field %s is too short\n",
                    tag
                );

                free(buffer);

                return -1;
            }

            indicator1 =
                (char)buffer[
                    base_address +
                    field_position
                ];

            indicator2 =
                (char)buffer[
                    base_address +
                    field_position +
                    1
                ];

            field =
                marc_field_create(
                    tag,
                    indicator1,
                    indicator2
                );
        }


        if (field == NULL)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: failed creating field %s\n",
                tag
            );

            free(buffer);

            return -1;
        }


        /*
         * ------------------------------------------------------------------
         * Control field
         * ------------------------------------------------------------------
         */

        if (tag[0] == '0' &&
            tag[1] == '0' &&
            tag[2] >= '1' &&
            tag[2] <= '9')
        {
            size_t value_length;
            char *value;

            /*
             * Control fields must end in
             * a field terminator.
             */

            if (buffer[
                    base_address +
                    field_position +
                    length - 1
                ] != MARC_FIELD_TERMINATOR)
            {
                fprintf(
                    stderr,
                    "ISO2709 READ ERROR: control field %s missing terminator\n",
                    tag
                );

                marc_field_free(field);
                free(buffer);

                return -1;
            }


            value_length =
                length - 1;


            value =
                malloc(value_length + 1);

            if (value == NULL)
            {
                marc_field_free(field);
                free(buffer);

                return -1;
            }


            memcpy(
                value,
                buffer +
                    base_address +
                    field_position,
                value_length
            );

            value[value_length] =
                '\0';


            if (marc_field_set_control_value(
                field,
                value
            ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 READ ERROR: failed setting control field %s\n",
                    tag
                );

                free(value);
                marc_field_free(field);
                free(buffer);

                return -1;
            }


            free(value);
        }


        /*
         * ------------------------------------------------------------------
         * Data field
         * ------------------------------------------------------------------
         */

        else
        {
            size_t field_start;
            size_t field_end;
            size_t cursor;


            field_start =
                base_address +
                field_position;


            /*
             * Field length includes the field terminator.
             */

            field_end =
                field_start +
                length -
                1;


            /*
             * Verify field terminator.
             */

            if (buffer[field_end] !=
                MARC_FIELD_TERMINATOR)
            {
                fprintf(
                    stderr,
                    "ISO2709 READ ERROR: data field %s missing terminator\n",
                    tag
                );

                marc_field_free(field);
                free(buffer);

                return -1;
            }


            /*
             * Skip the two indicators.
             */

            cursor =
                field_start + 2;


            while (cursor < field_end)
            {
                char code;

                size_t value_start;
                size_t value_length;

                char *value;


                /*
                 * Every subfield must begin with
                 * the subfield delimiter.
                 */

                if (buffer[cursor] !=
                    MARC_SUBFIELD_DELIMITER)
                {
                    fprintf(
                        stderr,
                        "ISO2709 READ ERROR: expected subfield delimiter in %s\n",
                        tag
                    );

                    marc_field_free(field);
                    free(buffer);

                    return -1;
                }


                cursor++;


                /*
                 * Subfield code.
                 */

                if (cursor >= field_end)
                {
                    fprintf(
                        stderr,
                        "ISO2709 READ ERROR: missing subfield code in %s\n",
                        tag
                    );

                    marc_field_free(field);
                    free(buffer);

                    return -1;
                }


                code =
                    (char)buffer[cursor];

                cursor++;


                /*
                 * Subfield value starts immediately
                 * after the code.
                 */

                value_start =
                    cursor;


                /*
                 * Find the next subfield delimiter
                 * or field terminator.
                 */

                while (cursor < field_end &&
                       buffer[cursor] !=
                       MARC_SUBFIELD_DELIMITER)
                {
                    cursor++;
                }


                value_length =
                    cursor -
                    value_start;


                value =
                    malloc(value_length + 1);

                if (value == NULL)
                {
                    marc_field_free(field);
                    free(buffer);

                    return -1;
                }


                memcpy(
                    value,
                    buffer + value_start,
                    value_length
                );

                value[value_length] =
                    '\0';


                if (marc_field_add_subfield(
                    field,
                    code,
                    value
                ) != 0)
                {
                    fprintf(
                        stderr,
                        "ISO2709 READ ERROR: failed adding subfield $%c to %s\n",
                        code,
                        tag
                    );

                    free(value);
                    marc_field_free(field);
                    free(buffer);

                    return -1;
                }


                free(value);
            }
        }


        /*
         * Add decoded field to record.
         */

        if (marc_record_add_field(
            record,
            field
        ) != 0)
        {
            fprintf(
                stderr,
                "ISO2709 READ ERROR: failed adding field %s to record\n",
                tag
            );

            marc_field_free(field);
            free(buffer);

            return -1;
        }


        directory_position += 12;
    }


    /*
     * The directory must contain at least
     * complete 12-byte entries.
     */

    if ((directory_end -
         directory_start) % 12 != 0)
    {
        fprintf(
            stderr,
            "ISO2709 READ ERROR: directory size is invalid\n"
        );

        free(buffer);

        return -1;
    }


    free(buffer);

    fprintf(
        stderr,
        "ISO2709 READ SUCCESS: read %zu bytes\n",
        record_length
    );

    return 0;
}