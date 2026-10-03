#include <stdio.h>
#include <stdlib.h>
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

    file = tmpfile();
    if (file == NULL)
    {
        printf("FAIL: Could not create truncated-record fixture.\\n");
        marc_record_free(record);
        return 1;
    }

    /*
     * The leader is complete, but the declared record length is larger
     * than the bytes available in the stream. This is truncation rather
     * than malformed structure because the input ends prematurely.
     */
    fputs("00030nam a2200024   4500", file);
    rewind(file);

    passed &= expect_result(
        "Truncated record body reported as truncated",
        marc_record_read(record, file),
        MARC_ERROR_TRUNCATED
    );
    fclose(file);

    file = tmpfile();
    if (file == NULL)
    {
        printf("FAIL: Could not create malformed-base-address fixture.\\n");
        marc_record_free(record);
        return 1;
    }

    /*
     * A complete leader with a non-numeric base address is malformed
     * leader metadata, not a truncated stream.
     */
    fputs("00024nam a2200abc   4500", file);
    rewind(file);

    passed &= expect_result(
        "Malformed base address reported as malformed",
        marc_record_read(record, file),
        MARC_ERROR_MALFORMED
    );
    fclose(file);

    /*
     * Read-state behaviour: a malformed later field may leave earlier
     * decoded fields in the destination record.
     */
    {
        MARC_Record *source = marc_record_create();
        MARC_Record *loaded = marc_record_create();
        MARC_Field *first = NULL;
        MARC_Field *second = NULL;
        unsigned char *raw = NULL;
        long raw_size;
        size_t raw_length;
        size_t last_field_terminator = 0;
        int found_terminator = 0;

        if (source == NULL || loaded == NULL)
        {
            printf("FAIL: Could not create partial-read test records.\n");
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        first = marc_field_create("245", '1', '0');
        second = marc_field_create("500", ' ', ' ');
        if (first == NULL || second == NULL)
        {
            printf("FAIL: Could not create partial-read test fields.\n");
            marc_field_free(first);
            marc_field_free(second);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        if (marc_field_add_subfield(first, 'a', "First field") != MARC_SUCCESS ||
            marc_field_add_subfield(second, 'a', "Second field") != MARC_SUCCESS ||
            marc_record_add_field(source, first) != MARC_SUCCESS ||
            marc_record_add_field(source, second) != MARC_SUCCESS)
        {
            printf("FAIL: Could not construct partial-read test record.\n");
            /*
             * Successful additions transfer ownership, so only free the
             * standalone fields here.
             */
            if (marc_record_get_field_count(source) == 0)
            {
                marc_field_free(first);
                marc_field_free(second);
            }
            else if (marc_record_get_field_count(source) == 1)
            {
                marc_field_free(second);
            }
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        file = tmpfile();
        if (file == NULL)
        {
            printf("FAIL: Could not create partial-read fixture.\n");
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        if (marc_record_write(source, file) != MARC_SUCCESS)
        {
            printf("FAIL: Could not write partial-read fixture.\n");
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        if (fseek(file, 0, SEEK_END) != 0)
        {
            printf("FAIL: Could not seek partial-read fixture.\n");
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        raw_size = ftell(file);
        if (raw_size <= 0)
        {
            printf("FAIL: Could not size partial-read fixture.\n");
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        raw_length = (size_t)raw_size;
        raw = malloc(raw_length);
        if (raw == NULL)
        {
            printf("FAIL: Could not allocate partial-read fixture buffer.\n");
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        rewind(file);

        if (fread(raw, 1, raw_length, file) != raw_length)
        {
            printf("FAIL: Could not read partial-read fixture.\n");
            free(raw);
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        /*
         * Replace the final field terminator before the record terminator.
         * The first field remains structurally valid, while the second
         * field must be rejected when decoded.
         */
        for (size_t i = raw_length - 1; i > 0; --i)
        {
            if (raw[i - 1] == 0x1E)
            {
                last_field_terminator = i - 1;
                found_terminator = 1;
                break;
            }
        }

        if (!found_terminator)
        {
            printf("FAIL: Could not locate second field terminator.\n");
            free(raw);
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        raw[last_field_terminator] = 'X';

        rewind(file);
        if (fwrite(raw, 1, raw_length, file) != raw_length)
        {
            printf("FAIL: Could not rewrite partial-read fixture.\n");
            free(raw);
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        if (fflush(file) != 0)
        {
            printf("FAIL: Could not flush partial-read fixture.\n");
            free(raw);
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        rewind(file);

        passed &= expect_result(
            "Malformed later field reports malformed",
            marc_record_read(loaded, file),
            MARC_ERROR_MALFORMED
        );

        passed &= expect_result(
            "Earlier valid field remains after failed read",
            (marc_record_get_field_count(loaded) == 1 &&
             strcmp(
                 marc_subfield_get_value(
                     marc_field_get_subfield(
                         marc_record_get_field(loaded, 0),
                         0
                     )
                 ),
                 "First field"
             ) == 0)
                ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
            MARC_SUCCESS
        );

        free(raw);
        fclose(file);
        marc_record_free(source);
        marc_record_free(loaded);
    }

    /*
     * A failed read does not clear fields that were already present in the
     * destination record.
     */
    {
        MARC_Record *source = marc_record_create();
        MARC_Record *loaded = marc_record_create();
        MARC_Field *source_field = NULL;
        MARC_Field *existing_field = NULL;

        if (source == NULL || loaded == NULL)
        {
            printf("FAIL: Could not create preservation test records.\n");
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        source_field = marc_field_create("245", '1', '0');
        existing_field = marc_field_create("500", ' ', ' ');

        if (source_field == NULL || existing_field == NULL ||
            marc_field_add_subfield(source_field, 'a', "Loaded") != MARC_SUCCESS ||
            marc_field_add_subfield(existing_field, 'a', "Existing") != MARC_SUCCESS ||
            marc_record_add_field(source, source_field) != MARC_SUCCESS ||
            marc_record_add_field(loaded, existing_field) != MARC_SUCCESS)
        {
            printf("FAIL: Could not construct preservation test records.\n");
            if (source_field != NULL &&
                marc_record_get_field_count(source) == 0)
            {
                marc_field_free(source_field);
            }
            if (existing_field != NULL &&
                marc_record_get_field_count(loaded) == 0)
            {
                marc_field_free(existing_field);
            }
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        file = tmpfile();
        if (file == NULL)
        {
            printf("FAIL: Could not create preservation fixture.\n");
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        if (marc_record_write(source, file) != MARC_SUCCESS)
        {
            printf("FAIL: Could not write preservation fixture.\n");
            fclose(file);
            marc_record_free(source);
            marc_record_free(loaded);
            return 1;
        }

        rewind(file);

        passed &= expect_result(
            "Successful read appends to existing record",
            marc_record_read(loaded, file),
            MARC_SUCCESS
        );

        passed &= expect_result(
            "Existing field preserved when reading",
            (marc_record_get_field_count(loaded) == 2 &&
             strcmp(
                 marc_field_get_tag(
                     marc_record_get_field(loaded, 0)
                 ),
                 "500"
             ) == 0)
                ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
            MARC_SUCCESS
        );

        passed &= expect_result(
            "Decoded field appended after existing field",
            (strcmp(
                 marc_field_get_tag(
                     marc_record_get_field(loaded, 1)
                 ),
                 "245"
             ) == 0)
                ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
            MARC_SUCCESS
        );

        fclose(file);
        marc_record_free(source);
        marc_record_free(loaded);
    }

    marc_record_free(record);

    /*
     * Ownership transfer: once a field is added successfully, the record
     * owns it and will release it when the record is freed.
     */
    record = marc_record_create();
    field = marc_field_create("245", '1', '0');

    if (record == NULL || field == NULL)
    {
        printf("FAIL: Could not create ownership test objects.\n");
        marc_field_free(field);
        marc_record_free(record);
        return 1;
    }

    passed &= expect_result(
        "Field ownership transferred on successful add",
        marc_record_add_field(record, field),
        MARC_SUCCESS
    );

    passed &= expect_result(
        "Transferred field remains accessible through record",
        (marc_record_get_field(record, 0) == field)
            ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
        MARC_SUCCESS
    );

    passed &= expect_result(
        "Transferred field retains subfield state",
        (marc_field_add_subfield(field, 'a', "Ownership test") == MARC_SUCCESS &&
         marc_field_get_subfield_count(field) == 1 &&
         strcmp(marc_subfield_get_value(
             marc_field_get_subfield(field, 0)
         ), "Ownership test") == 0)
            ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
        MARC_SUCCESS
    );

    /*
     * The record owns field after successful insertion. Freeing the record
     * therefore also releases the field and its subfields.
     */
    marc_record_free(record);

    /*
     * Standalone subfields remain caller-owned until explicitly freed.
     */
    {
        MARC_Subfield *standalone = marc_subfield_create(
            'a',
            "Standalone ownership"
        );

        if (standalone == NULL)
        {
            printf("FAIL: Could not create standalone subfield.\n");
            return 1;
        }

        passed &= expect_result(
            "Standalone subfield created successfully",
            (strcmp(
                marc_subfield_get_value(standalone),
                "Standalone ownership"
            ) == 0)
                ? MARC_SUCCESS : MARC_ERROR_INVALID_ARGUMENT,
            MARC_SUCCESS
        );

        marc_subfield_free(standalone);
    }

    printf("\n==========================\n");

    if (!passed)
    {
        printf("API error tests FAILED.\n");
        return 1;
    }

    printf("All API error tests passed!\n");
    return 0;
}
