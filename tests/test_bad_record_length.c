#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define SOURCE_FILE "test_many_fields.mrc"
#define CORRUPTED_FILE "test_bad_record_length.mrc"

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
        "ViiewLib incorrect ISO 2709 record length test\n"
        "===============================================\n\n"
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

    printf("PASS: Source record size is %ld bytes.\n", source_size);

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

    if (fread(buffer, 1, (size_t)source_size, source)
        != (size_t)source_size) {

        printf("ERROR: Failed reading complete source file.\n");
        free(buffer);
        fclose(source);
        return EXIT_FAILURE;
    }

    fclose(source);
    source = NULL;

    printf("PASS: Read complete source record.\n");

    printf("\n[3] Corrupt leader record length\n");

    /*
     * ISO 2709 stores the record length as five ASCII digits
     * at leader positions 00-04.
     *
     * The real record is 3716 bytes, but we deliberately claim
     * that it is 3715 bytes.
     *
     * The physical file remains completely intact.
     */

    if (source_size > 99998) {
        printf("ERROR: Source record is unexpectedly large.\n");
        free(buffer);
        return EXIT_FAILURE;
    }

    snprintf(
        (char *)buffer,
        6,
        "%05ld",
        source_size - 1
    );

    printf(
        "PASS: Leader now claims record length = %ld bytes.\n",
        source_size - 1
    );

    printf(
        "  Actual physical size = %ld bytes.\n",
        source_size
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
            "ERROR: Incorrect record length was "
            "incorrectly accepted.\n"
        );
        passed = 0;
    } else {
        printf(
            "PASS: Incorrect record length correctly rejected.\n"
        );
    }

    printf("\n[5] Cleanup\n");

    marc_record_free(record);
    record = NULL;

    printf("PASS: MARC record freed successfully.\n");

    printf("\n===============================================\n");

    if (!passed) {
        printf(
            "Incorrect record length test FAILED!\n"
        );
        return EXIT_FAILURE;
    }

    printf(
        "Incorrect record length test passed!\n"
    );

    return EXIT_SUCCESS;
}