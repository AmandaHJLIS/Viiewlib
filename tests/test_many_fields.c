#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define FIELD_COUNT 100

static int check_string(
    const char *label,
    const char *actual,
    const char *expected
)
{
    if (actual == NULL)
    {
        printf("ERROR: %s is NULL\n", label);
        return 0;
    }

    if (strcmp(actual, expected) != 0)
    {
        printf(
            "ERROR: %s mismatch\n"
            "  Expected: \"%s\"\n"
            "  Actual:   \"%s\"\n",
            label,
            expected,
            actual
        );

        return 0;
    }

    printf("PASS: %s\n", label);
    return 1;
}

static int add_variable_field(
    MARC_Record *record,
    const char *tag,
    char indicator1,
    char indicator2,
    char subfield_code,
    const char *value
)
{
    MARC_Field *field = marc_field_create(
        tag,
        indicator1,
        indicator2
    );

    if (field == NULL)
    {
        return 0;
    }

    if (marc_field_add_subfield(
            field,
            subfield_code,
            value
        ) != 0)
    {
        marc_field_free(field);
        return 0;
    }

    if (marc_record_add_field(
            record,
            field
        ) != 0)
    {
        marc_field_free(field);
        return 0;
    }

    return 1;
}

static int validate_field(
    MARC_Record *record,
    size_t index,
    const char *expected_tag,
    char expected_code,
    const char *expected_value
)
{
    MARC_Field *field =
        marc_record_get_field(record, index);

    MARC_Subfield *subfield;

    if (field == NULL)
    {
        printf(
            "ERROR: Field %zu could not be retrieved.\n",
            index
        );

        return 0;
    }

    if (strcmp(
            marc_field_get_tag(field),
            expected_tag
        ) != 0)
    {
        printf(
            "ERROR: Field %zu tag mismatch\n"
            "  Expected: %s\n"
            "  Actual:   %s\n",
            index,
            expected_tag,
            marc_field_get_tag(field)
        );

        return 0;
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL)
    {
        printf(
            "ERROR: Field %zu has no subfield 0.\n",
            index
        );

        return 0;
    }

    if (marc_subfield_get_code(subfield) != expected_code)
    {
        printf(
            "ERROR: Field %zu subfield code mismatch\n"
            "  Expected: $%c\n"
            "  Actual:   $%c\n",
            index,
            expected_code,
            marc_subfield_get_code(subfield)
        );

        return 0;
    }

    if (strcmp(
            marc_subfield_get_value(subfield),
            expected_value
        ) != 0)
    {
        printf(
            "ERROR: Field %zu subfield value mismatch\n"
            "  Expected: \"%s\"\n"
            "  Actual:   \"%s\"\n",
            index,
            expected_value,
            marc_subfield_get_value(subfield)
        );

        return 0;
    }

    return 1;
}

int main(void)
{
    const char *filename =
        "test_many_fields.mrc";

    MARC_Record *record = NULL;
    MARC_Record *loaded = NULL;

    MARC_Field *field = NULL;

    FILE *file = NULL;

    int result;
    int passed = 1;

    printf(
        "ViiewLib many fields test\n"
        "=========================\n\n"
    );

    /*
     * [1] Record creation
     */
    printf("[1] Record creation\n");

    record = marc_record_create();

    if (record == NULL)
    {
        printf(
            "ERROR: Record creation failed.\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "PASS: Record created successfully.\n\n"
    );

    /*
     * [2] Add control fields
     */
    printf("[2] Control field creation\n");

    if (marc_record_set_control_field(
            record,
            "001",
            "100000"
        ) != 0)
    {
        printf(
            "ERROR: Failed to create 001.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 001 created.\n");

    if (marc_record_set_control_field(
            record,
            "005",
            "20260914220000.0"
        ) != 0)
    {
        printf(
            "ERROR: Failed to create 005.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 005 created.\n");

    if (marc_record_set_control_field(
            record,
            "008",
            "260914s2026    xx            000 0 eng d"
        ) != 0)
    {
        printf(
            "ERROR: Failed to create 008.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 008 created.\n\n");

    /*
     * [3] Add 97 variable fields.
     *
     * The control fields above make 3 fields.
     * We add 97 more to reach exactly 100.
     *
     * Every variable field is 500 so that repeated
     * tags and field ordering are also exercised.
     */
    printf(
        "[3] Adding variable fields\n"
    );

    for (size_t i = 0; i < FIELD_COUNT - 3; i++)
    {
        char value[64];

        snprintf(
            value,
            sizeof(value),
            "Test field value %03zu",
            i + 1
        );

        if (!add_variable_field(
                record,
                "500",
                ' ',
                ' ',
                'a',
                value
            ))
        {
            printf(
                "ERROR: Failed to add variable field %zu.\n",
                i
            );

            passed = 0;
            break;
        }
    }

    if (marc_record_get_field_count(record) != FIELD_COUNT)
    {
        printf(
            "ERROR: Expected %d total fields, got %zu.\n",
            FIELD_COUNT,
            marc_record_get_field_count(record)
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Record contains exactly %d fields.\n",
            FIELD_COUNT
        );
    }

    printf("\n");

    /*
     * [4] Original record validation
     */
    printf("[4] Original record validation\n");

    /*
     * Control fields are sorted before variable fields.
     */
    field = marc_record_get_field(
        record,
        0
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "001"
        ) != 0)
    {
        printf(
            "ERROR: Expected field 0 to be 001.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Field 0 is 001.\n"
        );

        if (!check_string(
                "Original 001 value",
                marc_field_get_control_value(field),
                "100000"))
        {
            passed = 0;
        }
    }

    field = marc_record_get_field(
        record,
        1
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "005"
        ) != 0)
    {
        printf(
            "ERROR: Expected field 1 to be 005.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Field 1 is 005.\n"
        );
    }

    field = marc_record_get_field(
        record,
        2
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "008"
        ) != 0)
    {
        printf(
            "ERROR: Expected field 2 to be 008.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Field 2 is 008.\n"
        );
    }

    /*
     * Check first, middle, and last variable fields.
     */
    if (!validate_field(
            record,
            3,
            "500",
            'a',
            "Test field value 001"))
    {
        passed = 0;
    }
    else
    {
        printf(
            "PASS: First variable field validated.\n"
        );
    }

    if (!validate_field(
            record,
            50,
            "500",
            'a',
            "Test field value 048"))
    {
        passed = 0;
    }
    else
    {
        printf(
            "PASS: Middle variable field validated.\n"
        );
    }

    if (!validate_field(
            record,
            99,
            "500",
            'a',
            "Test field value 097"))
    {
        passed = 0;
    }
    else
    {
        printf(
            "PASS: Last variable field validated.\n"
        );
    }

    printf("\n");

    /*
     * [5] ISO 2709 write
     */
    printf("[5] ISO 2709 write\n");

    file = fopen(
        filename,
        "wb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open output file.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    result = marc_record_write(
        record,
        file
    );

    fclose(file);
    file = NULL;

    if (result != 0)
    {
        printf(
            "ERROR: Many-field record write failed.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    file = fopen(
        filename,
        "rb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not reopen output file.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    fseek(
        file,
        0,
        SEEK_END
    );

    long file_size = ftell(file);

    fclose(file);
    file = NULL;

    printf(
        "%s size: %ld bytes\n",
        filename,
        file_size
    );

    if (file_size <= 0)
    {
        printf(
            "ERROR: Output file is empty.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Many-field record "
            "written successfully.\n"
        );
    }

    marc_record_free(record);
    record = NULL;

    printf(
        "PASS: Original record freed.\n\n"
    );

    /*
     * [6] ISO 2709 read
     */
    printf("[6] ISO 2709 read\n");

    loaded = marc_record_create();

    if (loaded == NULL)
    {
        printf(
            "ERROR: Loaded record creation failed.\n"
        );

        return EXIT_FAILURE;
    }

    file = fopen(
        filename,
        "rb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open MARC file for reading.\n"
        );

        marc_record_free(loaded);
        return EXIT_FAILURE;
    }

    result = marc_record_read(
        loaded,
        file
    );

    fclose(file);
    file = NULL;

    if (result != 0)
    {
        printf(
            "ERROR: Many-field record read failed.\n"
        );

        marc_record_free(loaded);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Many-field record "
        "read successfully.\n\n"
    );

    /*
     * [7] Round-trip validation
     */
    printf("[7] Round-trip validation\n");

    if (marc_record_get_field_count(loaded) != FIELD_COUNT)
    {
        printf(
            "ERROR: Expected %d fields after round-trip, got %zu.\n",
            FIELD_COUNT,
            marc_record_get_field_count(loaded)
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip record contains exactly "
            "%d fields.\n",
            FIELD_COUNT
        );
    }

    /*
     * Verify control fields after sorting/reading.
     */
    field = marc_record_get_field(
        loaded,
        0
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "001"
        ) != 0)
    {
        printf(
            "ERROR: Round-trip field 0 is not 001.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip field 0 is 001.\n"
        );

        if (!check_string(
                "Round-trip 001 value",
                marc_field_get_control_value(field),
                "100000"))
        {
            passed = 0;
        }
    }

    field = marc_record_get_field(
        loaded,
        1
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "005"
        ) != 0)
    {
        printf(
            "ERROR: Round-trip field 1 is not 005.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip field 1 is 005.\n"
        );
    }

    field = marc_record_get_field(
        loaded,
        2
    );

    if (field == NULL ||
        strcmp(
            marc_field_get_tag(field),
            "008"
        ) != 0)
    {
        printf(
            "ERROR: Round-trip field 2 is not 008.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip field 2 is 008.\n"
        );
    }

    /*
     * Verify representative variable fields.
     */
    if (!validate_field(
            loaded,
            3,
            "500",
            'a',
            "Test field value 001"))
    {
        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip first variable field validated.\n"
        );
    }

    if (!validate_field(
            loaded,
            50,
            "500",
            'a',
            "Test field value 048"))
    {
        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip middle variable field validated.\n"
        );
    }

    if (!validate_field(
            loaded,
            99,
            "500",
            'a',
            "Test field value 097"))
    {
        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip last variable field validated.\n"
        );
    }

    printf("\n");

    /*
     * [8] Cleanup
     */
    printf("[8] Cleanup\n");

    marc_record_free(loaded);
    loaded = NULL;

    printf(
        "PASS: Loaded record freed successfully.\n\n"
    );

    printf(
        "=========================\n"
    );

    if (!passed)
    {
        printf(
            "Many fields test FAILED!\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Many fields test passed!\n"
    );

    return EXIT_SUCCESS;
}