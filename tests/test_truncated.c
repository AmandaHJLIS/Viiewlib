#include <stdio.h>
#include <stdlib.h>

#include "viiewlib/marc.h"

#define SOURCE_FILE "test_many_fields.mrc"
#define TRUNCATED_FILE "test_truncated.mrc"
#define TRUNCATE_BYTES 20

int main(void)
{
    FILE *source = NULL;
    FILE *output = NULL;

    MARC_Record *record = NULL;

    unsigned char *buffer = NULL;

    long source_size;
    size_t truncated_size;

    int result;
    int passed = 1;

    printf(
        "ViiewLib truncated ISO 2709 test\n"
        "=================================\n\n"
    );

    /*
     * --------------------------------------------------------------
     * [1] Open known-good MARC record
     * --------------------------------------------------------------
     */

    printf("[1] Open known-good MARC record\n");

    source = fopen(
        SOURCE_FILE,
        "rb"
    );

    if (source == NULL)
    {
        printf(
            "ERROR: Could not open %s\n",
            SOURCE_FILE
        );

        return EXIT_FAILURE;
    }

    if (fseek(
            source,
            0,
            SEEK_END
        ) != 0)
    {
        printf(
            "ERROR: Failed seeking to end of source file.\n"
        );

        fclose(source);

        return EXIT_FAILURE;
    }

    source_size = ftell(source);

    if (source_size <= 0)
    {
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

    if (source_size <= TRUNCATE_BYTES)
    {
        printf(
            "ERROR: Source record is too small to truncate.\n"
        );

        fclose(source);

        return EXIT_FAILURE;
    }

    /*
     * --------------------------------------------------------------
     * [2] Read source record
     * --------------------------------------------------------------
     */

    printf("\n[2] Read source record\n");

    if (fseek(
            source,
            0,
            SEEK_SET
        ) != 0)
    {
        printf(
            "ERROR: Failed rewinding source file.\n"
        );

        fclose(source);

        return EXIT_FAILURE;
    }

    buffer = malloc(
        (size_t)source_size
    );

    if (buffer == NULL)
    {
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
        ) != (size_t)source_size)
    {
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

    /*
     * --------------------------------------------------------------
     * [3] Create truncated record
     * --------------------------------------------------------------
     */

    printf("\n[3] Create truncated record\n");

    truncated_size =
        (size_t)source_size -
        TRUNCATE_BYTES;

    output = fopen(
        TRUNCATED_FILE,
        "wb"
    );

    if (output == NULL)
    {
        printf(
            "ERROR: Could not create %s\n",
            TRUNCATED_FILE
        );

        free(buffer);

        return EXIT_FAILURE;
    }

    if (fwrite(
            buffer,
            1,
            truncated_size,
            output
        ) != truncated_size)
    {
        printf(
            "ERROR: Failed writing truncated record.\n"
        );

        fclose(output);
        free(buffer);

        return EXIT_FAILURE;
    }

    fclose(output);
    output = NULL;

    printf(
        "PASS: Created truncated record.\n"
    );

    printf(
        "  Original size:  %ld bytes\n",
        source_size
    );

    printf(
        "  Truncated size: %zu bytes\n",
        truncated_size
    );

    printf(
        "  Removed:        %d bytes\n",
        TRUNCATE_BYTES
    );

    free(buffer);
    buffer = NULL;

    /*
     * --------------------------------------------------------------
     * [4] Attempt to read truncated record
     * --------------------------------------------------------------
     */

    printf(
        "\n[4] Attempt to read truncated record\n"
    );

    record = marc_record_create();

    if (record == NULL)
    {
        printf(
            "ERROR: Failed creating MARC record.\n"
        );

        return EXIT_FAILURE;
    }

    source = fopen(
        TRUNCATED_FILE,
        "rb"
    );

    if (source == NULL)
    {
        printf(
            "ERROR: Could not open truncated record.\n"
        );

        marc_record_free(record);

        return EXIT_FAILURE;
    }

    result = marc_record_read(
        record,
        source
    );

    fclose(source);
    source = NULL;

    /*
     * The truncated record MUST be rejected.
     */

    if (result == 0)
    {
        printf(
            "ERROR: Truncated record was incorrectly accepted.\n"
        );

        passed = 0;
    }
    else
    {
        printf(
            "PASS: Truncated record correctly rejected.\n"
        );
    }

    /*
     * --------------------------------------------------------------
     * [5] Cleanup
     * --------------------------------------------------------------
     */

    printf("\n[5] Cleanup\n");

    marc_record_free(record);
    record = NULL;

    printf(
        "PASS: MARC record freed successfully.\n"
    );

    printf(
        "\n=================================\n"
    );

    if (!passed)
    {
        printf(
            "Truncated ISO 2709 test FAILED!\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Truncated ISO 2709 test passed!\n"
    );

    return EXIT_SUCCESS;
}