#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

struct MARC_Subfield
{
    char code;
    char *value;
};

MARC_Subfield *marc_subfield_create(
    char code,
    const char *value
)
{
    MARC_Subfield *subfield;

    if (value == NULL ||
        code == '\0')
    {
        return NULL;
    }

    subfield = calloc(
        1,
        sizeof(MARC_Subfield)
    );

    if (subfield == NULL)
    {
        return NULL;
    }

    subfield->value = malloc(
        strlen(value) + 1
    );

    if (subfield->value == NULL)
    {
        free(subfield);
        return NULL;
    }

    strcpy(
        subfield->value,
        value
    );

    subfield->code = code;

    return subfield;
}

void marc_subfield_free(
    MARC_Subfield *subfield
)
{
    if (subfield == NULL)
    {
        return;
    }

    free(
        subfield->value
    );

    free(
        subfield
    );
}

char marc_subfield_get_code(
    const MARC_Subfield *subfield
)
{
    if (subfield == NULL)
    {
        return '\0';
    }

    return subfield->code;
}

const char *marc_subfield_get_value(
    const MARC_Subfield *subfield
)
{
    if (subfield == NULL)
    {
        return NULL;
    }

    return subfield->value;
}