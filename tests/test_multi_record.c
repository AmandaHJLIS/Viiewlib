#include <stdio.h>
#include <string.h>
#include "viiewlib/marc.h"

static int check_string(const char *name, const char *actual, const char *expected)
{
    if (actual == NULL || strcmp(actual, expected) != 0)
    {
        printf("FAIL: %s\n", name);
        return 0;
    }
    printf("PASS: %s\n", name);
    return 1;
}

static MARC_Record *make_record(const char *id, const char *title)
{
    MARC_Record *record = marc_record_create();
    MARC_Field *field;
    if (record == NULL || marc_record_set_control_field(record, "001", id) != MARC_SUCCESS)
    {
        marc_record_free(record);
        return NULL;
    }
    field = marc_field_create("245", '1', '0');
    if (field == NULL || marc_field_add_subfield(field, 'a', title) != MARC_SUCCESS)
    {
        marc_field_free(field);
        marc_record_free(record);
        return NULL;
    }
    if (marc_record_add_field(record, field) != MARC_SUCCESS)
    {
        marc_field_free(field);
        marc_record_free(record);
        return NULL;
    }
    return record;
}

int main(void)
{
    MARC_Record *first = make_record("000001", "First test record");
    MARC_Record *second = make_record("000002", "Second test record");
    MARC_Record *loaded_first = marc_record_create();
    MARC_Record *loaded_second = marc_record_create();
    MARC_Field *field;
    MARC_Subfield *subfield;
    FILE *file;
    int passed = 1;

    printf("==========================\nViiewLib multi-record tests\n==========================\n\n");

    if (first == NULL || second == NULL || loaded_first == NULL || loaded_second == NULL)
    {
        printf("FAIL: Could not create test records.\n");
        return 1;
    }

    file = tmpfile();
    if (file == NULL)
    {
        printf("FAIL: Could not create temporary file.\n");
        return 1;
    }

    passed &= marc_record_write(first, file) == MARC_SUCCESS;
    printf("%s: First record written\n", passed ? "PASS" : "FAIL");
    passed &= marc_record_write(second, file) == MARC_SUCCESS;
    printf("%s: Second record written\n", passed ? "PASS" : "FAIL");

    if (fflush(file) != 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        printf("FAIL: Could not rewind multi-record stream.\n");
        passed = 0;
    }

    if (marc_record_read(loaded_first, file) != MARC_SUCCESS)
    {
        printf("FAIL: First record could not be decoded.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: First record decoded\n");
        passed &= check_string("First record 001",
            marc_record_get_control_field(loaded_first, "001"),
            "000001");
        field = marc_record_get_field_by_tag(loaded_first, "245");
        subfield = field != NULL ? marc_field_get_subfield(field, 0) : NULL;
        passed &= check_string("First record 245 $a",
            subfield != NULL ? marc_subfield_get_value(subfield) : NULL,
            "First test record");
    }

    if (marc_record_read(loaded_second, file) != MARC_SUCCESS)
    {
        printf("FAIL: Second record could not be decoded.\n");
        passed = 0;
    }
    else
    {
        printf("PASS: Second record decoded\n");
        passed &= check_string("Second record 001",
            marc_record_get_control_field(loaded_second, "001"),
            "000002");
        field = marc_record_get_field_by_tag(loaded_second, "245");
        subfield = field != NULL ? marc_field_get_subfield(field, 0) : NULL;
        passed &= check_string("Second record 245 $a",
            subfield != NULL ? marc_subfield_get_value(subfield) : NULL,
            "Second test record");
    }

    passed &= marc_record_read(loaded_second, file) == MARC_ERROR_EOF;
    printf("%s: EOF reported after final record\n", passed ? "PASS" : "FAIL");

    fclose(file);
    marc_record_free(first);
    marc_record_free(second);
    marc_record_free(loaded_first);
    marc_record_free(loaded_second);

    if (!passed)
    {
        printf("Multi-record tests FAILED.\n");
        return 1;
    }

    printf("All multi-record tests passed!\n");
    return 0;
}
