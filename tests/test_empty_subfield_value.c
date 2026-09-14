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
        "test_empty_subfield_value.mrc";

    MARC_Record *record = NULL;
    MARC_Record *loaded = NULL;
    MARC_Field *field = NULL;
    MARC_Field *loaded_field = NULL;
    MARC_Subfield *subfield = NULL;
    MARC_Subfield *loaded_subfield = NULL;

    FILE *file = NULL;

    int result;
    int passed = 1;

    printf(
        "ViiewLib empty subfield value test\n"
        "=================================\n\n"
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
        "500",
        ' ',
        ' '
    );

    if (field == NULL)
    {
        printf("ERROR: 500 field creation failed.\n");
        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 500 field created.\n");

    result = marc_record_add_field(
        record,
        field
    );

    if (result != 0)
    {
        printf("ERROR: Failed to add 500 field to record.\n");
        marc_field_free(field);
        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf("PASS: 500 field added to record.\n\n");

    /*
     * [3] Empty subfield validation
     */
    printf("[3] Empty subfield validation\n");

    result = marc_field_add_subfield(
        field,
        'a',
        ""
    );

    if (result != 0)
    {
        printf("ERROR: Failed to add empty $a subfield.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Empty $a subfield added.\n");
    }

    if (marc_field_get_subfield_count(field) != 1)
    {
        printf(
            "ERROR: Expected 1 subfield, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: 500 field contains exactly 1 subfield.\n"
        );
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL)
    {
        printf("ERROR: Failed to retrieve $a subfield.\n");
        passed = 0;
    }
    else
    {
        if (marc_subfield_get_code(subfield) != 'a')
        {
            printf(
                "ERROR: Expected subfield code 'a', got '%c'.\n",
                marc_subfield_get_code(subfield)
            );

            passed = 0;
        }
        else
        {
            printf("PASS: Subfield code is $a.\n");
        }

        if (!check_string(
                "Subfield value",
                marc_subfield_get_value(subfield),
                ""))
        {
            passed = 0;
        }
    }

    printf("\n");

    /*
     * [4] ISO 2709 write
     */
    printf("[4] ISO 2709 write\n");

    file = fopen(filename, "wb");

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
            "ERROR: Empty-subfield-value record write failed.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("ERROR: Could not reopen output file.\n");
        marc_record_free(record);
        return EXIT_FAILURE;
    }

    fseek(file, 0, SEEK_END);
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
            "PASS: Empty-subfield-value record "
            "written successfully.\n"
        );
    }

    marc_record_free(record);
    record = NULL;

    printf("PASS: Original record freed.\n\n");

    /*
     * [5] ISO 2709 read
     */
    printf("[5] ISO 2709 read\n");

    loaded = marc_record_create();

    if (loaded == NULL)
    {
        printf("ERROR: Loaded record creation failed.\n");
        return EXIT_FAILURE;
    }

    file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("ERROR: Could not open MARC file for reading.\n");
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
            "ERROR: Empty-subfield-value record read failed.\n"
        );

        marc_record_free(loaded);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Empty-subfield-value record read successfully.\n\n"
    );

    /*
     * [6] Round-trip validation
     */
    printf("[6] Round-trip validation\n");

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
        printf("PASS: Loaded record contains exactly 1 field.\n");
    }

    loaded_field = marc_record_get_field(
        loaded,
        0
    );

    if (loaded_field == NULL)
    {
        printf("ERROR: Failed to retrieve round-trip 500 field.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Round-trip 500 field retrieved.\n");

        if (strcmp(
                marc_field_get_tag(loaded_field),
                "500"
            ) != 0)
        {
            printf(
                "ERROR: Round-trip field tag is not 500.\n"
            );

            passed = 0;
        }
        else
        {
            printf("PASS: Round-trip field tag is 500.\n");
        }

        if (marc_field_get_subfield_count(loaded_field) != 1)
        {
            printf(
                "ERROR: Expected 1 round-trip subfield, got %zu.\n",
                marc_field_get_subfield_count(loaded_field)
            );

            passed = 0;
        }
        else
        {
            printf(
                "PASS: Round-trip field contains exactly 1 subfield.\n"
            );
        }

        loaded_subfield = marc_field_get_subfield(
            loaded_field,
            0
        );

        if (loaded_subfield == NULL)
        {
            printf(
                "ERROR: Failed to retrieve round-trip $a subfield.\n"
            );

            passed = 0;
        }
        else
        {
            if (marc_subfield_get_code(loaded_subfield) != 'a')
            {
                printf(
                    "ERROR: Round-trip subfield code is not 'a'.\n"
                );

                passed = 0;
            }
            else
            {
                printf(
                    "PASS: Round-trip subfield code is $a.\n"
                );
            }

            if (!check_string(
                    "Round-trip subfield value",
                    marc_subfield_get_value(loaded_subfield),
                    ""))
            {
                passed = 0;
            }
        }
    }

    printf("\n");

    /*
     * [7] Cleanup
     */
    printf("[7] Cleanup\n");

    marc_record_free(loaded);
    loaded = NULL;

    printf("PASS: Loaded record freed successfully.\n\n");

    printf(
        "=================================\n"
    );

    if (!passed)
    {
        printf(
            "Empty subfield value test FAILED!\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Empty subfield value test passed!\n"
    );

    return EXIT_SUCCESS;
}