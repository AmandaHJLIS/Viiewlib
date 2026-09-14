#include <stdio.h>
#include <string.h>

#include "viiewlib/marc.h"

static int check_string(
    const char *label,
    const char *actual,
    const char *expected
)
{
    if (actual == NULL)
    {
        printf(
            "ERROR: %s is NULL.\n",
            label
        );

        return 0;
    }

    if (strcmp(actual, expected) != 0)
    {
        printf(
            "ERROR: %s is incorrect.\n"
            "       Expected: %s\n"
            "       Got:      %s\n",
            label,
            expected,
            actual
        );

        return 0;
    }

    printf(
        "PASS: %s\n",
        label
    );

    return 1;
}

int main(void)
{
    MARC_Record *record;
    MARC_Record *loaded;
    MARC_Field *field;
    MARC_Field *found_field;
    MARC_Subfield *subfield;
    FILE *file;
    long file_size;

    printf("ViiewLib MARC21 test suite\n");
    printf("==========================\n\n");

    /*
     * ============================================================
     * 1. Record creation
     * ============================================================
     */

    printf("[1] Record creation\n");

    record = marc_record_create();

    if (record == NULL)
    {
        printf("ERROR: Could not create record.\n");
        return 1;
    }

    printf("PASS: Record created successfully.\n\n");

    /*
     * ============================================================
     * 2. Control fields
     * ============================================================
     */

    printf("[2] Control fields\n");

    /*
     * 001
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

    if (!check_string(
        "001",
        marc_record_get_control_field(
            record,
            "001"
        ),
        "123456"
    ))
    {
        marc_record_free(record);
        return 1;
    }

    /*
     * 005
     */
    if (marc_record_set_control_field(
        record,
        "005",
        "20260914165000.0"
    ) != 0)
    {
        printf("ERROR: Could not create 005.\n");
        marc_record_free(record);
        return 1;
    }

    if (!check_string(
        "005",
        marc_record_get_control_field(
            record,
            "005"
        ),
        "20260914165000.0"
    ))
    {
        marc_record_free(record);
        return 1;
    }

    /*
     * 008
     *
     * Exactly 40 characters.
     */
    if (marc_record_set_control_field(
        record,
        "008",
        "260914s2026    xx            000 0 eng d"
    ) != 0)
    {
        printf("ERROR: Could not create 008.\n");
        marc_record_free(record);
        return 1;
    }

    if (!check_string(
        "008",
        marc_record_get_control_field(
            record,
            "008"
        ),
        "260914s2026    xx            000 0 eng d"
    ))
    {
        marc_record_free(record);
        return 1;
    }

    printf("Control field tests passed.\n\n");

    /*
     * ============================================================
     * 3. 100 - Main entry
     * ============================================================
     */

    printf("[3] 100 - Main entry\n");

    field = marc_field_create(
        "100",
        '1',
        ' '
    );

    if (field == NULL)
    {
        printf("ERROR: Could not create 100.\n");
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'a',
        "Smith, Amanda"
    ) != 0)
    {
        printf("ERROR: Could not add 100 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'd',
        "2005-"
    ) != 0)
    {
        printf("ERROR: Could not add 100 $d.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add 100 to record.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 100 created with $a and $d.\n\n");

    /*
     * ============================================================
     * 4. 245 - Title statement
     * ============================================================
     */

    printf("[4] 245 - Title statement\n");

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

    if (marc_field_add_subfield(
        field,
        'a',
        "The Example Book"
    ) != 0)
    {
        printf("ERROR: Could not add 245 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'b',
        "a test of ViiewLib"
    ) != 0)
    {
        printf("ERROR: Could not add 245 $b.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'c',
        "Amanda"
    ) != 0)
    {
        printf("ERROR: Could not add 245 $c.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add 245 to record.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_field_get_subfield_count(field) != 3)
    {
        printf(
            "ERROR: Expected 3 subfields in 245, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(record);
        return 1;
    }

    printf("PASS: 245 created with $a, $b and $c.\n\n");

    /*
     * ============================================================
     * 5. 264 - Publication information
     * ============================================================
     */

    printf("[5] 264 - Publication information\n");

    field = marc_field_create(
        "264",
        ' ',
        '1'
    );

    if (field == NULL)
    {
        printf("ERROR: Could not create 264.\n");
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'a',
        "Perth, WA :"
    ) != 0)
    {
        printf("ERROR: Could not add 264 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'b',
        "Viiew Press"
    ) != 0)
    {
        printf("ERROR: Could not add 264 $b.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'c',
        "2026"
    ) != 0)
    {
        printf("ERROR: Could not add 264 $c.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add 264 to record.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: 264 created with $a, $b and $c.\n\n");

    /*
     * ============================================================
     * 6. Repeated fields
     * ============================================================
     */

    printf("[6] Repeated fields\n");

    /*
     * First 650.
     */
    field = marc_field_create(
        "650",
        ' ',
        '0'
    );

    if (field == NULL)
    {
        printf("ERROR: Could not create first 650.\n");
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'a',
        "Libraries"
    ) != 0)
    {
        printf("ERROR: Could not add first 650 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add first 650.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    /*
     * Second 650.
     */
    field = marc_field_create(
        "650",
        ' ',
        '0'
    );

    if (field == NULL)
    {
        printf("ERROR: Could not create second 650.\n");
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'a',
        "Metadata"
    ) != 0)
    {
        printf("ERROR: Could not add second 650 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add second 650.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: Two repeated 650 fields created.\n");

    /*
     * Field lookup should return the first 650.
     */
    found_field = marc_record_get_field_by_tag(
        record,
        "650"
    );

    if (found_field == NULL)
    {
        printf("ERROR: Could not find repeated 650 field.\n");
        marc_record_free(record);
        return 1;
    }

    subfield = marc_field_get_subfield(
        found_field,
        0
    );

    if (subfield == NULL)
    {
        printf("ERROR: First 650 has no subfield.\n");
        marc_record_free(record);
        return 1;
    }

    if (!check_string(
        "First repeated 650 $a",
        marc_subfield_get_value(subfield),
        "Libraries"
    ))
    {
        marc_record_free(record);
        return 1;
    }

    printf("PASS: Field lookup returns first repeated 650.\n\n");

    /*
     * ============================================================
     * 7. Special characters
     * ============================================================
     */

    printf("[7] Special characters\n");

    field = marc_field_create(
        "500",
        ' ',
        ' '
    );

    if (field == NULL)
    {
        printf("ERROR: Could not create 500.\n");
        marc_record_free(record);
        return 1;
    }

    if (marc_field_add_subfield(
        field,
        'a',
        "O'Reilly & Associates: a test, with punctuation!"
    ) != 0)
    {
        printf("ERROR: Could not add 500 $a.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf("ERROR: Could not add 500.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf("PASS: Special-character field created.\n\n");

    /*
     * ============================================================
     * 8. Original record field count
     * ============================================================
     */

    printf("[8] Record structure\n");

    if (marc_record_get_field_count(record) != 9)
    {
        printf(
            "ERROR: Expected 9 fields, got %zu.\n",
            marc_record_get_field_count(record)
        );

        marc_record_free(record);
        return 1;
    }

    printf("PASS: Original record contains 9 fields.\n\n");

    /*
     * ============================================================
     * 9. ISO 2709 write
     * ============================================================
     */

    printf("[9] ISO 2709 write\n");

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

    if (marc_record_write(
        record,
        file
    ) != 0)
    {
        printf("ERROR: marc_record_write() failed.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    if (fflush(file) != 0)
    {
        printf("ERROR: Could not flush test.mrc.\n");

        fclose(file);
        marc_record_free(record);
        return 1;
    }

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

    fclose(file);

    printf("PASS: ISO 2709 record written successfully.\n\n");

    marc_record_free(record);

    printf("Original record freed.\n\n");

    /*
     * ============================================================
     * 10. ISO 2709 read
     * ============================================================
     */

    printf("[10] ISO 2709 read\n");

    loaded = marc_record_create();

    if (loaded == NULL)
    {
        printf("ERROR: Could not create loaded record.\n");
        return 1;
    }

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

    if (marc_record_read(
        loaded,
        file
    ) != 0)
    {
        printf("ERROR: marc_record_read() failed.\n");

        fclose(file);
        marc_record_free(loaded);
        return 1;
    }

    fclose(file);

    printf("PASS: ISO 2709 record read successfully.\n\n");

    /*
     * ============================================================
     * 11. Loaded record structure
     * ============================================================
     */

    printf("[11] Loaded record structure\n");

    if (marc_record_get_field_count(loaded) != 9)
    {
        printf(
            "ERROR: Expected 9 loaded fields, got %zu.\n",
            marc_record_get_field_count(loaded)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf("PASS: Loaded record contains 9 fields.\n\n");

    /*
     * ============================================================
     * 12. Control-field round-trip
     * ============================================================
     */

    printf("[12] Control-field round-trip\n");

    if (!check_string(
        "001",
        marc_record_get_control_field(
            loaded,
            "001"
        ),
        "123456"
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    if (!check_string(
        "005",
        marc_record_get_control_field(
            loaded,
            "005"
        ),
        "20260914165000.0"
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    if (!check_string(
        "008",
        marc_record_get_control_field(
            loaded,
            "008"
        ),
        "260914s2026    xx            000 0 eng d"
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf("Control-field round-trip tests passed.\n\n");

    /*
     * ============================================================
     * 13. 100 round-trip
     * ============================================================
     */

    printf("[13] 100 round-trip\n");

    field = marc_record_get_field_by_tag(
        loaded,
        "100"
    );

    if (field == NULL)
    {
        printf("ERROR: Loaded 100 is missing.\n");
        marc_record_free(loaded);
        return 1;
    }

    if (marc_field_get_indicator1(field) != '1' ||
        marc_field_get_indicator2(field) != ' ')
    {
        printf(
            "ERROR: 100 indicators changed: %c%c\n",
            marc_field_get_indicator1(field),
            marc_field_get_indicator2(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    if (marc_field_get_subfield_count(field) != 2)
    {
        printf(
            "ERROR: Expected 2 subfields in 100, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'a' ||
        !check_string(
            "100 $a",
            marc_subfield_get_value(subfield),
            "Smith, Amanda"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        1
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'd' ||
        !check_string(
            "100 $d",
            marc_subfield_get_value(subfield),
            "2005-"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf("100 round-trip passed.\n\n");

    /*
     * ============================================================
     * 14. 245 round-trip
     * ============================================================
     */

    printf("[14] 245 round-trip\n");

    field = marc_record_get_field_by_tag(
        loaded,
        "245"
    );

    if (field == NULL)
    {
        printf("ERROR: Loaded 245 is missing.\n");
        marc_record_free(loaded);
        return 1;
    }

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

    if (marc_field_get_subfield_count(field) != 3)
    {
        printf(
            "ERROR: Expected 3 subfields in 245, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'a' ||
        !check_string(
            "245 $a",
            marc_subfield_get_value(subfield),
            "The Example Book"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        1
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'b' ||
        !check_string(
            "245 $b",
            marc_subfield_get_value(subfield),
            "a test of ViiewLib"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        2
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'c' ||
        !check_string(
            "245 $c",
            marc_subfield_get_value(subfield),
            "Amanda"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf("245 round-trip passed.\n\n");

    /*
     * ============================================================
     * 15. 264 round-trip
     * ============================================================
     */

    printf("[15] 264 round-trip\n");

    field = marc_record_get_field_by_tag(
        loaded,
        "264"
    );

    if (field == NULL)
    {
        printf("ERROR: Loaded 264 is missing.\n");
        marc_record_free(loaded);
        return 1;
    }

    if (marc_field_get_indicator1(field) != ' ' ||
        marc_field_get_indicator2(field) != '1')
    {
        printf(
            "ERROR: 264 indicators changed: %c%c\n",
            marc_field_get_indicator1(field),
            marc_field_get_indicator2(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    if (marc_field_get_subfield_count(field) != 3)
    {
        printf(
            "ERROR: Expected 3 subfields in 264, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'a' ||
        !check_string(
            "264 $a",
            marc_subfield_get_value(subfield),
            "Perth, WA :"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        1
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'b' ||
        !check_string(
            "264 $b",
            marc_subfield_get_value(subfield),
            "Viiew Press"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        2
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'c' ||
        !check_string(
            "264 $c",
            marc_subfield_get_value(subfield),
            "2026"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf("264 round-trip passed.\n\n");

    /*
     * ============================================================
     * 16. Repeated field round-trip
     * ============================================================
     *
     * The ISO 2709 writer orders the loaded fields by tag.
     *
     * Loaded record order:
     *
     * 0 = 001
     * 1 = 005
     * 2 = 008
     * 3 = 100
     * 4 = 245
     * 5 = 264
     * 6 = 500
     * 7 = 650 Libraries
     * 8 = 650 Metadata
     */

    printf("[16] Repeated field round-trip\n");

    /*
     * First 650.
     */
    field = marc_record_get_field(
        loaded,
        7
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "650"
        ) != 0)
    {
        printf(
            "ERROR: First repeated 650 is missing.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'a' ||
        !check_string(
            "First 650 $a",
            marc_subfield_get_value(subfield),
            "Libraries"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: First 650 preserved as Libraries.\n"
    );

    /*
     * Second 650.
     */
    field = marc_record_get_field(
        loaded,
        8
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "650"
        ) != 0)
    {
        printf(
            "ERROR: Second repeated 650 is missing.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'a' ||
        !check_string(
            "Second 650 $a",
            marc_subfield_get_value(subfield),
            "Metadata"
        ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Second 650 preserved as Metadata.\n"
    );

    /*
     * Confirm that the repeated fields are distinct objects.
     */
    if (marc_record_get_field(
            loaded,
            7
        ) ==
        marc_record_get_field(
            loaded,
            8
        ))
    {
        printf(
            "ERROR: Repeated 650 fields point to the same field.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Repeated 650 fields remain distinct.\n"
    );

    /*
     * Confirm both repeated fields retain tag 650.
     */
    if (strcmp(
            marc_field_get_tag(
                marc_record_get_field(
                    loaded,
                    7
                )
            ),
            "650"
        ) != 0 ||
        strcmp(
            marc_field_get_tag(
                marc_record_get_field(
                    loaded,
                    8
                )
            ),
            "650"
        ) != 0)
    {
        printf(
            "ERROR: Repeated 650 field tags are incorrect.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Both repeated fields retain tag 650.\n"
    );

    printf(
        "Repeated-field round-trip passed.\n\n"
    );

    /*
     * ============================================================
     * 17. Special characters round-trip
     * ============================================================
     */

    printf("[17] Special characters\n");

    field = marc_record_get_field_by_tag(
        loaded,
        "500"
    );

    if (field == NULL)
    {
        printf("ERROR: Loaded 500 is missing.\n");
        marc_record_free(loaded);
        return 1;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL)
    {
        printf("ERROR: Loaded 500 $a is missing.\n");
        marc_record_free(loaded);
        return 1;
    }

    if (!check_string(
        "500 $a",
        marc_subfield_get_value(subfield),
        "O'Reilly & Associates: a test, with punctuation!"
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    printf("Special-character round-trip passed.\n\n");

    /*
     * ============================================================
     * 18. Final field lookup tests
     * ============================================================
     */

    printf("[18] Field lookup\n");

    found_field = marc_record_get_field_by_tag(
        loaded,
        "245"
    );

    if (found_field == NULL)
    {
        printf("ERROR: Could not find loaded 245.\n");
        marc_record_free(loaded);
        return 1;
    }

    printf("PASS: 245 lookup successful.\n");

    found_field = marc_record_get_field_by_tag(
        loaded,
        "001"
    );

    if (found_field == NULL)
    {
        printf("ERROR: Could not find loaded 001.\n");
        marc_record_free(loaded);
        return 1;
    }

    printf("PASS: 001 lookup successful.\n");

    found_field = marc_record_get_field_by_tag(
        loaded,
        "999"
    );

    if (found_field != NULL)
    {
        printf(
            "ERROR: Nonexistent 999 lookup did not return NULL.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    printf("PASS: Nonexistent 999 lookup returned NULL.\n\n");

    /*
     * ============================================================
     * Finished
     * ============================================================
     */

    marc_record_free(loaded);

    printf("Loaded record freed successfully.\n");
    printf("\n==========================\n");
    printf("All MARC21 tests passed!\n");

    return 0;
}