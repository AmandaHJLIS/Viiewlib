#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define SOURCE_FILE "test_many_fields.mrc"
#define CORRUPTED_FILE "test_bad_base_address.mrc"

int main(void)
{
    FILE *source = NULL;
    FILE *output = NULL;

    MARC_Record *record = NULL;

    unsigned char *buffer = NULL;

    long source_size;

    int result;
    int passed = 1;

    printf(
        "ViiewLib invalid ISO 2709 base address test\n"
        "============================================\n\n"
    );

    printf("[1] Open known-good MARC record\n");

    source = fopen(SOURCE_FILE, "rb");

    if (source == NULL) {
        printf("ERROR: Could not open %s\n", SOURCE_FILE);
        return EXIT_FAILURE;
    }

    if (fseek(source, 0, SEEK_END) != 0) {
        printf("ERROR: Failed seeking to end of source file.\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    source_size = ftell(source);

    if (source_size <= 24) {
        printf("ERROR: Source file has invalid size.\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Source record size is %ld bytes.\n",
        source_size
    );

    if (fseek(source, 0, SEEK_SET) != 0) {
        printf("ERROR: Failed rewinding source file.\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    printf("\n[2] Read source record\n");

    buffer = malloc((size_t)source_size);

    if (buffer == NULL) {
        printf("ERROR: Failed allocating source buffer.\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    if (fread(
        buffer,
        1,
        (size_t)source_size,
        source
    ) != (size_t)source_size) {

        printf("ERROR: Failed reading complete source file.\n");
        free(buffer);
        fclose(source);
        return EXIT_FAILURE;
    }

    fclose(source);
    source = NULL;

    printf("PASS: Read complete source record.\n");

    printf("\n[3] Corrupt leader base address\n");

    /*
     * ISO 2709 stores the base address of data at
     * leader positions 12-16 as five ASCII digits.
     *
     * The known-good record uses:
     *
     *     1225
     *
     * We replace it with:
     *
     *     99999
     *
     * This is larger than the complete record and therefore
     * cannot be a valid base address.
     */

    memcpy(
        buffer + 12,
        "99999",
        5
    );

    printf(
        "PASS: Leader base address changed from 1225 to 99999.\n"
    );

    printf(
        "  Record length remains: %ld bytes.\n",
        source_size
    );

    printf(
        "  Physical record remains intact.\n"
    );

    output = fopen(CORRUPTED_FILE, "wb");

    if (output == NULL) {
        printf(
            "ERROR: Could not create %s\n",
            CORRUPTED_FILE
        );
        free(buffer);
        return EXIT_FAILURE;
    }

    if (fwrite(
        buffer,
        1,
        (size_t)source_size,
        output
    ) != (size_t)source_size) {

        printf("ERROR: Failed writing corrupted record.\n");
        fclose(output);
        free(buffer);
        return EXIT_FAILURE;
    }

    fclose(output);
    output = NULL;

    printf("PASS: Created corrupted record.\n");

    free(buffer);
    buffer = NULL;

    printf("\n[4] Attempt to read corrupted record\n");

    record = marc_record_create();

    if (record == NULL) {
        printf("ERROR: Failed creating MARC record.\n");
        return EXIT_FAILURE;
    }

    source = fopen(CORRUPTED_FILE, "rb");

    if (source == NULL) {
        printf(
            "ERROR: Could not open corrupted record.\n"
        );
        marc_record_free(record);
        return EXIT_FAILURE;
    }

    result = marc_record_read(record, source);

    fclose(source);
    source = NULL;

    if (result == 0) {
        printf(
            "ERROR: Invalid base address was incorrectly accepted.\n"
        );
        passed = 0;
    } else {
        printf(
            "PASS: Invalid base address correctly rejected.\n"
        );
    }

    printf("\n[5] Cleanup\n");

    marc_record_free(record);
    record = NULL;

    printf(
        "PASS: MARC record freed successfully.\n"
    );

    printf(
        "\n============================================\n"
    );

    if (!passed) {
        printf(
            "Invalid base address test FAILED!\n"
        );
        return EXIT_FAILURE;
    }

    printf(
        "Invalid base address test passed!\n"
    );

    return EXIT_SUCCESS;
}