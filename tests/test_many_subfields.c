#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define SUBFIELD_COUNT 20

typedef struct
{
    char code;
    const char *value;
} ExpectedSubfield;

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

static int validate_subfields(
    MARC_Field *field,
    const ExpectedSubfield *expected
)
{
    int passed = 1;

    if (marc_field_get_subfield_count(field) != SUBFIELD_COUNT)
    {
        printf(
            "ERROR: Expected %d subfields, got %zu.\n",
            SUBFIELD_COUNT,
            marc_field_get_subfield_count(field)
        );

        return 0;
    }

    printf(
        "PASS: Field contains exactly %d subfields.\n",
        SUBFIELD_COUNT
    );

    for (size_t i = 0; i < SUBFIELD_COUNT; i++)
    {
        MARC_Subfield *subfield =
            marc_field_get_subfield(field, i);

        if (subfield == NULL)
        {
            printf(
                "ERROR: Subfield %zu could not be retrieved.\n",
                i
            );

            passed = 0;
            continue;
        }

        if (marc_subfield_get_code(subfield) != expected[i].code)
        {
            printf(
                "ERROR: Subfield %zu code mismatch\n"
                "  Expected: $%c\n"
                "  Actual:   $%c\n",
                i,
                expected[i].code,
                marc_subfield_get_code(subfield)
            );

            passed = 0;
        }
        else
        {
            printf(
                "PASS: Subfield %zu code is $%c.\n",
                i,
                expected[i].code
            );
        }

        if (!check_string(
                "Subfield value",
                marc_subfield_get_value(subfield),
                expected[i].value))
        {
            passed = 0;
        }
    }

    return passed;
}

int main(void)
{
    const char *filename =
        "test_many_subfields.mrc";

    const ExpectedSubfield expected[SUBFIELD_COUNT] =
    {
        { 'a', "Libraries" },
        { 'b', "Metadata" },
        { 'c', "Cataloguing" },
        { 'd', "MARC21" },
        { 'e', "ISO2709" },
        { 'x', "Classification" },
        { 'x', "Authority Control" },
        { 'x', "Digital Libraries" },
        { 'y', "Australia" },
        { 'y', "Western Australia" },
        { 'z', "Perth" },
        { 'z', "Forrestfield" },
        { 'a', "Repeated A" },
        { 'b', "Repeated B" },
        { 'x', "Repeated X One" },
        { 'x', "Repeated X Two" },
        { 'y', "Repeated Y" },
        { 'z', "Repeated Z" },
        { '6', "Linkage" },
        { '8', "Field Link" }
    };

    MARC_Record *record = NULL;
    MARC_Record *loaded = NULL;

    MARC_Field *field = NULL;
    MARC_Field *loaded_field = NULL;

    FILE *file = NULL;

    int result;
    int passed = 1;

    printf(
        "ViiewLib many subfields test\n"
        "============================\n\n"
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

    printf(
        "PASS: Record created successfully.\n\n"
    );

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
        printf(
            "ERROR: 650 field creation failed.\n"
        );

        marc_record_free(record);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: 650 field created.\n"
    );

    result = marc_record_add_field(
        record,
        field
    );

    if (result != 0)
    {
        printf(
            "ERROR: Failed to add 650 field to record.\n"
        );

        marc_field_free(field);
        marc_record_free(record);

        return EXIT_FAILURE;
    }

    printf(
        "PASS: 650 field added to record.\n\n"
    );

    /*
     * [3] Add many subfields
     */
    printf("[3] Adding %d subfields\n", SUBFIELD_COUNT);

    for (size_t i = 0; i < SUBFIELD_COUNT; i++)
    {
        result = marc_field_add_subfield(
            field,
            expected[i].code,
            expected[i].value
        );

        if (result != 0)
        {
            printf(
                "ERROR: Failed to add subfield %zu ($%c).\n",
                i,
                expected[i].code
            );

            passed = 0;
            break;
        }

        printf(
            "PASS: Added subfield %zu ($%c).\n",
            i,
            expected[i].code
        );
    }

    printf("\n");

    /*
     * [4] Validate original field
     */
    printf("[4] Original subfield validation\n");

    if (!validate_subfields(field, expected))
    {
        passed = 0;
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
            "ERROR: Many-subfield record write failed.\n"
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
            "PASS: Many-subfield record "
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
            "ERROR: Many-subfield record read failed.\n"
        );

        marc_record_free(loaded);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Many-subfield record "
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

        if (!validate_subfields(
                loaded_field,
                expected))
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
        "============================\n"
    );

    if (!passed)
    {
        printf(
            "Many subfields test FAILED!\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Many subfields test passed!\n"
    );

    return EXIT_SUCCESS;
}