#include <stdio.h>

#include "viiewlib/marc.h"

int main(void)
{
    MARC_Record *record;
    FILE *file;
    int result;

    printf("ViiewLib empty MARC record test\n");
    printf("===============================\n\n");

    /*
     * ============================================================
     * 1. Empty record creation
     * ============================================================
     */

    printf("[1] Empty record creation\n");

    record = marc_record_create();

    if (record == NULL)
    {
        printf("ERROR: Could not create empty record.\n");
        return 1;
    }

    printf("PASS: Empty record created.\n");

    if (marc_record_get_field_count(record) != 0)
    {
        printf(
            "ERROR: Expected 0 fields, got %zu.\n",
            marc_record_get_field_count(record)
        );

        marc_record_free(record);
        return 1;
    }

    printf("PASS: Empty record contains 0 fields.\n\n");

    /*
     * ============================================================
     * 2. Empty record ISO 2709 write rejection
     * ============================================================
     */

    printf("[2] Empty record ISO 2709 write rejection\n");

    file = fopen(
        "test_empty.mrc",
        "wb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open test_empty.mrc for writing.\n"
        );

        marc_record_free(record);
        return 1;
    }

    result = marc_record_write(
        record,
        file
    );

    fclose(file);

    /*
     * An empty record is currently rejected by
     * the ISO 2709 writer. This is expected behaviour.
     */
    if (result == 0)
    {
        printf(
            "ERROR: Empty record was unexpectedly accepted "
            "by ISO 2709 writer.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: Empty record correctly rejected by "
        "ISO 2709 writer.\n"
    );

    /*
     * ============================================================
     * 3. Cleanup
     * ============================================================
     */

    printf("\n[3] Cleanup\n");

    marc_record_free(record);

    printf("PASS: Empty record freed successfully.\n");

    printf("\n===============================\n");
    printf("Empty record test passed!\n");

    return 0;
}