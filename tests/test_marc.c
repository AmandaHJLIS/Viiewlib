#include <stdio.h>
#include <string.h>

#include "viiewlib/marc.h"

int main(void)
{
    MARC_Record *record;
    MARC_Record *loaded;
    MARC_Field *field;
    MARC_Field *found_field;
    MARC_Subfield *subfield;
    FILE *file;
    long file_size;

    printf("ViiewLib ISO 2709 test\n");
    printf("======================\n");

    /*
     * Create original record.
     */
    record = marc_record_create();

    if (record == NULL)
    {
        printf("ERROR: Could not create record.\n");
        return 1;
    }

    printf("Record created successfully.\n");

    /*
     * Add control field 001.
     */
    if (marc_record_set_control_field(
        record,
        "001",
        "123456"
    ) != 0)
    {
        printf("ERROR: Could not create 001.\n");
        marc_record_free(record);
        return 1;
    }

    printf("001 control field added successfully.\n");

    /*
     * Create 245.
     */
    field = marc_field_create(
        "245",
        '1',
        '0'
    );

    if (field == NULL)
    {
        printf("ERROR: Could not create 245.\n");
        marc_record_free(record);
        return 1;
    }

    printf("245 field created successfully.\n");

    /*
     * Add 245 $a.
     */
    if (marc_field_add_subfield(
        field,
        'a',
        "The Example Book"
    ) != 0)
    {
        printf("ERROR: Could not create 245 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf("245 $a added successfully.\n");

    /*
     * Add 245 to record.
     */
    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add 245.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf("245 field added to record successfully.\n");

    /*
     * Verify original field count.
     */
    if (marc_record_get_field_count(record) != 2)
    {
        printf(
            "ERROR: Expected 2 fields, got %zu.\n",
            marc_record_get_field_count(record)
        );

        marc_record_free(record);
        return 1;
    }

    printf("Original record contains 2 fields.\n");

    /*
     * Test field lookup by tag on the original record.
     */
    found_field = marc_record_get_field_by_tag(
        record,
        "245"
    );

    if (found_field == NULL)
    {
        printf("ERROR: Could not find 245 using field lookup.\n");
        marc_record_free(record);
        return 1;
    }

    if (strcmp(
        marc_field_get_tag(found_field),
        "245"
    ) != 0)
    {
        printf(
            "ERROR: Field lookup returned unexpected tag: %s\n",
            marc_field_get_tag(found_field)
        );

        marc_record_free(record);
        return 1;
    }

    printf("245 field lookup successful: 245\n");

    /*
     * Verify lookup of 001.
     */
    found_field = marc_record_get_field_by_tag(
        record,
        "001"
    );

    if (found_field == NULL)
    {
        printf("ERROR: Could not find 001 using field lookup.\n");
        marc_record_free(record);
        return 1;
    }

    if (strcmp(
        marc_field_get_tag(found_field),
        "001"
    ) != 0)
    {
        printf(
            "ERROR: 001 lookup returned unexpected tag: %s\n",
            marc_field_get_tag(found_field)
        );

        marc_record_free(record);
        return 1;
    }

    printf("001 field lookup successful: 001\n");

    /*
     * Verify lookup of a field that does not exist.
     */
    found_field = marc_record_get_field_by_tag(
        record,
        "999"
    );

    if (found_field != NULL)
    {
        printf(
            "ERROR: Lookup unexpectedly found nonexistent 999 field.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf("Nonexistent field lookup correctly returned NULL.\n");

    /*
     * Open .mrc file for writing.
     */
    file = fopen(
        "test.mrc",
        "wb"
    );

    if (file == NULL)
    {
        printf("ERROR: Could not open test.mrc for writing.\n");
        marc_record_free(record);
        return 1;
    }

    printf("test.mrc opened for writing.\n");

    /*
     * Write ISO 2709 record.
     */
    if (marc_record_write(
        record,
        file
    ) != 0)
    {
        printf("ERROR: marc_record_write() failed.\n");
        printf("The ISO 2709 writer returned an error.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    /*
     * Make sure all buffered data reached the file.
     */
    if (fflush(file) != 0)
    {
        printf("ERROR: Could not flush test.mrc.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    /*
     * Check file size before closing.
     */
    if (fseek(
        file,
        0,
        SEEK_END
    ) != 0)
    {
        printf("ERROR: Could not seek to end of test.mrc.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    file_size = ftell(file);

    if (file_size < 0)
    {
        printf("ERROR: Could not determine test.mrc size.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    printf(
        "test.mrc size: %ld bytes\n",
        file_size
    );

    if (file_size == 0)
    {
        printf("ERROR: test.mrc is empty!\n"