#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "viiewlib/marc.h"

#define SOURCE_FILE "test_many_fields.mrc"
#define CORRUPTED_FILE "test_bad_directory.mrc"

#define LEADER_SIZE 24
#define DIRECTORY_ENTRY_SIZE 12

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
        "ViiewLib malformed ISO 2709 directory test\n"
        "============================================\n\n"
    );

    printf("[1] Open known-good MARC record\n");

    source = fopen(SOURCE_FILE, "rb");

    if (source == NULL) {
        printf(
            "ERROR: Could not open %s\n",
            SOURCE_FILE
        );
        return EXIT_FAILURE;
    }

    if (fseek(source, 0, SEEK_END) != 0) {
        printf(
            "ERROR: Failed seeking to end of source file.\n"
        );
        fclose(source);
        return EXIT_FAILURE;
    }

    source_size = ftell(source);

    if (source_size <= LEADER_SIZE) {
        printf(
            "ERROR: Source file has invalid size.\n"
        );
        fclose(source);
        return EXIT_FAILURE;
    }

    printf(
        "PASS: Source record size is %ld bytes.\n",
        source_size
    );

    if (fseek(source, 0, SEEK_SET) != 0) {
        printf(
            "ERROR: Failed rewinding source file.\n"
        );
        fclose(source);
        return EXIT_FAILURE;
    }

    printf("\n[2] Read source record\n");

    buffer = malloc((size_t)source_size);

    if (buffer == NULL) {
        printf(
            "ERROR: Failed allocating source buffer.\n"
        );
        fclose(source);
        return EXIT_FAILURE;
    }

    if (fread(
        buffer,
        1,
        (size_t)source_size,
        source
    ) != (size_t)source_size) {

        printf(
            "ERROR: Failed reading complete source file.\n"
        );
        free(buffer);
        fclose(source);
        return EXIT_FAILURE;
    }

    fclose(source);
    source = NULL;

    printf(
        "PASS: Read complete source record.\n"
    );

    printf("\n[3] Corrupt first directory entry\n");

    /*
     * The first directory entry begins immediately after
     * the 24-byte leader.
     *
     * Directory entry layout:
     *
     *   bytes 0-2  = tag
     *   bytes 3-6  = field length
     *   bytes 7-11 = field position
     *
     * We leave the tag and field position alone and replace
     * the field length with 9999.
     *
     * This should cause the field to extend beyond the actual
     * variable-field area.
     */

    printf(
        "  First directory entry begins at byte %d.\n",
        LEADER_SIZE
    );

    printf(
        "  Original tag: %.3s\n",
        buffer + LEADER_SIZE
    );

    printf(
        "  Original field length: %.4s\n",
        buffer + LEADER_SIZE + 3
    );

    printf(
        "  Original field position: %.5s\n",
        buffer + LEADER_SIZE + 7
    );

    memcpy(
        buffer + LEADER_SIZE + 3,
        "9999",
        4
    );

    printf(
        "PASS: First directory field length changed to 9999.\n"
    );

    printf(
        "  Record length remains: %ld bytes.\n",
        source_size
    );

    printf(
        "  Physical record data remains otherwise unchanged.\n"
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
        "PASS: Created corrupted record.\n"
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
            "ERROR: Malformed directory entry was "
            "incorrectly accepted.\n"
        );
        passed = 0;
    } else {
        printf(
            "PASS: Malformed directory entry correctly rejected.\n"
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
            "Malformed directory test FAILED!\n"
        );
        return EXIT_FAILURE;
    }

    printf(
        "Malformed directory test passed!\n"
    );

    return EXIT_SUCCESS;
}