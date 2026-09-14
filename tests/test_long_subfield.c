#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define LONG_VALUE_LENGTH 1000

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
            "  Expected length: %zu\n"
            "  Actual length:   %zu\n",
            label,
            strlen(expected),
            strlen(actual)
        );

        return 0;
    }

    printf(
        "PASS: %s (%zu characters)\n",
        label,
        strlen(actual)
    );

    return 1;
}

int main(void)
{
    const char *filename =
        "test_long_subfield.mrc";

    MARC_Record *record = NULL;
    MARC_Record *loaded = NULL;

    MARC_Field *field = NULL;
    MARC_Field *loaded_field = NULL;

    MARC_Subfield *subfield = NULL;
    MARC_Subfield *loaded_subfield = NULL;

    FILE *file = NULL;

    char *long_value = NULL;

    int result;
    int passed = 1;

    printf(
        "ViiewLib long subfield value test\n"
        "=================================\n\n"
    );

    /*
     * [1] Generate long value
     */
    printf("[1] Long value generation\n");

    long_value = malloc(
        LONG_VALUE_LENGTH + 1
    );

    if (long_value == NULL)
    {
        printf(
            "ERROR: Failed to allocate long subfield value.\n"
        );

        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < LONG_VALUE_LENGTH; i++)
    {
        /*
         * Use a predictable repeating pattern.
         */
        long_value[i] =
            (char)('A' + (i % 26));
    }

    long_value[LONG_VALUE_LENGTH] = '\0';

    printf(
        "PASS: Generated subfield value of %zu characters.\n\n",
        strlen(long_value)
    );

    /*
     * [2] Record creation
     */
    printf("[2] Record creation\n");

    record = marc_record_create();

    if (record == NULL)
    {
        printf("ERROR: Record creation failed.\n");
        free(long_value);
        return EXIT_FAILURE;
    }

    printf("PASS: Record created successfully.\n\n");

    /*
     * [3] Variable field creation
     */
    printf("[3] Variable field creation\n");

    field = marc_field_create(
        "500",
        ' ',
        ' '
    );

    if (field == NULL)
    {
        printf("ERROR: 500 field creation failed.\n");
        marc_record_free(record);
        free(long_value);
        return EXIT_FAILURE;
    }

    printf("PASS: 500 field created.\n");

    result = marc_record_add_field(
        record,
        field
    );

    if (result != 0)
    {
        printf(
            "ERROR: Failed to add 500 field to record.\n"
        );

        marc_field_free(field);
        marc_record_free(record);
        free(long_value);

        return EXIT_FAILURE;
    }

    printf("PASS: 500 field added to record.\n\n");

    /*
     * [4] Long subfield creation
     */
    printf("[4] Long subfield creation\n");

    result = marc_field_add_subfield(
        field,
        'a',
        long_value
    );

    if (result != 0)
    {
        printf(
            "ERROR: Failed to add long $a subfield.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Added $a subfield with %zu characters.\n",
            strlen(long_value)
        );
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
            "PASS: Field contains exactly 1 subfield.\n"
        );
    }

    subfield = marc_field_get_subfield(
        field,
        0
    );

    if (subfield == NULL)
    {
        printf(
            "ERROR: Failed to retrieve long $a subfield.\n"
        );

        passed = 0;
    }
    else
    {
        if (marc_subfield_get_code(subfield) != 'a')
        {
            printf(
                "ERROR: Subfield code is not $a.\n"
            );

            passed = 0;
        }
        else
        {
            printf(
                "PASS: Subfield code is $a.\n"
            );
        }

        if (!check_string(
                "Original long subfield value",
                marc_subfield_get_value(subfield),
                long_value))
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
        printf(
            "ERROR: Could not open output file.\n"
        );

        marc_record_free(record);
        free(long_value);

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
            "ERROR: Long subfield record write failed.\n"
        );

        marc_record_free(record);
        free(long_value);

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
        free(long_value);

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
            "PASS: Long subfield record "
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

        free(long_value);
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
        free(long_value);

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
            "ERROR: Long subfield record read failed.\n"
        );

        marc_record_free(loaded);
        free(long_value);

        return EXIT_FAILURE;
    }

    printf(
        "PASS: Long subfield record "
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
            "ERROR: Failed to retrieve round-trip 500 field.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Round-trip 500 field retrieved.\n"
        );

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
            printf(
                "PASS: Round-trip field tag is 500.\n"
            );
        }

        if (marc_field_get_subfield_count(
                loaded_field
            ) != 1)
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
                "PASS: Round-trip field contains "
                "exactly 1 subfield.\n"
            );
        }
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
        if (marc_subfield_get_code(
                loaded_subfield
            ) != 'a')
        {
            printf(
                "ERROR: Round-trip subfield code is not $a.\n"
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
                "Round-trip long subfield value",
                marc_subfield_get_value(loaded_subfield),
                long_value))
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

    free(long_value);
    long_value = NULL;

    printf(
        "PASS: Loaded record and long value freed successfully.\n\n"
    );

    printf(
        "=================================\n"
    );

    if (!passed)
    {
        printf(
            "Long subfield value test FAILED!\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Long subfield value test passed!\n"
    );

    return EXIT_SUCCESS;
}