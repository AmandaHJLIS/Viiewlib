#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define MARC_SUBFIELD_DELIMITER 0x1F
#define MARC_FIELD_TERMINATOR   0x1E
#define MARC_RECORD_TERMINATOR  0x1D
#define MARC_LEADER_LENGTH      24
#define MARC_DIRECTORY_ENTRY    12


static int is_control_field(const char *tag)
{
    return tag != NULL &&
           tag[0] == '0' &&
           tag[1] == '0';
}


static size_t field_length(const MARC_Field *field)
{
    size_t length = 0;

    if (marc_field_is_control_field(field))
    {
        const char *value =
            marc_field_get_control_value(field);

        if (value != NULL)
            length = strlen(value);

        return length + 1;
    }

    /*
     * Two indicator characters.
     */
    length = 2;

    size_t subfield_count =
        marc_field_get_subfield_count(field);

    for (size_t i = 0; i < subfield_count; ++i)
    {
        const MARC_Subfield *subfield =
            marc_field_get_subfield(field, i);

        if (subfield == NULL)
            continue;

        const char *value =
            marc_subfield_get_value(subfield);

        /*
         * Subfield delimiter + subfield code.
         */
        length += 2;

        if (value != NULL)
            length += strlen(value);
    }

    /*
     * Field terminator.
     */
    return length + 1;
}


static size_t encode_field(
    const MARC_Field *field,
    unsigned char *buffer,
    size_t buffer_size
)
{
    size_t offset = 0;

    if (field == NULL || buffer == NULL)
        return 0;

    size_t required_length =
        field_length(field);

    if (buffer_size < required_length)
        return 0;

    /*
     * Control field.
     */
    if (marc_field_is_control_field(field))
    {
        const char *value =
            marc_field_get_control_value(field);

        if (value != NULL)
        {
            size_t value_length =
                strlen(value);

            memcpy(
                buffer + offset,
                value,
                value_length
            );

            offset += value_length;
        }

        buffer[offset++] =
            MARC_FIELD_TERMINATOR;

        return offset;
    }

    /*
     * Data field indicators.
     */
    buffer[offset++] =
        (unsigned char)marc_field_get_indicator1(field);

    buffer[offset++] =
        (unsigned char)marc_field_get_indicator2(field);

    size_t subfield_count =
        marc_field_get_subfield_count(field);

    for (size_t i = 0; i < subfield_count; ++i)
    {
        const MARC_Subfield *subfield =
            marc_field_get_subfield(field, i);

        if (subfield == NULL)
            continue;

        const char *value =
            marc_subfield_get_value(subfield);

        buffer[offset++] =
            MARC_SUBFIELD_DELIMITER;

        buffer[offset++] =
            (unsigned char)marc_subfield_get_code(subfield);

        if (value != NULL)
        {
            size_t value_length =
                strlen(value);

            memcpy(
                buffer + offset,
                value,
                value_length
            );

            offset += value_length;
        }
    }

    buffer[offset++] =
        MARC_FIELD_TERMINATOR;

    return offset;
}


static void stable_sort_fields(
    MARC_Field **fields,
    size_t count
)
{
    /*
     * Stable insertion sort.
     *
     * This keeps repeated MARC fields in their original order.
     */
    for (size_t i = 1; i < count; ++i)
    {
        MARC_Field *current =
            fields[i];

        size_t j = i;

        while (j > 0)
        {
            const char *previous_tag =
                marc_field_get_tag(fields[j - 1]);

            const char *current_tag =
                marc_field_get_tag(current);

            if (strcmp(previous_tag, current_tag) <= 0)
                break;

            fields[j] =
                fields[j - 1];

            --j;
        }

        fields[j] =
            current;
    }
}


int marc_record_write(
    const MARC_Record *record,
    FILE *stream
)
{
    if (record == NULL || stream == NULL)
        return -1;

    size_t field_count =
        marc_record_get_field_count(record);

    MARC_Field **fields = NULL;

    if (field_count > 0)
    {
        fields = malloc(
            field_count * sizeof(MARC_Field *)
        );

        if (fields == NULL)
            return -1;

        for (size_t i = 0; i < field_count; ++i)
        {
            fields[i] =
                marc_record_get_field(record, i);
        }

        stable_sort_fields(
            fields,
            field_count
        );
    }

    /*
     * Directory length:
     *
     * 12 bytes per field
     * + directory terminator
     */
    size_t directory_length =
        field_count * MARC_DIRECTORY_ENTRY;

    directory_length += 1;

    /*
     * Base address:
     *
     * 24-byte leader
     * + directory
     */
    size_t base_address =
        MARC_LEADER_LENGTH +
        directory_length;

    /*
     * Calculate field data length.
     */
    size_t data_length = 0;

    for (size_t i = 0; i < field_count; ++i)
    {
        data_length +=
            field_length(fields[i]);
    }

    /*
     * Complete ISO 2709 record:
     *
     * leader
     * directory
     * field data
     * record terminator
     */
    size_t record_length =
        MARC_LEADER_LENGTH +
        directory_length +
        data_length +
        1;

    if (record_length > 99999 ||
        base_address > 99999)
    {
        free(fields);
        return -1;
    }

    unsigned char *buffer =
        malloc(record_length);

    if (buffer == NULL)
    {
        free(fields);
        return -1;
    }

    memset(
        buffer,
        ' ',
        record_length
    );

    /*
     * --------------------------------------------------------
     * Leader
     * --------------------------------------------------------
     *
     * Preserve the leader from the MARC_Record when possible.
     */
    const char *existing_leader =
        marc_record_get_leader(record);

    if (existing_leader != NULL &&
        strlen(existing_leader) == MARC_LEADER_LENGTH)
    {
        memcpy(
            buffer,
            existing_leader,
            MARC_LEADER_LENGTH
        );
    }
    else
    {
        /*
         * Generic fallback leader.
         */
        memcpy(
            buffer,
            "00000nam a2200000   4500",
            MARC_LEADER_LENGTH
        );
    }

    /*
     * Write record length into leader positions 00-04.
     */
    char record_length_text[6];

    snprintf(
        record_length_text,
        sizeof(record_length_text),
        "%05zu",
        record_length
    );

    memcpy(
        buffer,
        record_length_text,
        5
    );

    /*
     * Write base address into leader positions 12-16.
     */
    char base_address_text[6];

    snprintf(
        base_address_text,
        sizeof(base_address_text),
        "%05zu",
        base_address
    );

    memcpy(
        buffer + 12,
        base_address_text,
        5
    );

    /*
     * --------------------------------------------------------
     * Directory
     * --------------------------------------------------------
     */
    size_t directory_offset =
        MARC_LEADER_LENGTH;

    size_t data_offset =
        base_address;

    size_t field_position = 0;

    for (size_t i = 0; i < field_count; ++i)
    {
        MARC_Field *field =
            fields[i];

        const char *tag =
            marc_field_get_tag(field);

        size_t length =
            field_length(field);

        /*
         * Three-character tag.
         */
        memcpy(
            buffer + directory_offset,
            tag,
            3
        );

        /*
         * Four-digit field length.
         */
        char length_text[5];

        snprintf(
            length_text,
            sizeof(length_text),
            "%04zu",
            length
        );

        memcpy(
            buffer + directory_offset + 3,
            length_text,
            4
        );

        /*
         * Five-digit field position.
         */
        char position_text[6];

        snprintf(
            position_text,
            sizeof(position_text),
            "%05zu",
            field_position
        );

        memcpy(
            buffer + directory_offset + 7,
            position_text,
            5
        );

        directory_offset +=
            MARC_DIRECTORY_ENTRY;

        /*
         * Encode field.
         */
        size_t encoded =
            encode_field(
                field,
                buffer + data_offset,
                record_length - data_offset
            );

        if (encoded != length)
        {
            free(buffer);
            free(fields);
            return -1;
        }

        data_offset += encoded;
        field_position += encoded;
    }

    /*
     * Directory terminator.
     */
    buffer[directory_offset] =
        MARC_FIELD_TERMINATOR;

    /*
     * Record terminator.
     */
    buffer[record_length - 1] =
        MARC_RECORD_TERMINATOR;

    /*
     * Write record.
     */
    size_t written =
        fwrite(
            buffer,
            1,
            record_length,
            stream
        );

    free(buffer);
    free(fields);

    if (written != record_length)
        return -1;

    return 0;
}


int marc_record_read(
    MARC_Record *record,
    FILE *stream
)
{
    if (record == NULL || stream == NULL)
        return -1;

    /*
     * --------------------------------------------------------
     * Read leader
     * --------------------------------------------------------
     *
     * The stream is intentionally not rewound. This allows
     * multiple MARC records to be read consecutively.
     */
    unsigned char leader[MARC_LEADER_LENGTH];

    size_t leader_read =
        fread(
            leader,
            1,
            MARC_LEADER_LENGTH,
            stream
        );

    /*
     * Clean EOF.
     */
    if (leader_read == 0 && feof(stream))
        return -1;

    /*
     * Partial leader.
     */
    if (leader_read != MARC_LEADER_LENGTH)
        return -1;

    /*
     * Parse five-digit record length.
     */
    char record_length_text[6];

    memcpy(
        record_length_text,
        leader,
        5
    );

    record_length_text[5] =
        '\0';

    char *endptr = NULL;

    long record_length_long =
        strtol(
            record_length_text,
            &endptr,
            10
        );

    if (endptr == record_length_text ||
        *endptr != '\0' ||
        record_length_long < MARC_LEADER_LENGTH ||
        record_length_long > 99999)
    {
        return -1;
    }

    size_t record_length =
        (size_t)record_length_long;

    /*
     * Parse five-digit base address.
     */
    char base_address_text[6];

    memcpy(
        base_address_text,
        leader + 12,
        5
    );

    base_address_text[5] =
        '\0';

    endptr = NULL;

    long base_address_long =
        strtol(
            base_address_text,
            &endptr,
            10
        );

    if (endptr == base_address_text ||
        *endptr != '\0' ||
        base_address_long < MARC_LEADER_LENGTH ||
        base_address_long >= (long)record_length)
    {
        return -1;
    }

    size_t base_address =
        (size_t)base_address_long;

    /*
     * The leader has already been consumed.
     * Read the rest of the record.
     */
    size_t remaining =
        record_length -
        MARC_LEADER_LENGTH;

    unsigned char *buffer =
        malloc(remaining);

    if (buffer == NULL)
        return -1;

    size_t bytes_read =
        fread(
            buffer,
            1,
            remaining,
            stream
        );

    if (bytes_read != remaining)
    {
        free(buffer);
        return -1;
    }

    /*
     * Final byte must be the record terminator.
     */
    if (buffer[remaining - 1] !=
        MARC_RECORD_TERMINATOR)
    {
        free(buffer);
        return -1;
    }

    /*
     * --------------------------------------------------------
     * Directory
     * --------------------------------------------------------
     *
     * The directory occupies the space between the leader and
     * the base address, ending with a field terminator.
     */
    size_t directory_offset =
        MARC_LEADER_LENGTH;

    size_t directory_length =
        base_address -
        MARC_LEADER_LENGTH;

    if (directory_length < 1)
    {
        free(buffer);
        return -1;
    }

    if (buffer[directory_length - 1] !=
        MARC_FIELD_TERMINATOR)
    {
        free(buffer);
        return -1;
    }

    size_t directory_data_length =
        directory_length - 1;

    if (directory_data_length %
            MARC_DIRECTORY_ENTRY != 0)
    {
        free(buffer);
        return -1;
    }

    size_t field_count =
        directory_data_length /
        MARC_DIRECTORY_ENTRY;

    /*
     * --------------------------------------------------------
     * Preserve the original MARC leader.
     * --------------------------------------------------------
     */
    char leader_text[
        MARC_LEADER_LENGTH + 1
    ];

    memcpy(
        leader_text,
        leader,
        MARC_LEADER_LENGTH
    );

    leader_text[MARC_LEADER_LENGTH] =
        '\0';

    if (marc_record_set_leader(
            record,
            leader_text
        ) != 0)
    {
        free(buffer);
        return -1;
    }

    /*
     * --------------------------------------------------------
     * Parse directory entries.
     * --------------------------------------------------------
     */
    for (size_t i = 0; i < field_count; ++i)
    {
        /*
         * Directory entries are stored immediately after the
         * leader in the complete record.
         */
        size_t entry_offset =
            i * MARC_DIRECTORY_ENTRY;

        /*
         * buffer starts immediately after the leader, so the
         * directory begins at buffer offset zero.
         */
        size_t buffer_entry_offset =
            entry_offset;

        /*
         * Tag.
         */
        char tag[4];

        memcpy(
            tag,
            buffer + buffer_entry_offset,
            3
        );

        tag[3] =
            '\0';

        /*
         * Field length.
         */
        char field_length_text[5];

        memcpy(
            field_length_text,
            buffer +
                buffer_entry_offset +
                3,
            4
        );

        field_length_text[4] =
            '\0';

        endptr = NULL;

        long field_length_long =
            strtol(
                field_length_text,
                &endptr,
                10
            );

        if (endptr == field_length_text ||
            *endptr != '\0' ||
            field_length_long < 1)
        {
            free(buffer);
            return -1;
        }

        size_t current_field_length =
            (size_t)field_length_long;

        /*
         * Field position.
         */
        char field_position_text[6];

        memcpy(
            field_position_text,
            buffer +
                buffer_entry_offset +
                7,
            5
        );

        field_position_text[5] =
            '\0';

        endptr = NULL;

        long field_position_long =
            strtol(
                field_position_text,
                &endptr,
                10
            );

        if (endptr == field_position_text ||
            *endptr != '\0' ||
            field_position_long < 0)
        {
            free(buffer);
            return -1;
        }

        size_t current_field_position =
            (size_t)field_position_long;

        /*
         * Convert the directory position into an offset within
         * the remainder buffer.
         */
        size_t field_data_absolute =
            base_address +
            current_field_position;

        if (field_data_absolute < MARC_LEADER_LENGTH ||
            field_data_absolute >= record_length)
        {
            free(buffer);
            return -1;
        }

        size_t field_data_offset =
            field_data_absolute -
            MARC_LEADER_LENGTH;

        /*
         * Ensure the complete field fits inside the record.
         */
        if (field_data_offset >= remaining ||
            current_field_length >
                remaining - field_data_offset)
        {
            free(buffer);
            return -1;
        }

        /*
         * Every field must end with a field terminator.
         */
        if (buffer[
                field_data_offset +
                current_field_length -
                1
            ] != MARC_FIELD_TERMINATOR)
        {
            free(buffer);
            return -1;
        }

        /*
         * ----------------------------------------------------
         * Control field
         * ----------------------------------------------------
         */
        if (is_control_field(tag))
        {
            size_t value_length =
                current_field_length - 1;

            char *value =
                malloc(value_length + 1);

            if (value == NULL)
            {
                free(buffer);
                return -1;
            }

            memcpy(
                value,
                buffer + field_data_offset,
                value_length
            );

            value[value_length] =
                '\0';

            int result =
                marc_record_set_control_field(
                    record,
                    tag,
                    value
                );

            free(value);

            if (result != 0)
            {
                free(buffer);
                return -1;
            }

            continue;
        }

        /*
         * ----------------------------------------------------
         * Data field
         * ----------------------------------------------------
         */
        if (current_field_length < 3)
        {
            free(buffer);
            return -1;
        }

        size_t data_length =
            current_field_length - 1;

        unsigned char *field_data =
            buffer + field_data_offset;

        char indicator1 =
            (char)field_data[0];

        char indicator2 =
            (char)field_data[1];

        MARC_Field *field =
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

        size_t offset = 2;

        while (offset < data_length)
        {
            /*
             * A data field must contain a subfield delimiter.
             */
            if (field_data[offset] !=
                MARC_SUBFIELD_DELIMITER)
            {
                marc_field_free(field);
                free(buffer);
                return -1;
            }

            ++offset;

            /*
             * Need a subfield code.
             */
            if (offset >= data_length)
            {
                marc_field_free(field);
                free(buffer);
                return -1;
            }

            char code =
                (char)field_data[offset];

            ++offset;

            /*
             * Locate the next subfield delimiter.
             */
            size_t value_start =
                offset;

            while (offset < data_length &&
                   field_data[offset] !=
                       MARC_SUBFIELD_DELIMITER)
            {
                ++offset;
            }

            size_t value_length =
                offset - value_start;

            char *value =
                malloc(value_length + 1);

            if (value == NULL)
            {
                marc_field_free(field);
                free(buffer);
                return -1;
            }

            memcpy(
                value,
                field_data + value_start,
                value_length
            );

            value[value_length] =
                '\0';

            /*
             * Add the subfield using ViiewLib's public API.
             *
             * marc_field_add_subfield() creates the
             * MARC_Subfield internally.
             */
            if (marc_field_add_subfield(
                    field,
                    code,
                    value
                ) != 0)
            {
                free(value);
                marc_field_free(field);
                free(buffer);
                return -1;
            }

            free(value);
        }

        /*
         * Transfer the field to the record.
         */
        if (marc_record_add_field(
                record,
                field
            ) != 0)
        {
            marc_field_free(field);
            free(buffer);
            return -1;
        }
    }

    free(buffer);

    return 0;
}