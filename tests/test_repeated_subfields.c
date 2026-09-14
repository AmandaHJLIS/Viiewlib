#include <stdio.h>
#include <stdlib.h>
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

int main(void)
{
    const char *filename =
        "test_repeated_subfields.mrc";

    MARC_Record *record = NULL;
    MARC_Record *loaded = NULL;

    MARC_Field *field = NULL;
    MARC_Field *loaded_field = NULL;

    MARC_Subfield *subfield = NULL;
    MARC_Subfield *loaded_subfield = NULL;

    FILE *file = NULL;

    int result;
    int passed = 1;

    const char *expected_a =
        "Libraries";

    const char *expected_x1 =
        "Metadata";

    const char *expected_x2 =
        "Cataloguing";

    printf(
        "ViiewLib repeated subfields test\n"
        "================================\n\n"
    );

    /*
     * [1] Record creation
     */
    printf("[1] Record creation\n");

    record = marc_record_create();

    if (record == NULL)
    {
        printf("ERROR: Record creation failed.\n");
        return EXIT_FAILURE;
    }

    printf("PASS: Record created successfully.\n\n");

    /*
     * [2] Variable field creation
     */
    printf("[2] Variable field creation\n");

    field = marc_field_create(
        "650",
        ' ',
        '0'
    );

    if (field == NULL)
    {
        printf("ERROR: 650 field creation failed.\n");
        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 650 field created.\n");

    result = marc_record_add_field(
        record,
        field
    );

    if (result != 0)
    {
        printf("ERROR: Failed to add 650 field to record.\n");
        marc_field_free(field);
        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 650 field added to record.\n\n");

    /*
     * [3] Repeated subfield creation
     *
     * 650 $a Libraries $x Metadata $x Cataloguing
     */
    printf("[3] Repeated subfield creation\n");

    result = marc_field_add_subfield(
        field,
        'a',
        expected_a
    );

    if (result != 0)
    {
        printf("ERROR: Failed to add $a subfield.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Added $a Libraries.\n");
    }

    result = marc_field_add_subfield(
        field,
        'x',
        expected_x1
    );

    if (result != 0)
    {
        printf("ERROR: Failed to add first $x subfield.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Added first $x Metadata.\n");
    }

    result = marc_field_add_subfield(
        field,
        'x',
        expected_x2
    );

    if (result != 0)
    {
        printf("ERROR: Failed to add second $x subfield.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Added second $x Cataloguing.\n");
    }

    if (marc_field_get_subfield_count(field) != 3)
    {
        printf(
            "ERROR: Expected 3 subfields, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: 650 field contains exactly 3 subfields.\n"
        );
    }

    printf("\n");

    /*
     * [4] Original subfield validation
     */
    printf("[4] Original subfield validation\n");

    for (size_t i = 0; i < 3; i++)
    {
        subfield = marc_field_get_subfield(
            field,
            i
        );

        if (subfield == NULL)
        {
            printf(
                "ERROR: Failed to retrieve original subfield %zu.\n",
                i
            );

            passed = 0;
            continue;
        }

        printf(
            "PASS: Original subfield %zu retrieved.\n",
            i
        );
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'a')
    {
        printf("ERROR: First subfield is not $a.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: First subfield code is $a.\n");

        if (!check_string(
                "First subfield value",
                marc_subfield_get_value(subfield),
                expected_a))
        {
            passed = 0;
        }
    }

    subfield = marc_field_get_subfield(
        field,
        1
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'x')
    {
        printf("ERROR: Second subfield is not $x.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Second subfield code is $x.\n");

        if (!check_string(
                "First $x value",
                marc_subfield_get_value(subfield),
                expected_x1))
        {
            passed = 0;
        }
    }

    subfield = marc_field_get_subfield(
        field,
        2
    );

    if (subfield == NULL ||
        marc_subfield_get_code(subfield) != 'x')
    {
        printf("ERROR: Third subfield is not $x.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Third subfield code is $x.\n");

        if (!check_string(
                "Second $x value",
                marc_subfield_get_value(subfield),
                expected_x2))
        {
            passed = 0;
        }
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
        printf("ERROR: Could not open output file.\n");
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
            "ERROR: Repeated-subfield record write failed.\n"
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
        printf("ERROR: Could not reopen output file.\n");
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
        printf("ERROR: Output file is empty.\n");
        passed = 0;
    }
    else
    {
        printf(
            "PASS: Repeated-subfield record "
            "written successfully.\n"
        );
    }

    marc_record_free(record);
    record = NULL;

    printf("PASS: Original record freed.\n\n");

    /*
     * [6] ISO 2709 read
     */
    printf("[6] ISO 2709 read\n");

    loaded = marc_record_create();

    if (loaded == NULL)
    {
        printf("ERROR: Loaded record creation failed.\n");
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
            "ERROR: Repeated-subfield record read failed.\n"
        );

        marc_record_free(loaded);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Repeated-subfield record "
        "read successfully.\n\n"
    );

    /*
     * [7] Round-trip validation
     */
    printf("[7] Round-trip validation\n");

    if (marc_record_get_field_count(loaded) != 1)
    {
        printf(
            "ERROR: Expected 1 field, got %zu.\n",
            marc_record_get_field_count(loaded)
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Loaded record contains exactly 1 field.\n"
        );
    }

    loaded_field = marc_record_get_field(
        loaded,
        0
    );

    if (loaded_field == NULL)
    {
        printf(
            "ERROR: Failed to retrieve round-trip 650 field.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip 650 field retrieved.\n"
        );

        if (strcmp(
                marc_field_get_tag(loaded_field),
                "650"
            ) != 0)
        {
            printf(
                "ERROR: Round-trip field tag is not 650.\n"
            );

            passed = 0;
        }
        else
        {
            printf(
                "PASS: Round-trip field tag is 650.\n"
            );
        }

        if (marc_field_get_subfield_count(
                loaded_field
            ) != 3)
        {
            printf(
                "ERROR: Expected 3 round-trip subfields, got %zu.\n",
                marc_field_get_subfield_count(loaded_field)
            );

            passed = 0;
        }
        else
        {
            printf(
                "PASS: Round-trip field contains "
                "exactly 3 subfields.\n"
            );
        }
    }

    /*
     * Verify order and repeated $x codes survived.
     */
    loaded_subfield = marc_field_get_subfield(
        loaded_field,
        0
    );

    if (loaded_subfield == NULL ||
        marc_subfield_get_code(loaded_subfield) != 'a')
    {
        printf(
            "ERROR: Round-trip first subfield is not $a.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip first subfield code is $a.\n"
        );

        if (!check_string(
                "Round-trip $a value",
                marc_subfield_get_value(loaded_subfield),
                expected_a))
        {
            passed = 0;
        }
    }

    loaded_subfield = marc_field_get_subfield(
        loaded_field,
        1
    );

    if (loaded_subfield == NULL ||
        marc_subfield_get_code(loaded_subfield) != 'x')
    {
        printf(
            "ERROR: Round-trip second subfield is not $x.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip first $x code is preserved.\n"
        );

        if (!check_string(
                "Round-trip first $x value",
                marc_subfield_get_value(loaded_subfield),
                expected_x1))
        {
            passed = 0;
        }
    }

    loaded_subfield = marc_field_get_subfield(
        loaded_field,
        2
    );

    if (loaded_subfield == NULL ||
        marc_subfield_get_code(loaded_subfield) != 'x')
    {
        printf(
            "ERROR: Round-trip third subfield is not $x.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip second $x code is preserved.\n"
        );

        if (!check_string(
                "Round-trip second $x value",
                marc_subfield_get_value(loaded_subfield),
                expected_x2))
        {
            passed = 0;
        }
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
        "================================\n"
    );

    if (!passed)
    {
        printf(
            "Repeated subfields test FAILED!\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Repeated subfields test passed!\n"
    );

    return EXIT_SUCCESS;
}