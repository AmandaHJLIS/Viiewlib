#include <stdio.h>
#include <string.h>

#include "viiewlib/marc.h"

int main(void)
{
    MARC_Record *record;
    MARC_Record *loaded;
    MARC_Field *field;
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
        printf("ERROR: test.mrc is empty!\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    /*
     * Return to beginning before closing.
     */
    if (fseek(
        file,
        0,
        SEEK_SET
    ) != 0)
    {
        printf("ERROR: Could not rewind test.mrc.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    fclose(file);

    printf("test.mrc written successfully.\n");

    /*
     * Free original record.
     */
    marc_record_free(record);

    printf("Original record freed.\n");

    /*
     * Create record for reading.
     */
    loaded = marc_record_create();

    if (loaded == NULL)
    {
        printf("ERROR: Could not create loaded record.\n");
        return 1;
    }

    printf("Empty record created for reading.\n");

    /*
     * Open .mrc file for reading.
     */
    file = fopen(
        "test.mrc",
        "rb"
    );

    if (file == NULL)
    {
        printf("ERROR: Could not open test.mrc for reading.\n");
        marc_record_free(loaded);
        return 1;
    }

    printf("test.mrc opened for reading.\n");

    /*
     * Read ISO 2709 record.
     */
    if (marc_record_read(
        loaded,
        file
    ) != 0)
    {
        printf("ERROR: marc_record_read() failed.\n");
        printf("The ISO 2709 reader rejected the record.\n");

        fclose(file);
        marc_record_free(loaded);
        return 1;
    }

    fclose(file);

    printf("test.mrc read successfully.\n");

    /*
     * Verify loaded field count.
     */
    if (marc_record_get_field_count(loaded) != 2)
    {
        printf(
            "ERROR: Expected 2 loaded fields, got %zu.\n",
            marc_record_get_field_count(loaded)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf("Loaded record contains 2 fields.\n");

    /*
     * Verify 001.
     */
    if (marc_record_get_control_field(
        loaded,
        "001"
    ) == NULL)
    {
        printf("ERROR: Loaded 001 is missing.\n");
        marc_record_free(loaded);
        return 1;
    }

    if (strcmp(
        marc_record_get_control_field(
            loaded,
            "001"
        ),
        "123456"
    ) != 0)
    {
        printf(
            "ERROR: Loaded 001 value is incorrect: %s\n",
            marc_record_get_control_field(
                loaded,
                "001"
            )
        );

        marc_record_free(loaded);
        return 1;
    }

    printf("001 round-trip verified: 123456\n");

    /*
     * Get loaded 245.
     */
    field = marc_record_get_field(
        loaded,
        1
    );

    if (field == NULL)
    {
        printf("ERROR: Could not retrieve loaded 245.\n");
        marc_record_free(loaded);
        return 1;
    }

    /*
     * Verify tag.
     */
    if (strcmp(
        marc_field_get_tag(field),
        "245"
    ) != 0)
    {
        printf(
            "ERROR: Expected loaded field 245, got %s.\n",
            marc_field_get_tag(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    /*
     * Verify indicators.
     */
    if (marc_field_get_indicator1(field) != '1' ||
        marc_field_get_indicator2(field) != '0')
    {
        printf(
            "ERROR: 245 indicators changed: %c%c\n",
            marc_field_get_indicator1(field),
            marc_field_get_indicator2(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf("245 indicators verified: 10\n");

    /*
     * Verify subfield count.
     */
    if (marc_field_get_subfield_count(field) != 1)
    {
        printf(
            "ERROR: Expected 1 subfield, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    /*
     * Get $a.
     */
    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL)
    {
        printf("ERROR: Could not retrieve 245 $a.\n");
        marc_record_free(loaded);
        return 1;
    }

    /*
     * Verify subfield code.
     */
    if (marc_subfield_get_code(subfield) != 'a')
    {
        printf(
            "ERROR: Expected subfield code a, got %c.\n",
            marc_subfield_get_code(subfield)
        );

        marc_record_free(loaded);
        return 1;
    }

    /*
     * Verify subfield value.
     */
    if (strcmp(
        marc_subfield_get_value(subfield),
        "The Example Book"
    ) != 0)
    {
        printf(
            "ERROR: Unexpected 245 $a value: %s\n",
            marc_subfield_get_value(subfield)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "245 $a round-trip verified: %s\n",
        marc_subfield_get_value(subfield)
    );

    /*
     * Finished.
     */
    marc_record_free(loaded);

    printf("Loaded record freed successfully.\n");
    printf("All ISO 2709 tests passed!\n");

    return 0;
}