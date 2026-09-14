#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

struct MARC_Field
{
char tag[4];

char indicator1;
char indicator2;

char *control_value;

MARC_Subfield **subfields;
size_t subfield_count;

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

MARC_Field *marc_field_create(
const char *tag,
char indicator1,
char indicator2
)
{
MARC_Field *field;

if (tag == NULL)
{
    return NULL;
}

if (strlen(tag) != 3)
{
    return NULL;
}

field = calloc(
    1,
    sizeof(MARC_Field)
);

if (field == NULL)
{
    return NULL;
}

memcpy(
    field->tag,
    tag,
    3
);

field->tag[3] = '\0';

field->indicator1 = indicator1;
field->indicator2 = indicator2;

return field;

}

void marc_field_free(
MARC_Field *field
)
{
size_t i;

if (field == NULL)
{
    return;
}

free(
    field->control_value
);

if (field->subfields != NULL)
{
    for (i = 0; i < field->subfield_count; i++)
    {
        marc_subfield_free(
            field->subfields[i]
        );
    }

    free(
        field->subfields
    );
}

free(
    field
);

}

const char *marc_field_get_tag(
const MARC_Field *field
)
{
if (field == NULL)
{
return NULL;
}

return field->tag;

}

char marc_field_get_indicator1(
const MARC_Field *field
)
{
if (field == NULL)
{
return '\0';
}

return field->indicator1;

}

char marc_field_get_indicator2(
const MARC_Field *field
)
{
if (field == NULL)
{
return '\0';
}

return field->indicator2;

}

int marc_field_is_control_field(
const MARC_Field *field
)
{
if (field == NULL)
{
return 0;
}

return is_control_tag(
    field->tag
);

}

int marc_field_set_control_value(
MARC_Field *field,
const char *value
)
{
char *new_value;

if (field == NULL ||
    value == NULL)
{
    return -1;
}

if (!is_control_tag(
    field->tag
))
{
    return -1;
}

new_value = malloc(
    strlen(value) + 1
);

if (new_value == NULL)
{
    return -1;
}

strcpy(
    new_value,
    value
);

free(
    field->control_value
);

field->control_value = new_value;

return 0;

}

const char *marc_field_get_control_value(
const MARC_Field *field
)
{
if (field == NULL)
{
return NULL;
}

if (!is_control_tag(
    field->tag
))
{
    return NULL;
}

return field->control_value;

}

int marc_field_add_subfield(
MARC_Field *field,
char code,
const char *value
)
{
MARC_Subfield *subfield;
MARC_Subfield **new_subfields;

if (field == NULL ||
    value == NULL ||
    code == '\0')
{
    return -1;
}

if (is_control_tag(
    field->tag
))
{
    return -1;
}

subfield = marc_subfield_create(
    code,
    value
);

if (subfield == NULL)
{
    return -1;
}

new_subfields = realloc(
    field->subfields,
    sizeof(MARC_Subfield *) *
    (field->subfield_count + 1)
);

if (new_subfields == NULL)
{
    marc_subfield_free(
        subfield
    );

    return -1;
}

field->subfields = new_subfields;

field->subfields[
    field->subfield_count
] = subfield;

field->subfield_count++;

return 0;

}

size_t marc_field_get_subfield_count(
const MARC_Field *field
)
{
if (field == NULL)
{
return 0;
}

return field->subfield_count;

}

MARC_Subfield *marc_field_get_subfield(
    const MARC_Field *field,
    size_t index
)
{
    if (field == NULL)
    {
        return NULL;
    }

    if (index >= field->subfield_count)
    {
        return NULL;
    }

    return field->subfields[index];
}