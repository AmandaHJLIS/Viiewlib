#ifndef VIIEWLIB_MARC_H
#define VIIEWLIB_MARC_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MARC_Record MARC_Record;
typedef struct MARC_Field MARC_Field;
typedef struct MARC_Subfield MARC_Subfield;

/*
 * MARC record management.
 */
MARC_Record *marc_record_create(void);

void marc_record_free(
    MARC_Record *record
);

/*
 * Leader.
 *
 * A MARC21 leader is exactly 24 characters.
 */
const char *marc_record_get_leader(
    const MARC_Record *record
);

int marc_record_set_leader(
    MARC_Record *record,
    const char *leader
);

/*
 * Field management.
 *
 * Control fields and data fields are stored
 * together in the record's field list.
 */
int marc_record_add_field(
    MARC_Record *record,
    MARC_Field *field
);

size_t marc_record_get_field_count(
    const MARC_Record *record
);

MARC_Field *marc_record_get_field(
    const MARC_Record *record,
    size_t index
);

/*
 * Field lookup.
 *
 * Returns the first field with the requested tag.
 */
MARC_Field *marc_record_get_field_by_tag(
    const MARC_Record *record,
    const char *tag
);


/*
 * Convenience functions for control fields.
 *
 * MARC21 control fields are normally 001 through 009.
 */
int marc_record_set_control_field(
    MARC_Record *record,
    const char *tag,
    const char *value
);

const char *marc_record_get_control_field(
    const MARC_Record *record,
    const char *tag
);

/*
 * Field creation.
 *
 * For data fields, indicator1 and indicator2
 * should contain the MARC indicators.
 *
 * For control fields, both indicators should
 * normally be '\0'.
 */
MARC_Field *marc_field_create(
    const char *tag,
    char indicator1,
    char indicator2
);

void marc_field_free(
    MARC_Field *field
);

/*
 * Field information.
 */
const char *marc_field_get_tag(
    const MARC_Field *field
);

char marc_field_get_indicator1(
    const MARC_Field *field
);

char marc_field_get_indicator2(
    const MARC_Field *field
);

int marc_field_is_control_field(
    const MARC_Field *field
);

/*
 * Control-field value.
 *
 * Control fields contain one value and no subfields.
 */
int marc_field_set_control_value(
    MARC_Field *field,
    const char *value
);

const char *marc_field_get_control_value(
    const MARC_Field *field
);

/*
 * Subfield management.
 */
int marc_field_add_subfield(
    MARC_Field *field,
    char code,
    const char *value
);

size_t marc_field_get_subfield_count(
    const MARC_Field *field
);

MARC_Subfield *marc_field_get_subfield(
    const MARC_Field *field,
    size_t index
);

/*
 * Subfield information.
 */
MARC_Subfield *marc_subfield_create(
    char code,
    const char *value
);

void marc_subfield_free(
    MARC_Subfield *subfield
);

char marc_subfield_get_code(
    const MARC_Subfield *subfield
);

const char *marc_subfield_get_value(
    const MARC_Subfield *subfield
);

/*
 * MARC21 / ISO 2709 I/O.
 */
int marc_record_read(
    MARC_Record *record,
    FILE *stream
);

int marc_record_write(
    const MARC_Record *record,
    FILE *stream
);

#ifdef __cplusplus
}
#endif

#endif /* VIIEWLIB_MARC_H */