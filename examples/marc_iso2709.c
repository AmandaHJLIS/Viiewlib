#include <stdio.h>

#include <viiewlib/marc.h>

int main(void)
{
    MARC_Record *record = NULL;
    MARC_Record *loaded = NULL;
    MARC_Field *title = NULL;
    MARC_Subfield *subfield = NULL;
    FILE *file = NULL;

    printf("ViiewLib MARC / ISO 2709 API example\n");
    printf("====================================\n\n");

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
            "ViiewLib ISO 2709 Example") != 0) {

        printf("ERROR: Failed to add 245 $a.\n");
        marc_field_free(title);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 245 $a added.\n");

    /*
     * Add the field to the record.
     * The record takes ownership of the field.
     */
    if (marc_record_add_field(record, title) != 0) {
        printf("ERROR: Failed to add 245 field to record.\n");
        marc_field_free(title);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 245 field added to record.\n");

    /* Write the record as ISO 2709. */
    file = fopen("examples/api_example.mrc", "wb");

    if (file == NULL) {
        printf("ERROR: Failed to open output file.\n");
        marc_record_free(record);
        return 1;
    }

    if (marc_record_write(record, file) != 0) {
        printf("ERROR: Failed to write ISO 2709 record.\n");
        fclose(file);
        marc_record_free(record);
        return 1;
    }

    fclose(file);

    printf("PASS: MARC record written as ISO 2709.\n");

    /* Open the ISO 2709 record again. */
    file = fopen("examples/api_example.mrc", "rb");

    if (file == NULL) {
        printf("ERROR: Failed to reopen ISO 2709 file.\n");
        marc_record_free(record);
        return 1;
    }

    /* Create a destination record for the decoded data. */
    loaded = marc_record_create();

    if (loaded == NULL) {
        printf("ERROR: Failed to create destination record.\n");
        fclose(file);
        marc_record_free(record);
        return 1;
    }

    /* Decode the ISO 2709 record. */
    if (marc_record_read(loaded, file) != 0) {
        printf("ERROR: Failed to read ISO 2709 record.\n");
        fclose(file);
        marc_record_free(loaded);
        marc_record_free(record);
        return 1;
    }

    fclose(file);

    printf("PASS: ISO 2709 record decoded successfully.\n");

    /*
     * Retrieve the first field from the decoded record.
     *
     * marc_record_get_field() uses a zero-based field index.
     * This record contains one field, so index 0 is the 245.
     */
    title = marc_record_get_field(loaded, 0);

    if (title == NULL) {
        printf("ERROR: Failed to retrieve decoded 245 field.\n");
        marc_record_free(loaded);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: Decoded 245 field retrieved.\n");

    /* Retrieve the title subfield. */
    subfield = marc_field_get_subfield(title, 0);

    if (subfield == NULL) {
        printf("ERROR: Failed to retrieve decoded 245 $a.\n");
        marc_record_free(loaded);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: Decoded subfield retrieved.\n");
    printf("      Code:  $%c\n",
           marc_subfield_get_code(subfield));

    printf("      Value: %s\n",
           marc_subfield_get_value(subfield));

    printf("\nMARC / ISO 2709 API example completed successfully!\n");

    marc_record_free(loaded);
    marc_record_free(record);

    return 0;
}