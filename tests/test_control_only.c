#include <stdio.h>
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
        printf(
            "ERROR: %s is NULL.\n",
            label
        );

        return 0;
    }

    if (strcmp(actual, expected) != 0)
    {
        printf(
            "ERROR: %s is incorrect.\n"
            "       Expected: %s\n"
            "       Got:      %s\n",
            label,
            expected,
            actual
        );

        return 0;
    }

    printf(
        "PASS: %s\n",
        label
    );

    return 1;
}

int main(void)
{
    MARC_Record *record;
    MARC_Record *loaded;
    FILE *file;
    long file_size;

    const char *expected_001 =
        "123456";

    const char *expected_005 =
        "20260914165000.0";

    const char *expected_008 =
        "260914s2026    xx            000 0 eng d";

    printf("ViiewLib control-fields-only test\n");
    printf("=================================\n\n");

    printf("[1] Control-fields-only record creation\n");

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

    printf("\n[2] Control field creation\n");

    if (marc_record_set_control_field(
        record,
        "001",
        expected_001
    ) != 0)
    {
        printf(
            "ERROR: Could not create 001.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: 001 created.\n"
    );

    if (marc_record_set_control_field(
        record,
        "005",
        expected_005
    ) != 0)
    {
        printf(
            "ERROR: Could not create 005.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: 005 created.\n"
    );

    if (marc_record_set_control_field(
        record,
        "008",
        expected_008
    ) != 0)
    {
        printf(
            "ERROR: Could not create 008.\n"
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: 008 created.\n"
    );

    printf("\n[3] Original control fields\n");

    if (!check_string(
        "Original 001",
        marc_record_get_control_field(
            record,
            "001"
        ),
        expected_001
    ))
    {
        marc_record_free(record);
        return 1;
    }

    if (!check_string(
        "Original 005",
        marc_record_get_control_field(
            record,
            "005"
        ),
        expected_005
    ))
    {
        marc_record_free(record);
        return 1;
    }

    if (!check_string(
        "Original 008",
        marc_record_get_control_field(
            record,
            "008"
        ),
        expected_008
    ))
    {
        marc_record_free(record);
        return 1;
    }

    if (marc_record_get_field_count(record) != 3)
    {
        printf(
            "ERROR: Expected 3 fields, got %zu.\n",
            marc_record_get_field_count(record)
        );

        marc_record_free(record);
        return 1;
    }

    printf(
        "PASS: Record contains exactly 3 control fields.\n"
    );

    printf("\n[4] ISO 2709 write\n");

    file = fopen(
        "test_control_only.mrc",
        "wb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open "
            "test_control_only.mrc for writing.\n"
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
            "ERROR: Control-fields-only record "
            "write failed.\n"
        );

        fclose(file);
        marc_record_free(record);
        return 1;
    }

    if (fflush(file) != 0)
    {
        printf(
            "ERROR: Could not flush "
            "test_control_only.mrc.\n"
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
            "ERROR: Could not seek to end of "
            "test_control_only.mrc.\n"
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
        "test_control_only.mrc size: %ld bytes\n",
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
        "PASS: Control-fields-only record "
        "written successfully.\n"
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
        "test_control_only.mrc",
        "rb"
    );

    if (file == NULL)
    {
        printf(
            "ERROR: Could not open "
            "test_control_only.mrc for reading.\n"
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
            "ERROR: Control-fields-only record "
            "read failed.\n"
        );

        fclose(file);
        marc_record_free(loaded);
        return 1;
    }

    fclose(file);

    printf(
        "PASS: Control-fields-only record "
        "read successfully.\n"
    );

    printf("\n[6] Control field round-trip\n");

    if (!check_string(
        "Round-trip 001",
        marc_record_get_control_field(
            loaded,
            "001"
        ),
        expected_001
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    if (!check_string(
        "Round-trip 005",
        marc_record_get_control_field(
            loaded,
            "005"
        ),
        expected_005
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    if (!check_string(
        "Round-trip 008",
        marc_record_get_control_field(
            loaded,
            "008"
        ),
        expected_008
    ))
    {
        marc_record_free(loaded);
        return 1;
    }

    if (marc_record_get_field_count(loaded) != 3)
    {
        printf(
            "ERROR: Expected 3 fields after round-trip, "
            "got %zu.\n",
            marc_record_get_field_count(loaded)
        );

        marc_record_free(loaded);
        return 1;
    }

    printf(
        "PASS: Loaded record contains exactly "
        "3 control fields.\n"
    );

    printf("\n[7] Cleanup\n");

    marc_record_free(loaded);

    printf(
        "PASS: Loaded record freed successfully.\n"
    );

    printf("\n=================================\n");
    printf(
        "Control-fields-only test passed!\n"
    );

    return 0;
}