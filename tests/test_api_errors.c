#include <stdio.h>
#include <string.h>
#include "viiewlib/marc.h"

static int expect_result(const char *name, MARC_Result actual, MARC_Result expected)
{
    if (actual != expected)
    {
        printf("FAIL: %s (expected %d, got %d)\n", name, (int)expected, (int)actual);
        return 0;
    }
    printf("PASS: %s\n", name);
    return 1;
}

int main(void)
{
    MARC_Record *record;
    MARC_Field *field;
    FILE *file;
    int passed = 1;

    printf("==========================\n");
    printf("ViiewLib API error tests\n");
    printf("==========================\n\n");

    passed &= expect_result(
        "NULL record rejected by marc_record_set_leader()",
        marc_record_set_leader(NULL, "00000nam a2200000   4500"),
        MARC_ERROR_INVALID_ARGUMENT
    );

    record = marc_record_create();
    if (record == NULL)
    {
        printf("FAIL: Could not create test record.\n");
        return 1;
    }

    passed &= expect_result(
        "Invalid leader length rejected",
        marc_record_set_leader(record, "short"),
        MARC_ERROR_INVALID_ARGUMENT
    );

    passed &= expect_result(
        "NULL field rejected by marc_record_add_field()",
        marc_record_add_field(record, NULL),
        MARC_ERROR_INVALID_ARGUMENT
    );

    passed &= expect_result(
        "Invalid add_field leaves field count unchanged",
        (marc_record_add_field(record, NULL) == MARC_ERROR_INVALID_ARGUMENT &&
         marc_record_get_field_count(record) == 0)
            ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
        MARC_SUCCESS
    );

    field = marc_field_create("245", '1', '0');
    if (field == NULL)
    {
        printf("FAIL: Could not create test field.\n");
        marc_record_free(record);
        return 1;
    }

    passed &= expect_result(
        "Control value rejected on data field",
        marc_field_set_control_value(field, "invalid"),
        MARC_ERROR_INVALID_ARGUMENT
    );

    passed &= expect_result(
        "Invalid control value leaves data field unchanged",
        (marc_field_get_control_value(field) == NULL &&
         marc_field_get_subfield_count(field) == 0)
            ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
        MARC_SUCCESS
    );

    passed &= expect_result(
        "NUL subfield code rejected",
        marc_field_add_subfield(field, '\0', "invalid"),
        MARC_ERROR_INVALID_ARGUMENT
    );

    marc_field_free(field);

    field = marc_field_create("001", '\0', '\0');
    if (field == NULL)
    {
        printf("FAIL: Could not create control field.\n");
        marc_record_free(record);
        return 1;
    }

    passed &= expect_result(
        "Subfield rejected on control field",
        marc_field_add_subfield(field, 'a', "invalid"),
        MARC_ERROR_INVALID_ARGUMENT
    );

    marc_field_free(field);

    passed &= expect_result(
        "NULL record rejected by marc_record_write()",
        marc_record_write(NULL, stdout),
        MARC_ERROR_INVALID_ARGUMENT
    );

    passed &= expect_result(
        "NULL stream rejected by marc_record_write()",
        marc_record_write(record, NULL),
        MARC_ERROR_INVALID_ARGUMENT
    );

    passed &= expect_result(
        "NULL record rejected by marc_record_read()",
        marc_record_read(NULL, stdout),
        MARC_ERROR_INVALID_ARGUMENT
    );

    passed &= expect_result(
        "NULL stream rejected by marc_record_read()",
        marc_record_read(record, NULL),
        MARC_ERROR_INVALID_ARGUMENT
    );

    file = tmpfile();
    if (file == NULL)
    {
        printf("FAIL: Could not create temporary file.\n");
        marc_record_free(record);
        return 1;
    }

    passed &= expect_result(
        "Clean EOF reported distinctly",
        marc_record_read(record, file),
        MARC_ERROR_EOF
    );
    fclose(file);

    file = tmpfile();
    if (file == NULL)
    {
        printf("FAIL: Could not create temporary file.\n");
        marc_record_free(record);
        return 1;
    }

    fputs("00010", file);
    rewind(file);

    passed &= expect_result(
        "Partial leader reported as truncated",
        marc_record_read(record, file),
        MARC_ERROR_TRUNCATED
    );
    fclose(file);

    file = tmpfile();
    if (file == NULL)
    {
        printf("FAIL: Could not create temporary file.\n");
        marc_record_free(record);
        return 1;
    }

    fputs("ABCDEnam a2200000   4500", file);
    rewind(file);

    passed &= expect_result(
        "Malformed leader reported as malformed",
        marc_record_read(record, file),
        MARC_ERROR_MALFORMED
    );
    fclose(file);

    marc_record_free(record);

    printf("\n==========================\n");

    if (!passed)
    {
        printf("API error tests FAILED.\n");
        return 1;
    }

    printf("All API error tests passed!\n");
    return 0;
}
