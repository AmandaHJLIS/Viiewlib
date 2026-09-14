#include <stdio.h>
#include <stdlib.h>

#include "viiewlib/marc.h"

#define SOURCE_FILE "test_many_fields.mrc"
#define CORRUPTED_FILE "test_missing_terminator.mrc"

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
        "ViiewLib missing ISO 2709 record terminator test\n"
        "=================================================\n\n"
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

    if (source_size <= 25) {
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

    printf("\n[3] Remove record terminator\n");

    /*
     * The final byte of a valid ISO 2709 record must be
     * the record terminator (0x1D).
     */

    printf(
        "  Final byte before corruption: 0x%02X\n",
        buffer[source_size - 1]
    );

    if (buffer[source_size - 1] != 0x1D) {
        printf(
            "ERROR: Source record does not end with "
            "0x1D as expected.\n"
        );
        free(buffer);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Source record ends with record terminator 0x1D.\n"
    );

    /*
     * We deliberately do NOT replace the terminator with another
     * byte. Instead, we physically remove it from the file.
     */

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
        (size_t)source_size - 1,
        output
    ) != (size_t)source_size - 1) {

        printf(
            "ERROR: Failed writing corrupted record.\n"
        );
        fclose(output);
        free(buffer);
        return EXIT_FAILURE;
    }

    fclose(output);
    output = NULL;

    printf(
        "PASS: Created record without final record terminator.\n"
    );

    printf(
        "  Original size:  %ld bytes\n",
        source_size
    );

    printf(
        "  Corrupted size: %ld bytes\n",
        source_size - 1
    );

    printf(
        "  Removed:        1 byte (0x1D)\n"
    );

    free(buffer);
    buffer = NULL;

    printf("\n[4] Attempt to read corrupted record\n");

    record = marc_record_create();

    if (record == NULL) {
        printf(
            "ERROR: Failed creating MARC record.\n"
        );
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
            "ERROR: Missing record terminator was "
            "incorrectly accepted.\n"
        );
        passed = 0;
    } else {
        printf(
            "PASS: Missing record terminator correctly rejected.\n"
        );
    }

    printf("\n[5] Cleanup\n");

    marc_record_free(record);
    record = NULL;

    printf(
        "PASS: MARC record freed successfully.\n"
    );

    printf(
        "\n=================================================\n"
    );

    if (!passed) {
        printf(
            "Missing record terminator test FAILED!\n"
        );
        return EXIT_FAILURE;
    }

    printf(
        "Missing record terminator test passed!\n"
    );

    return EXIT_SUCCESS;
}