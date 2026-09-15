#ifndef VIIEWLIB_MARC_H
#define VIIEWLIB_MARC_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opaque MARC data structures.
 *
 * The internal representation of records, fields, and subfields is
 * intentionally hidden from API consumers.
 */
typedef struct MARC_Record MARC_Record;
typedef struct MARC_Field MARC_Field;
typedef struct MARC_Subfield MARC_Subfield;


/*
 * --------------------------------------------------------------------------
 * MARC record management
 * --------------------------------------------------------------------------
 */

/*
 * Creates an empty MARC record.
 *
 * Returns:
 *     A newly allocated MARC_Record on success.
 *     NULL on allocation failure.
 *
 * The caller owns the returned record and must release it with
 * marc_record_free().
 */
MARC_Record *marc_record_create(void);

/*
 * Frees a MARC record and all fields and subfields owned by it.
 *
 * Passing NULL is permitted and has no effect.
 *
 * Any MARC_Field successfully added to the record with
 * marc_record_add_field() is owned by the record and is freed here.
 */
void marc_record_free(
    MARC_Record *record
);


/*
 * --------------------------------------------------------------------------
 * MARC leader
 * --------------------------------------------------------------------------
 */

/*
 * Returns the 24-character MARC21 leader associated with the record.
 *
 * The returned pointer is owned by the record and must not be modified
 * or freed by the caller.
 *
 * The returned string is NUL-terminated for C API use.
 */
const char *marc_record_get_leader(
    const MARC_Record *record
);

/*
 * Sets the MARC21 leader.
 *
 * The leader must contain exactly 24 characters, excluding the terminating
 * NUL character.
 *
 * The library copies the supplied leader; the caller retains ownership
 * of the input string.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value on failure.
 */
int marc_record_set_leader(
    MARC_Record *record,
    const char *leader
);


/*
 * --------------------------------------------------------------------------
 * MARC field management
 * --------------------------------------------------------------------------
 *
 * Control fields and data fields are stored together in the record's
 * field list.
 */

/*
 * Adds a field to a MARC record.
 *
 * On success, ownership of 'field' is transferred to 'record'.
 * The caller must not free the field separately after successful addition.
 *
 * If this function fails, ownership remains with the caller.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value on failure.
 */
int marc_record_add_field(
    MARC_Record *record,
    MARC_Field *field
);

/*
 * Returns the number of fields currently contained in the record.
 *
 * Returns 0 for an empty record.
 */
size_t marc_record_get_field_count(
    const MARC_Record *record
);

/*
 * Returns the field at 'index'.
 *
 * Fields are indexed from zero in their stored order.
 *
 * The returned field remains owned by the record and must not be freed
 * separately by the caller.
 *
 * Returns NULL if 'record' is NULL or 'index' is out of range.
 */
MARC_Field *marc_record_get_field(
    const MARC_Record *record,
    size_t index
);

/*
 * Returns the first field with the requested tag.
 *
 * For repeated fields, only the first matching field is returned.
 *
 * The returned field remains owned by the record and must not be freed
 * separately by the caller.
 *
 * Returns NULL if no matching field is found or if an argument is invalid.
 */
MARC_Field *marc_record_get_field_by_tag(
    const MARC_Record *record,
    const char *tag
);


/*
 * --------------------------------------------------------------------------
 * MARC control-field convenience functions
 * --------------------------------------------------------------------------
 *
 * MARC21 control fields are normally identified by tags 001 through 009.
 * Control fields contain a value and do not contain indicators or
 * subfields.
 */

/*
 * Sets the value of a control field.
 *
 * If the specified control field already exists, its value is replaced.
 * If it does not exist, the library creates it.
 *
 * The supplied value is copied by the library.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value on failure.
 */
int marc_record_set_control_field(
    MARC_Record *record,
    const char *tag,
    const char *value
);

/*
 * Returns the value of the requested control field.
 *
 * The returned pointer is owned by the record and must not be modified
 * or freed by the caller.
 *
 * Returns NULL if the control field does not exist or if an argument
 * is invalid.
 */
const char *marc_record_get_control_field(
    const MARC_Record *record,
    const char *tag
);


/*
 * --------------------------------------------------------------------------
 * MARC field creation
 * --------------------------------------------------------------------------
 */

/*
 * Creates a MARC field.
 *
 * For data fields, indicator1 and indicator2 contain the MARC indicators.
 *
 * For control fields, both indicators should normally be '\0'. Control
 * fields do not encode indicators in ISO 2709.
 *
 * The supplied tag is copied by the library.
 *
 * Returns:
 *     A newly allocated MARC_Field on success.
 *     NULL on allocation failure or invalid arguments.
 *
 * The caller owns the returned field until it is successfully added to
 * a MARC record with marc_record_add_field().
 */
MARC_Field *marc_field_create(
    const char *tag,
    char indicator1,
    char indicator2
);

/*
 * Frees a standalone MARC field and all subfields owned by it.
 *
 * Do not call this on a field after ownership has successfully been
 * transferred to a MARC_Record with marc_record_add_field().
 */
void marc_field_free(
    MARC_Field *field
);


/*
 * --------------------------------------------------------------------------
 * MARC field information
 * --------------------------------------------------------------------------
 */

/*
 * Returns the three-character MARC tag.
 *
 * The returned pointer is owned by the field and must not be modified
 * or freed by the caller.
 */
const char *marc_field_get_tag(
    const MARC_Field *field
);

/*
 * Returns the first indicator of a data field.
 *
 * For control fields, the result is normally '\0'.
 */
char marc_field_get_indicator1(
    const MARC_Field *field
);

/*
 * Returns the second indicator of a data field.
 *
 * For control fields, the result is normally '\0'.
 */
char marc_field_get_indicator2(
    const MARC_Field *field
);

/*
 * Returns non-zero if the field is a MARC21 control field.
 *
 * Returns zero for a data field or an invalid/NULL field.
 */
int marc_field_is_control_field(
    const MARC_Field *field
);


/*
 * --------------------------------------------------------------------------
 * MARC control-field values
 * --------------------------------------------------------------------------
 */

/*
 * Sets the value of a control field.
 *
 * The supplied value is copied by the library.
 *
 * This function is intended for control fields. Data fields should
 * use marc_field_add_subfield() instead.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value on failure.
 */
int marc_field_set_control_value(
    MARC_Field *field,
    const char *value
);

/*
 * Returns the value stored in a control field.
 *
 * The returned pointer is owned by the field and must not be modified
 * or freed by the caller.
 *
 * Returns NULL if the field is not a control field or if the argument
 * is invalid.
 */
const char *marc_field_get_control_value(
    const MARC_Field *field
);


/*
 * --------------------------------------------------------------------------
 * MARC subfield management
 * --------------------------------------------------------------------------
 */

/*
 * Adds a subfield to a data field.
 *
 * The subfield is appended after all existing subfields, preserving
 * insertion order.
 *
 * Repeated subfield codes are permitted and are preserved independently.
 *
 * The supplied value is copied by the library.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value on failure.
 */
int marc_field_add_subfield(
    MARC_Field *field,
    char code,
    const char *value
);

/*
 * Returns the number of subfields contained in the field.
 *
 * Returns 0 for a field containing no subfields.
 */
size_t marc_field_get_subfield_count(
    const MARC_Field *field
);

/*
 * Returns the subfield at 'index'.
 *
 * Subfields are indexed from zero in their stored order.
 *
 * The returned subfield remains owned by its parent field and must not
 * be freed separately by the caller.
 *
 * Returns NULL if the field is NULL or 'index' is out of range.
 */
MARC_Subfield *marc_field_get_subfield(
    const MARC_Field *field,
    size_t index
);


/*
 * --------------------------------------------------------------------------
 * MARC subfield information
 * --------------------------------------------------------------------------
 */

/*
 * Creates a standalone MARC subfield.
 *
 * The supplied value is copied by the library.
 *
 * The caller owns the returned subfield and must release it with
 * marc_subfield_free().
 *
 * Subfields normally become part of a field through
 * marc_field_add_subfield(), which creates and owns the subfield
 * internally.
 *
 * Returns:
 *     A newly allocated MARC_Subfield on success.
 *     NULL on allocation failure or invalid arguments.
 */
MARC_Subfield *marc_subfield_create(
    char code,
    const char *value
);

/*
 * Frees a standalone MARC subfield.
 *
 * A subfield owned by a MARC_Field must not be freed separately.
 */
void marc_subfield_free(
    MARC_Subfield *subfield
);

/*
 * Returns the MARC subfield code.
 */
char marc_subfield_get_code(
    const MARC_Subfield *subfield
);

/*
 * Returns the subfield value.
 *
 * The returned pointer is owned by the subfield and must not be modified
 * or freed by the caller.
 */
const char *marc_subfield_get_value(
    const MARC_Subfield *subfield
);


/*
 * --------------------------------------------------------------------------
 * MARC21 / ISO 2709 I/O
 * --------------------------------------------------------------------------
 */

/*
 * Reads one MARC21 record encoded using ISO 2709 from 'stream'.
 *
 * The stream must be positioned at the beginning of a complete ISO 2709
 * record.
 *
 * Decoded fields are appended to the supplied record. The record is not
 * automatically cleared before reading.
 *
 * If an error occurs after one or more fields have been decoded, those
 * fields may remain in the record. The caller is responsible for deciding
 * whether to discard the partially populated record.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value if the record is malformed, incomplete, or an
 *     I/O or allocation error occurs.
 */
int marc_record_read(
    MARC_Record *record,
    FILE *stream
);

/*
 * Writes one MARC21 record to 'stream' using ISO 2709 encoding.
 *
 * The record is not modified by this operation.
 *
 * Returns:
 *     0 on success.
 *     A non-zero value if the record is invalid, cannot be encoded,
 *     or an I/O error occurs.
 */
int marc_record_write(
    const MARC_Record *record,
    FILE *stream
);


#ifdef __cplusplus
}
#endif

#endif /* VIIEWLIB_MARC_H */