#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

struct MARC_Record
{
    char *leader;

    MARC_Field **fields;
    size_t field_count;
};

static int is_control_tag(
    const char *tag
)
{
    if (tag == NULL)
    {
        return 0;
    }

    if (strlen(tag) != 3)
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

MARC_Record *marc_record_create(void)
{
    MARC_Record *record;

    record = calloc(
        1,
        sizeof(MARC_Record)
    );

    if (record == NULL)
    {
        return NULL;
    }

    record->leader = calloc(
        25,
        sizeof(char)
    );

    if (record->leader == NULL)
    {
        free(record);
        return NULL;
    }

    /*
     * A MARC21 leader is exactly 24 characters.
     *
     * Until ISO 2709 encoding is performed,
     * initialise it with spaces.
     */
    memset(
        record->leader,
        ' ',
        24
    );

    record->leader[24] = '\0';

    return record;
}

void marc_record_free(
    MARC_Record *record
)
{
    size_t i;

    if (record == NULL)
    {
        return;
    }

    if (record->fields != NULL)
    {
        for (i = 0; i < record->field_count; i++)
        {
            marc_field_free(
                record->fields[i]
            );
        }

        free(
            record->fields
        );
    }

    free(
        record->leader
    );

    free(
        record
    );
}

const char *marc_record_get_leader(
    const MARC_Record *record
)
{
    if (record == NULL)
    {
        return NULL;
    }

    return record->leader;
}

int marc_record_set_leader(
    MARC_Record *record,
    const char *leader
)
{
    if (record == NULL ||
        leader == NULL)
    {
        return -1;
    }

    if (strlen(leader) != 24)
    {
        return -1;
    }

    memcpy(
        record->leader,
        leader,
        24
    );

    record->leader[24] = '\0';

    return 0;
}

int marc_record_add_field(
    MARC_Record *record,
    MARC_Field *field
)
{
    MARC_Field **new_fields;

    if (record == NULL ||
        field == NULL)
    {
        return -1;
    }

    new_fields = realloc(
        record->fields,
        sizeof(MARC_Field *) *
        (record->field_count + 1)
    );

    if (new_fields == NULL)
    {
        return -1;
    }

    record->fields = new_fields;

    record->fields[
        record->field_count
    ] = field;

    record->field_count++;

    return 0;
}

size_t marc_record_get_field_count(
    const MARC_Record *record
)
{
    if (record == NULL)
    {
        return 0;
    }

    return record->field_count;
}

MARC_Field *marc_record_get_field(
    const MARC_Record *record,
    size_t index
)
{
    if (record == NULL)
    {
        return NULL;
    }

    if (index >= record->field_count)
    {
        return NULL;
    }

    return record->fields[index];
}

int marc_record_set_control_field(
    MARC_Record *record,
    const char *tag,
    const char *value
)
{
    MARC_Field *field;

    if (record == NULL ||
        tag == NULL ||
        value == NULL)
    {
        return -1;
    }

    if (!is_control_tag(tag))
    {
        return -1;
    }

    /*
     * If the control field already exists,
     * replace its value.
     */
    for (size_t i = 0; i < record->field_count; i++)
    {
        field = record->fields[i];

        if (strcmp(
            marc_field_get_tag(field),
            tag
        ) == 0)
        {
            return marc_field_set_control_value(
                field,
                value
            );
        }
    }

    /*
     * Otherwise create a new control field.
     */
    field = marc_field_create(
        tag,
        '\0',
        '\0'
    );

    if (field == NULL)
    {
        return -1;
    }

    if (marc_field_set_control_value(
        field,
        value
    ) != 0)
    {
        marc_field_free(field);
        return -1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        marc_field_free(field);
        return -1;
    }

    return 0;
}

const char *marc_record_get_control_field(
    const MARC_Record *record,
    const char *tag
)
{
    MARC_Field *field;

    if (record == NULL ||
        tag == NULL)
    {
        return NULL;
    }

    if (!is_control_tag(tag))
    {
        return NULL;
    }

    for (size_t i = 0; i < record->field_count; i++)
    {
        field = record->fields[i];

        if (strcmp(
            marc_field_get_tag(field),
            tag
        ) == 0)
        {
            return marc_field_get_control_value(
                field
            );
        }
    }

    return NULL;
}