#include <stdio.h>

#include <viiewlib/marc.h>

int main(void)
{
    MARC_Record *record = NULL;
    MARC_Field *title = NULL;
    MARC_Subfield *subfield = NULL;

    printf("ViiewLib basic API example\n");
    printf("==========================\n\n");

    /* Create a MARC record. */
    record = marc_record_create();

    if (record == NULL) {
        printf("ERROR: Failed to create MARC record.\n");
        return 1;
    }

    printf("PASS: MARC record created.\n");

    /* Create a 245 title field. */
    title = marc_field_create("245", '1', '0');

    if (title == NULL) {
        printf("ERROR: Failed to create 245 field.\n");
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 245 field created.\n");

    /* Add title subfield $a. */
    if (marc_field_add_subfield(
            title,
            'a',
            "ViiewLib API Example") != 0) {

        printf("ERROR: Failed to add 245 $a.\n");
        marc_field_free(title);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 245 $a added.\n");

    /*
     * Add the field to the record.
     *
     * marc_record_add_field() takes ownership of the field,
     * so the field must not be freed separately after this succeeds.
     */
    if (marc_record_add_field(record, title) != 0) {
        printf("ERROR: Failed to add 245 field to record.\n");
        marc_field_free(title);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 245 field added to record.\n");

    /* Retrieve the first subfield through the public API. */
    subfield = marc_field_get_subfield(title, 0);

    if (subfield == NULL) {
        printf("ERROR: Failed to retrieve 245 $a.\n");
        marc_record_free(record);
        return 1;
    }

    printf("PASS: Retrieved subfield.\n");
    printf("      Code:  $%c\n",
           marc_subfield_get_code(subfield));

    printf("      Value: %s\n",
           marc_subfield_get_value(subfield));

    printf("\nAPI consumer example completed successfully!\n");

    /* The record owns the field and its subfield. */
    marc_record_free(record);

    return 0;
}