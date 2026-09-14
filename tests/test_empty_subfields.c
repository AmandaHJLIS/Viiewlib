#include <stdio.h>
#include <string.h>

#include "viiewlib/marc.h"

int main(void)
{
    MARC_Record *record;
    MARC_Record *loaded;
    MARC_Field *field;
    FILE *file;
    long file_size;

    printf("ViiewLib zero-subfield variable field test\n");
    printf("===========================================\n\n");

    printf("[1] Record creation\n");

    record = marc_record_create();

    if (record == NULL)
    {
        printf(
            "ERROR: Could not create record.\n"
        );

        return 1;
    }

    printf(
        "PASS: Record created successfully.\n"
    );

    printf("\n[2] Variable field creation\n");

    field = marc_field_create(
        "500",
        ' ',
        ' '
    );

    if (field == NULL)
    {
        printf(
            "ERROR: Could not create 500 field.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: 500 field created.\n"
    );

    if (marc_record_add_field(
        record,
        field
    ) != 0)
    {
        printf(
            "ERROR: Could not add 500 field "
            "to record.\n"
        );

        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: 500 field added to record.\n"
    );

    printf("\n[3] Zero-subfield validation\n");

    if (marc_field_get_subfield_count(field) != 0)
    {
        printf(
            "ERROR: Expected 0 subfields, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: 500 field contains exactly 0 subfields.\n"
    );

    if (marc_record_get_field_count(record) != 1)
    {
        printf(
            "ERROR: Expected 1 field, got %zu.\n",
            marc_record_get_field_count(record)
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: Record contains exactly 1 field.\n"
    );

    printf("\n[4] ISO 2709 write\n");

    file = fopen(
        "test_empty_subfields.mrc",
        "wb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open "
            "test_empty_subfields.mrc for writing.\n"
        );

        marc_record_free(record);
        return 1;
    }

    if (marc_record_write(
        record,
        file
    ) != 0)
    {
        printf(
            "ERROR: Zero-subfield variable field "
            "was rejected by the ISO 2709 writer.\n"
        );

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    if (fflush(file) != 0)
    {
        printf(
            "ERROR: Could not flush output file.\n"
        );

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
        printf(
            "ERROR: Could not seek to end of output file.\n"
        );

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    file_size = ftell(file);

    if (file_size < 0)
    {
        printf(
            "ERROR: Could not determine record size.\n"
        );

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    fclose(file);

    printf(
        "test_empty_subfields.mrc size: %ld bytes\n",
        file_size
    );

    if (file_size == 0)
    {
        printf(
            "ERROR: Generated ISO 2709 record is empty.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: Zero-subfield field written successfully.\n"
    );

    marc_record_free(record);

    printf(
        "PASS: Original record freed.\n"
    );

    printf("\n[5] ISO 2709 read\n");

    loaded = marc_record_create();

    if (loaded == NULL)
    {
        printf(
            "ERROR: Could not create loaded record.\n"
        );

        return 1;
    }

    file = fopen(
        "test_empty_subfields.mrc",
        "rb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open "
            "test_empty_subfields.mrc for reading.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    if (marc_record_read(
        loaded,
        file
    ) != 0)
    {
        printf(
            "ERROR: Zero-subfield record "
            "read failed.\n"
        );

        fclose(file);
        marc_record_free(loaded);
        return 1;
    }

    fclose(file);

    printf(
        "PASS: Zero-subfield record "
        "read successfully.\n"
    );

    printf("\n[6] Round-trip validation\n");

    if (marc_record_get_field_count(loaded) != 1)
    {
        printf(
            "ERROR: Expected 1 field after round-trip, "
            "got %zu.\n",
            marc_record_get_field_count(loaded)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Loaded record contains exactly 1 field.\n"
    );

    field = marc_record_get_field(
        loaded,
        0
    );

    if (field == NULL)
    {
        printf(
            "ERROR: Could not retrieve round-trip field.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Round-trip 500 field retrieved.\n"
    );

    if (strcmp(
        marc_field_get_tag(field),
        "500"
    ) != 0)
    {
        printf(
            "ERROR: Round-trip field tag is incorrect.\n"
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Round-trip field tag is 500.\n"
    );

    if (marc_field_get_subfield_count(field) != 0)
    {
        printf(
            "ERROR: Expected 0 subfields after "
            "round-trip, got %zu.\n",
            marc_field_get_subfield_count(field)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Round-trip field still contains "
        "exactly 0 subfields.\n"
    );

    printf("\n[7] Cleanup\n");

    marc_record_free(loaded);

    printf(
        "PASS: Loaded record freed successfully.\n"
    );

    printf("\n===========================================\n");
    printf(
        "Zero-subfield field test passed!\n"
    );

    return 0;
}