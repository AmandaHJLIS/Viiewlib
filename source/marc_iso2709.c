#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "viiewlib/marc.h"

#define MARC_SUBFIELD_DELIMITER 0x1F
#define MARC_FIELD_TERMINATOR   0x1E
#define MARC_RECORD_TERMINATOR  0x1D

#define MARC_LEADER_LENGTH      24
#define MARC_DIRECTORY_ENTRY    12


/*
 * Determine whether a MARC tag is a control field.
 *
 * MARC 21 control fields occupy the 001-009 range.
 *
 * Control fields contain their value directly and do not
 * contain indicators or subfields.
 */
static int is_control_field(
    const char *tag
)
{
    if (tag == NULL)
    {
        return 0;
    }

    return (
        tag[0] == '0' &&
        tag[1] == '0' &&
        tag[2] >= '1' &&
        tag[2] <= '9'
    );
}


/*
 * Validate a MARC field tag.
 *
 * MARC tags are exactly three characters.
 */
static int is_valid_tag(
    const char *tag
)
{
    if (tag == NULL)
    {
        return 0;
    }

    if (tag[0] == '\0' ||
        tag[1] == '\0' ||
        tag[2] == '\0')
    {
        return 0;
    }

    if (tag[3] != '\0')
    {
        return 0;
    }

    return 1;
}


/*
 * Check whether a value fits inside an ISO 2709
 * fixed-width decimal field.
 *
 * Examples:
 *
 *     4 digits -> 0000 through 9999
 *     5 digits -> 00000 through 99999
 */
static int fits_decimal_width(
    size_t value,
    size_t digits
)
{
    size_t maximum;
    size_t i;

    maximum = 1;

    for (i = 0; i < digits; i++)
    {
        maximum *= 10;
    }

    return value < maximum;
}


/*
 * Write a fixed-width decimal value.
 *
 * The caller must ensure that the value fits within
 * the requested number of digits.
 *
 * The output is zero-filled on the left.
 *
 * Examples:
 *
 *     width 4, value 25  -> "0025"
 *     width 5, value 133 -> "00133"
 */
static void write_decimal(
    unsigned char *buffer,
    size_t width,
    size_t value
)
{
    size_t i;

    for (i = width; i > 0; i--)
    {
        buffer[i - 1] =
            (unsigned char)(
                '0' +
                (value % 10)
            );

        value /= 10;
    }
}


/*
 * Compare two fields by MARC tag.
 */
static int compare_fields(
    MARC_Field *a,
    MARC_Field *b
)
{
    const char *tag_a;
    const char *tag_b;

    tag_a = marc_field_get_tag(a);
    tag_b = marc_field_get_tag(b);

    if (tag_a == NULL || tag_b == NULL)
    {
        return 0;
    }

    return strcmp(tag_a, tag_b);
}


/*
 * Return the encoded length of a MARC field.
 */
static size_t field_length(
    MARC_Field *field
)
{
    const char *tag;
    size_t length;
    size_t count;
    size_t i;

    if (field == NULL)
    {
        return 0;
    }

    tag =
        marc_field_get_tag(
            field
        );

    if (tag == NULL)
    {
        return 0;
    }

    /*
     * Control fields contain their value directly.
     */
    if (is_control_field(tag))
    {
        const char *value;

        value =
            marc_field_get_control_value(
                field
            );

        if (value == NULL)
        {
            /*
             * An empty control field still contains
             * its field terminator.
             */
            return 1;
        }

        return strlen(value) + 1;
    }

    /*
     * Data field:
     *
     * two indicators
     */
    length = 2;

    count =
        marc_field_get_subfield_count(
            field
        );

    for (i = 0; i < count; i++)
    {
        MARC_Subfield *subfield;
        const char *value;

        /*
         * marc_field_get_subfield() takes an index.
         */
        subfield =
            marc_field_get_subfield(
                field,
                i
            );

        if (subfield == NULL)
        {
            continue;
        }

        value =
            marc_subfield_get_value(
                subfield
            );

        /*
         * Subfield delimiter + code.
         */
        length += 2;

        if (value != NULL)
        {
            length += strlen(value);
        }
    }

    /*
     * Field terminator.
     */
    length += 1;

    return length;
}


/*
 * Encode one MARC field.
 */
static int encode_field(
    MARC_Field *field,
    unsigned char *buffer,
    size_t buffer_size
)
{
    const char *tag;
    size_t required;
    size_t position;
    size_t count;
    size_t i;

    if (field == NULL || buffer == NULL)
    {
        return -1;
    }

    tag =
        marc_field_get_tag(
            field
        );

    if (tag == NULL)
    {
        return -1;
    }

    required =
        field_length(
            field
        );

    if (required == 0 ||
        required > buffer_size)
    {
        return -1;
    }

    /*
     * Control field.
     *
     * Control fields contain only their value followed
     * by the field terminator.
     */
    if (is_control_field(tag))
    {
        const char *value;
        size_t value_length;

        value =
            marc_field_get_control_value(
                field
            );

        if (value == NULL)
        {
            buffer[0] =
                MARC_FIELD_TERMINATOR;

            return 0;
        }

        value_length =
            strlen(value);

        memcpy(
            buffer,
            value,
            value_length
        );

        buffer[value_length] =
            MARC_FIELD_TERMINATOR;

        return 0;
    }

    /*
     * Data field indicators.
     */
    buffer[0] =
        (unsigned char)
            marc_field_get_indicator1(
                field
            );

    buffer[1] =
        (unsigned char)
            marc_field_get_indicator2(
                field
            );

    position = 2;

    count =
        marc_field_get_subfield_count(
            field
        );

    for (i = 0; i < count; i++)
    {
        MARC_Subfield *subfield;
        const char *value;
        char code;
        size_t value_length;

        subfield =
            marc_field_get_subfield(
                field,
                i
            );

        if (subfield == NULL)
        {
            continue;
        }

        code =
            marc_subfield_get_code(
                subfield
            );

        value =
            marc_subfield_get_value(
                subfield
            );

        buffer[position++] =
            MARC_SUBFIELD_DELIMITER;

        buffer[position++] =
            (unsigned char)code;

        if (value != NULL)
        {
            value_length =
                strlen(value);

            memcpy(
                buffer + position,
                value,
                value_length
            );

            position += value_length;
        }
    }

    buffer[position] =
        MARC_FIELD_TERMINATOR;

    return 0;
}


/*
 * Stable insertion sort by MARC tag.
 *
 * qsort() is not guaranteed to preserve the order of
 * equal elements. Repeated MARC fields must retain their
 * original order.
 */
static void stable_sort_fields(
    MARC_Field **fields,
    size_t field_count
)
{
    size_t i;

    for (i = 1; i < field_count; i++)
    {
        MARC_Field *current;
        size_t j;

        current = fields[i];
        j = i;

        while (
            j > 0 &&
            compare_fields(
                fields[j - 1],
                current
            ) > 0
        )
        {
            fields[j] =
                fields[j - 1];

            j--;
        }

        fields[j] = current;
    }
}


/*
 * Write one MARC 21 record in ISO 2709 format.
 */
int marc_record_write(
    const MARC_Record *record,
    FILE *file
)
{
    MARC_Field **fields;

    size_t field_count;
    size_t i;

    size_t directory_length;
    size_t data_length;
    size_t base_address;
    size_t record_length;

    size_t directory_position;
    size_t data_position;

    unsigned char *directory;
    unsigned char *data;

    unsigned char leader[MARC_LEADER_LENGTH];

    if (record == NULL || file == NULL)
    {
        return -1;
    }

    field_count =
        marc_record_get_field_count(
            record
        );

    /*
     * ISO 2709 records require at least one field.
     */
    if (field_count == 0)
    {
        return -1;
    }

    fields =
        malloc(
            sizeof(MARC_Field *) *
            field_count
        );

    if (fields == NULL)
    {
        return -1;
    }

    /*
     * Collect and validate fields.
     */
    for (i = 0; i < field_count; i++)
    {
        const char *tag;

        fields[i] =
            marc_record_get_field(
                record,
                i
            );

        if (fields[i] == NULL)
        {
            free(fields);
            return -1;
        }

        tag =
            marc_field_get_tag(
                fields[i]
            );

        if (!is_valid_tag(tag))
        {
            free(fields);
            return -1;
        }
    }

    /*
     * Sort fields by MARC tag while preserving the
     * original order of repeated fields.
     */
    stable_sort_fields(
        fields,
        field_count
    );

    /*
     * Directory:
     *
     * 12 bytes per entry
     * plus directory terminator.
     */
    if (field_count >
        (SIZE_MAX - 1) /
        MARC_DIRECTORY_ENTRY)
    {
        free(fields);
        return -1;
    }

    directory_length =
        field_count *
        MARC_DIRECTORY_ENTRY;

    directory_length += 1;

    /*
     * Calculate field-data length.
     */
    data_length = 0;

    for (i = 0; i < field_count; i++)
    {
        size_t length;

        length =
            field_length(
                fields[i]
            );

        if (length == 0)
        {
            free(fields);
            return -1;
        }

        /*
         * ISO 2709 directory field length is four digits.
         */
        if (!fits_decimal_width(
                length,
                4
            ))
        {
            free(fields);
            return -1;
        }

        /*
         * Prevent size_t overflow.
         */
        if (data_length >
            SIZE_MAX - length)
        {
            free(fields);
            return -1;
        }

        data_length += length;
    }

    /*
     * Base address:
     *
     * leader + directory + directory terminator.
     */
    if (MARC_LEADER_LENGTH >
        SIZE_MAX - directory_length)
    {
        free(fields);
        return -1;
    }

    base_address =
        MARC_LEADER_LENGTH +
        directory_length;

    /*
     * ISO 2709 base address is five decimal digits.
     */
    if (!fits_decimal_width(
            base_address,
            5
        ))
    {
        free(fields);
        return -1;
    }

    /*
     * Complete ISO 2709 record:
     *
     * leader
     * directory
     * data
     * record terminator
     */
    if (base_address >
        SIZE_MAX - data_length)
    {
        free(fields);
        return -1;
    }

    record_length =
        base_address +
        data_length;

    if (record_length >
        SIZE_MAX - 1)
    {
        free(fields);
        return -1;
    }

    record_length += 1;

    /*
     * ISO 2709 record length is five decimal digits.
     */
    if (!fits_decimal_width(
            record_length,
            5
        ))
    {
        free(fields);
        return -1;
    }

    /*
     * Allocate directory.
     */
    directory =
        malloc(
            directory_length
        );

    if (directory == NULL)
    {
        free(fields);
        return -1;
    }

    /*
     * Allocate field data.
     */
    data =
        malloc(
            data_length
        );

    if (data == NULL)
    {
        free(directory);
        free(fields);
        return -1;
    }

    /*
     * Construct leader.
     */
    memset(
        leader,
        ' ',
        sizeof(leader)
    );

    /*
     * Record length.
     */
    write_decimal(
        leader,
        5,
        record_length
    );

    /*
     * Basic MARC leader values.
     */
    leader[5]  = 'n';
    leader[6]  = 'a';
    leader[7]  = ' ';
    leader[8]  = ' ';
    leader[9]  = 'a';
    leader[10] = '2';
    leader[11] = '2';

    /*
     * Base address of data.
     */
    write_decimal(
        leader + 12,
        5,
        base_address
    );

    leader[17] = ' ';
    leader[18] = ' ';
    leader[19] = ' ';
    leader[20] = '4';
    leader[21] = '5';
    leader[22] = '0';
    leader[23] = '0';

    directory_position = 0;
    data_position = 0;

    /*
     * Build directory and field data.
     */
    for (i = 0; i < field_count; i++)
    {
        const char *tag;
        size_t length;

        tag =
            marc_field_get_tag(
                fields[i]
            );

        length =
            field_length(
                fields[i]
            );

        if (!fits_decimal_width(
                length,
                4
            ))
        {
            free(data);
            free(directory);
            free(fields);
            return -1;
        }

        /*
         * Starting position is five decimal digits.
         */
        if (!fits_decimal_width(
                data_position,
                5
            ))
        {
            free(data);
            free(directory);
            free(fields);
            return -1;
        }

        /*
         * Tag.
         */
        directory[directory_position++] =
            (unsigned char)tag[0];

        directory[directory_position++] =
            (unsigned char)tag[1];

        directory[directory_position++] =
            (unsigned char)tag[2];

        /*
         * Field length: four digits.
         */
        write_decimal(
            directory +
                directory_position,
            4,
            length
        );

        directory_position += 4;

        /*
         * Starting position: five digits.
         */
        write_decimal(
            directory +
                directory_position,
            5,
            data_position
        );

        directory_position += 5;

        /*
         * Encode field data.
         */
        if (encode_field(
                fields[i],
                data + data_position,
                data_length - data_position
            ) != 0)
        {
            free(data);
            free(directory);
            free(fields);
            return -1;
        }

        data_position += length;
    }

    /*
     * Directory terminator.
     */
    directory[directory_position] =
        MARC_FIELD_TERMINATOR;

    /*
     * Write leader.
     */
    if (fwrite(
            leader,
            1,
            MARC_LEADER_LENGTH,
            file
        ) != MARC_LEADER_LENGTH)
    {
        free(data);
        free(directory);
        free(fields);
        return -1;
    }

    /*
     * Write directory.
     */
    if (fwrite(
            directory,
            1,
            directory_length,
            file
        ) != directory_length)
    {
        free(data);
        free(directory);
        free(fields);
        return -1;
    }

    /*
     * Write field data.
     */
    if (fwrite(
            data,
            1,
            data_length,
            file
        ) != data_length)
    {
        free(data);
        free(directory);
        free(fields);
        return -1;
    }

    /*
     * Record terminator.
     */
    if (fputc(
            MARC_RECORD_TERMINATOR,
            file
        ) == EOF)
    {
        free(data);
        free(directory);
        free(fields);
        return -1;
    }

    free(data);
    free(directory);
    free(fields);

    return 0;
}


/*
 * Read one ISO 2709 MARC record.
 */
int marc_record_read(
    MARC_Record *record,
    FILE *file
)
{
    unsigned char leader[MARC_LEADER_LENGTH];
    unsigned char *buffer;

    long file_size_long;

    size_t file_size;
    size_t record_length;
    size_t base_address;

    size_t directory_end;
    size_t directory_position;

    if (record == NULL || file == NULL)
    {
        return -1;
    }

    /*
     * Determine file size.
     */
    if (fseek(
            file,
            0,
            SEEK_END
        ) != 0)
    {
        return -1;
    }

    file_size_long =
        ftell(file);

    if (file_size_long < 0)
    {
        return -1;
    }

    file_size =
        (size_t)file_size_long;

    if (fseek(
            file,
            0,
            SEEK_SET
        ) != 0)
    {
        return -1;
    }

    if (file_size <
        MARC_LEADER_LENGTH)
    {
        fprintf(
            stderr,
            "ISO2709 READ: file too small for leader\n"
        );

        return -1;
    }

    if (fread(
            leader,
            1,
            MARC_LEADER_LENGTH,
            file
        ) != MARC_LEADER_LENGTH)
    {
        return -1;
    }

    /*
     * Parse record length.
     */
    {
        char length_text[6];

        memcpy(
            length_text,
            leader,
            5
        );

        length_text[5] = '\0';

        record_length =
            (size_t)strtoul(
                length_text,
                NULL,
                10
            );
    }

    /*
     * Parse base address.
     */
    {
        char base_text[6];

        memcpy(
            base_text,
            leader + 12,
            5
        );

        base_text[5] = '\0';

        base_address =
            (size_t)strtoul(
                base_text,
                NULL,
                10
            );
    }

    if (record_length <
        MARC_LEADER_LENGTH + 1)
    {
        fprintf(
            stderr,
            "ISO2709 READ: invalid record length: %lu\n",
            (unsigned long)record_length
        );

        return -1;
    }

    if (record_length > file_size)
    {
        fprintf(
            stderr,
            "ISO2709 READ: record length exceeds file size\n"
        );

        return -1;
    }

    if (base_address <
        MARC_LEADER_LENGTH)
    {
        fprintf(
            stderr,
            "ISO2709 READ: invalid base address: %lu\n",
            (unsigned long)base_address
        );

        return -1;
    }

    if (base_address >= record_length)
    {
        fprintf(
            stderr,
            "ISO2709 READ: base address outside record\n"
        );

        return -1;
    }

    /*
     * Read the complete record.
     */
    buffer =
        malloc(record_length);

    if (buffer == NULL)
    {
        return -1;
    }

    if (fseek(
            file,
            0,
            SEEK_SET
        ) != 0)
    {
        free(buffer);
        return -1;
    }

    if (fread(
            buffer,
            1,
            record_length,
            file
        ) != record_length)
    {
        free(buffer);
        return -1;
    }

    /*
     * Record terminator.
     */
    if (buffer[record_length - 1] !=
        MARC_RECORD_TERMINATOR)
    {
        fprintf(
            stderr,
            "ISO2709 READ: missing record terminator\n"
        );

        free(buffer);
        return -1;
    }

    /*
     * Directory terminator.
     */
    if (base_address <=
        MARC_LEADER_LENGTH)
    {
        free(buffer);
        return -1;
    }

    directory_end =
        base_address - 1;

    if (buffer[directory_end] !=
        MARC_FIELD_TERMINATOR)
    {
        fprintf(
            stderr,
            "ISO2709 READ: missing directory terminator\n"
        );

        free(buffer);
        return -1;
    }

    if ((directory_end -
         MARC_LEADER_LENGTH) %
        MARC_DIRECTORY_ENTRY != 0)
    {
        fprintf(
            stderr,
            "ISO2709 READ: malformed directory length\n"
        );

        free(buffer);
        return -1;
    }

    directory_position =
        MARC_LEADER_LENGTH;

    while (directory_position <
           directory_end)
    {
        char tag[4];
        char length_text[5];
        char position_text[6];

        size_t length;
        size_t field_position;
        size_t field_start;
        size_t field_end;

        MARC_Field *field;

        /*
         * Directory entry must fit.
         */
        if (directory_position +
                MARC_DIRECTORY_ENTRY >
            directory_end)
        {
            free(buffer);
            return -1;
        }

        /*
         * Tag.
         */
        tag[0] =
            (char)buffer[
                directory_position
            ];

        tag[1] =
            (char)buffer[
                directory_position + 1
            ];

        tag[2] =
            (char)buffer[
                directory_position + 2
            ];

        tag[3] = '\0';

        /*
         * Field length.
         */
        memcpy(
            length_text,
            buffer +
                directory_position + 3,
            4
        );

        length_text[4] = '\0';

        length =
            (size_t)strtoul(
                length_text,
                NULL,
                10
            );

        /*
         * Field starting position.
         */
        memcpy(
            position_text,
            buffer +
                directory_position + 7,
            5
        );

        position_text[5] = '\0';

        field_position =
            (size_t)strtoul(
                position_text,
                NULL,
                10
            );

        if (length == 0)
        {
            fprintf(
                stderr,
                "ISO2709 READ: zero-length field %s\n",
                tag
            );

            free(buffer);
            return -1;
        }

        field_start =
            base_address +
            field_position;

        if (field_start <
                base_address ||
            field_start >=
                record_length)
        {
            fprintf(
                stderr,
                "ISO2709 READ: invalid field start for %s\n",
                tag
            );

            free(buffer);
            return -1;
        }

        if (length >
            record_length - field_start)
        {
            fprintf(
                stderr,
                "ISO2709 READ: field %s exceeds record\n",
                tag
            );

            free(buffer);
            return -1;
        }

        field_end =
            field_start +
            length -
            1;

        if (buffer[field_end] !=
            MARC_FIELD_TERMINATOR)
        {
            fprintf(
                stderr,
                "ISO2709 READ: field %s missing terminator\n",
                tag
            );

            free(buffer);
            return -1;
        }

        /*
         * Debug information.
         */
        fprintf(
            stderr,
            "ISO2709 READ DEBUG: field %s "
            "start=%lu length=%lu end=%lu\n",
            tag,
            (unsigned long)field_start,
            (unsigned long)length,
            (unsigned long)field_end
        );

        /*
         * Dump raw field bytes.
         */
        {
            size_t debug_position;

            fprintf(
                stderr,
                "ISO2709 READ DEBUG: raw %s: ",
                tag
            );

            for (
                debug_position = field_start;
                debug_position <
                    field_start + length;
                debug_position++
            )
            {
                fprintf(
                    stderr,
                    "%02X ",
                    (unsigned int)
                        buffer[debug_position]
                );
            }

            fprintf(
                stderr,
                "\n"
            );
        }

        /*
         * Control fields.
         */
        if (is_control_field(tag))
        {
            size_t value_length;
            char *value;

            value_length =
                length - 1;

            value =
                malloc(
                    value_length + 1
                );

            if (value == NULL)
            {
                free(buffer);
                return -1;
            }

            memcpy(
                value,
                buffer + field_start,
                value_length
            );

            value[value_length] =
                '\0';

            fprintf(
                stderr,
                "ISO2709 READ DEBUG: "
                "control field %s "
                "value=[%s]\n",
                tag,
                value
            );

            if (marc_record_set_control_field(
                    record,
                    tag,
                    value
                ) != 0)
            {
                free(value);
                free(buffer);
                return -1;
            }

            free(value);
        }
        else
        {
            char indicator1;
            char indicator2;
            size_t cursor;

            indicator1 =
                (char)buffer[
                    field_start
                ];

            indicator2 =
                (char)buffer[
                    field_start + 1
                ];

            field =
                marc_field_create(
                    tag,
                    indicator1,
                    indicator2
                );

            if (field == NULL)
            {
                free(buffer);
                return -1;
            }

            /*
             * Skip the two indicators.
             */
            cursor =
                field_start + 2;

            fprintf(
                stderr,
                "ISO2709 READ DEBUG: "
                "parsing data field %s\n",
                tag
            );

            while (cursor < field_end)
            {
                char code;

                size_t value_start;
                size_t value_length;

                char *value;

                int add_result;

                /*
                 * Expect subfield delimiter.
                 */
                if (buffer[cursor] !=
                    MARC_SUBFIELD_DELIMITER)
                {
                    fprintf(
                        stderr,
                        "ISO2709 READ: field %s "
                        "expected subfield delimiter "
                        "at offset %lu, got 0x%02X\n",
                        tag,
                        (unsigned long)
                            (cursor - field_start),
                        (unsigned int)
                            buffer[cursor]
                    );

                    marc_field_free(field);
                    free(buffer);
                    return -1;
                }

                fprintf(
                    stderr,
                    "ISO2709 READ DEBUG: %s "
                    "subfield delimiter found "
                    "at offset %lu\n",
                    tag,
                    (unsigned long)
                        (cursor - field_start)
                );

                cursor++;

                if (cursor >= field_end)
                {
                    fprintf(
                        stderr,
                        "ISO2709 READ: field %s "
                        "missing subfield code\n",
                        tag
                    );

                    marc_field_free(field);
                    free(buffer);
                    return -1;
                }

                code =
                    (char)buffer[cursor];

                fprintf(
                    stderr,
                    "ISO2709 READ DEBUG: %s "
                    "subfield code = [%c] "
                    "(0x%02X)\n",
                    tag,
                    code,
                    (unsigned int)
                        (unsigned char)code
                );

                cursor++;

                value_start =
                    cursor;

                /*
                 * Read until the next subfield
                 * delimiter or field terminator.
                 */
                while (
                    cursor < field_end &&
                    buffer[cursor] !=
                        MARC_SUBFIELD_DELIMITER
                )
                {
                    cursor++;
                }

                value_length =
                    cursor -
                    value_start;

                value =
                    malloc(
                        value_length + 1
                    );

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

                fprintf(
                    stderr,
                    "ISO2709 READ DEBUG: %s "
                    "subfield $%c "
                    "value_length=%lu "
                    "value=[%s]\n",
                    tag,
                    code,
                    (unsigned long)value_length,
                    value
                );

                /*
                 * Critical reconstruction step.
                 */
                add_result =
                    marc_field_add_subfield(
                        field,
                        code,
                        value
                    );

                fprintf(
                    stderr,
                    "ISO2709 READ DEBUG: %s "
                    "add_subfield($%c) returned %d\n",
                    tag,
                    code,
                    add_result
                );

                fprintf(
                    stderr,
                    "ISO2709 READ DEBUG: %s "
                    "subfield count now %lu\n",
                    tag,
                    (unsigned long)
                        marc_field_get_subfield_count(
                            field
                        )
                );

                if (add_result != 0)
                {
                    fprintf(
                        stderr,
                        "ISO2709 READ: failed to add "
                        "subfield $%c to %s\n",
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

            fprintf(
                stderr,
                "ISO2709 READ DEBUG: field %s "
                "finished with %lu subfields\n",
                tag,
                (unsigned long)
                    marc_field_get_subfield_count(
                        field
                    )
            );

            /*
             * Transfer ownership to the record.
             */
            if (marc_record_add_field(
                    record,
                    field
                ) != 0)
            {
                fprintf(
                    stderr,
                    "ISO2709 READ: failed to add "
                    "field %s to record\n",
                    tag
                );

                marc_field_free(field);
                free(buffer);
                return -1;
            }

            fprintf(
                stderr,
                "ISO2709 READ DEBUG: field %s "
                "added to record\n",
                tag
            );
        }

        directory_position +=
            MARC_DIRECTORY_ENTRY;
    }

    free(buffer);

    return 0;
}