# ViiewLib Testing

ViiewLib includes a growing test suite covering MARC 21 record handling and ISO 2709 encoding and decoding.

The test suite is intended to verify both normal API behaviour and the library's handling of malformed or incomplete ISO 2709 records.

ViiewLib is still under active development, so the test suite does not currently represent complete MARC 21 or ISO 2709 conformance testing.

---

## Running the Tests

From the ViiewLib project directory, run:

```bash
make test
```

The main test program is compiled with:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic \
    -Iinclude \
    tests/test_marc.c \
    -L. -lviiewlib \
    -o tests/test_marc
```

and then executed with:

```bash
./tests/test_marc
```

Additional regression tests are maintained as individual C programs under `tests/`.

Generated executables, object files, static libraries, and `.mrc` test records are excluded from version control through `.gitignore`.

---

# Test Organisation

The current test suite consists of two groups.

### Core MARC 21 / ISO 2709 suite

Tests **1–18** are contained in:

```text
tests/test_marc.c
```

This program covers the core API, record creation, MARC field handling, ISO 2709 encoding/decoding, and round-trip integrity.

### Standalone regression tests

Tests **19–31** are individual C programs.

Each standalone test targets a specific edge case or malformed ISO 2709 condition.

This separation allows individual regression cases to be compiled and run independently while keeping the primary functional test suite straightforward.

---

# Core MARC 21 Tests

## 1. MARC 21 Record Creation

**Test file:** `tests/test_marc.c`

The core test verifies that a MARC record can be created and populated through the ViiewLib API.

Tested functionality includes:

* MARC record creation
* Field creation
* Field insertion
* Subfield creation
* Subfield insertion
* Field lookup
* Record field counting
* Record cleanup

---

## 2. Control Fields

**Test file:** `tests/test_marc.c`

Control fields are tested using standard MARC 21 control-field tags.

Currently tested:

* `001`
* `005`
* `008`

The tests verify that control fields can be created, populated, written to ISO 2709, and read back without losing their values.

---

## 3. Main Entry — 100

**Test file:** `tests/test_marc.c`

A `100` main-entry field is created with:

```text
$a
$d
```

The test verifies that both subfield codes and their values are preserved.

---

## 4. Title Statement — 245

**Test file:** `tests/test_marc.c`

A `245` title statement is created with:

```text
$a
$b
$c
```

The test verifies creation and correct subfield storage.

---

## 5. Publication Information — 264

**Test file:** `tests/test_marc.c`

A `264` publication field is created with:

```text
$a
$b
$c
```

The test verifies that the complete variable field structure is stored correctly.

---

## 6. Repeated Fields

**Test file:** `tests/test_marc.c`

Multiple `650` fields are created in the same record.

For example:

```text
650 $a Libraries
650 $a Metadata
```

The test verifies that:

* Multiple fields with the same tag can exist.
* Each field remains distinct.
* Field order is preserved.
* Field lookup returns the first matching field.

---

## 7. Special Characters

**Test file:** `tests/test_marc.c`

Fields containing special characters are created and checked.

The test ensures that ordinary character data survives MARC record handling without unintended modification.

---

## 8. Record Structure

**Test file:** `tests/test_marc.c`

The test verifies the number of fields in the original record.

The current core record contains:

```text
9 fields
```

This provides a baseline for later round-trip comparison.

---

## 9. ISO 2709 Write

**Test file:** `tests/test_marc.c`

The MARC record is encoded into an ISO 2709 record.

The writer generates:

1. Record leader
2. Directory
3. Variable fields
4. Field terminators
5. Record terminator

The generated record is written to:

```text
test.mrc
```

The test verifies that the record is successfully serialized.

---

## 10. ISO 2709 Read

**Test file:** `tests/test_marc.c`

The generated `test.mrc` record is read back using the ViiewLib ISO 2709 reader.

The test verifies successful decoding.

---

## 11. Loaded Record Structure

**Test file:** `tests/test_marc.c`

After decoding the ISO 2709 record, the test verifies that the resulting MARC record contains the expected number of fields.

This checks that the directory was interpreted correctly.

---

## 12. Control-Field Round Trip

**Test file:** `tests/test_marc.c`

The decoded record is checked against the original values for:

```text
001
005
008
```

This verifies control-field preservation across:

```text
MARC → ISO 2709 → MARC
```

---

## 13. 100 Round Trip

**Test file:** `tests/test_marc.c`

The decoded `100` field is checked for preservation of:

```text
$a
$d
```

Both subfield codes and values are compared.

---

## 14. 245 Round Trip

**Test file:** `tests/test_marc.c`

The decoded `245` field is checked for preservation of:

```text
$a
$b
$c
```

---

## 15. 264 Round Trip

**Test file:** `tests/test_marc.c`

The decoded `264` field is checked for preservation of:

```text
$a
$b
$c
```

---

## 16. Repeated-Field Round Trip

**Test file:** `tests/test_marc.c`

The repeated `650` fields are decoded and compared against the originals.

The test verifies that:

* Both fields remain present.
* Both retain tag `650`.
* Their order is preserved.
* Their individual `$a` values remain distinct.

---

## 17. Special-Character Round Trip

**Test file:** `tests/test_marc.c`

A field containing special characters is encoded and decoded.

The resulting value is compared against the original.

This verifies character preservation across the ISO 2709 serialization layer.

---

## 18. Field Lookup

**Test file:** `tests/test_marc.c`

The public field lookup functionality is tested using:

```text
245
001
999
```

The test verifies that:

* Existing `245` fields can be found.
* Existing `001` fields can be found.
* A nonexistent `999` field returns `NULL`.

---

# Standalone Regression Tests

## 19. Empty Record

**Test file:** `tests/test_empty.c`

An empty MARC record is created.

The test verifies:

* Empty record creation succeeds.
* The record contains zero fields.
* The ISO 2709 writer rejects the empty record.
* Cleanup succeeds.

The writer currently reports:

```text
ISO2709 WRITE ERROR: record contains no fields
```

This is expected behaviour.

---

## 20. Control-Fields-Only Record

**Test file:** `tests/test_control_only.c`

A record containing only:

```text
001
005
008
```

is created.

The test verifies that:

* Control fields can make up the entire record.
* The field count is correct.
* The record can be encoded.
* The record can be decoded.
* Control-field values survive the round trip.

---

## 21. Variable Field With Zero Subfields

**Test file:** `tests/test_empty_subfields.c`

A variable field such as:

```text
500
```

is created without any subfields.

The test verifies that the field can be serialized and decoded while retaining its zero-subfield structure.

---

## 22. Empty Subfield Value

**Test file:** `tests/test_empty_subfield_value.c`

A field containing an explicitly empty subfield value is created:

```text
500 $a ""
```

The test verifies that an empty value survives ISO 2709 encoding and decoding.

---

## 23. Repeated Subfields

**Test file:** `tests/test_repeated_subfields.c`

A `650` field containing repeated subfield codes is tested:

```text
650 $a Libraries
    $x Metadata
    $x Cataloguing
```

The test verifies:

* Multiple subfields in one field.
* Repeated subfield codes.
* Subfield order.
* Subfield values.
* Round-trip preservation.

---

## 24. Long Subfield Value

**Test file:** `tests/test_long_subfield.c`

A `500` field containing a 1,000-character `$a` value is created.

The generated value is compared byte-for-byte after ISO 2709 round-trip processing.

This verifies handling of substantially larger variable field data.

---

## 25. Many Subfields

**Test file:** `tests/test_many_subfields.c`

A single `650` field containing 20 subfields is tested.

The test includes repeated `$a`, `$b`, `$x`, `$y`, and `$z` values as well as `$6` and `$8`.

The test verifies:

* Large subfield counts.
* Repeated subfield codes.
* Ordering.
* Value preservation.
* ISO 2709 round-trip integrity.

---

## 26. Many Fields

**Test file:** `tests/test_many_fields.c`

A larger MARC record is generated containing:

* `001`
* `005`
* `008`
* 97 repeated `500` fields

for a total of:

```text
100 fields
```

The test verifies successful encoding and decoding of the larger record.

The current generated record is:

```text
Record length:    3716 bytes
Directory:        1201 bytes
Base address:     1225
Variable data:    2490 bytes
```

---

# ISO 2709 Malformed-Input Tests

## 27. Truncated Record

**Test file:** `tests/test_truncated.c`

The test uses the known-good record generated by:

```text
tests/test_many_fields.c
```

The 3,716-byte record is truncated by removing its final 20 bytes.

The reader must reject the incomplete record.

Expected result:

```text
ISO2709 READ ERROR: failed reading complete record
```

---

## 28. Incorrect Record Length

**Test file:** `tests/test_bad_record_length.c`

The test uses the known-good record generated by:

```text
tests/test_many_fields.c
```

The leader's record length is deliberately changed from:

```text
3716
```

to:

```text
3715
```

while the physical file remains 3,716 bytes.

The reader must reject the malformed record.

---

## 29. Invalid Base Address

**Test file:** `tests/test_bad_base_address.c`

The test uses the known-good record generated by:

```text
tests/test_many_fields.c
```

The leader's base address is changed from:

```text
1225
```

to:

```text
99999
```

The physical record remains otherwise intact.

The reader correctly rejects the invalid base address.

Expected diagnostic:

```text
ISO2709 READ ERROR: base address 99999 is invalid
```

---

## 30. Missing Record Terminator

**Test file:** `tests/test_missing_terminator.c`

The test uses the known-good record generated by:

```text
tests/test_many_fields.c
```

The final ISO 2709 record terminator:

```text
0x1D
```

is removed.

The resulting incomplete record is rejected by the reader.

---

## 31. Malformed Directory

**Test file:** `tests/test_bad_directory.c`

The test uses the known-good record generated by:

```text
tests/test_many_fields.c
```

The first directory entry originally contains:

```text
Tag:             001
Field length:    0007
Field position: 00000
```

The field length is corrupted to:

```text
9999
```

The reader detects that the resulting field would extend outside the valid variable-field area and rejects the record.

Expected diagnostic:

```text
ISO2709 READ ERROR: field 001 length is out of bounds
```

---

# Test Matrix

| Test | Test File                           | Area                         | Coverage                      |
| ---: | ----------------------------------- | ---------------------------- | ----------------------------- |
|    1 | `tests/test_marc.c`                 | Record creation              | Basic MARC record creation    |
|    2 | `tests/test_marc.c`                 | Control fields               | `001`, `005`, `008`           |
|    3 | `tests/test_marc.c`                 | Main entry                   | `100` with subfields          |
|    4 | `tests/test_marc.c`                 | Title                        | `245` with subfields          |
|    5 | `tests/test_marc.c`                 | Publication                  | `264` with subfields          |
|    6 | `tests/test_marc.c`                 | Repeated fields              | Multiple `650` fields         |
|    7 | `tests/test_marc.c`                 | Special characters           | Character preservation        |
|    8 | `tests/test_marc.c`                 | Record structure             | Field counting                |
|    9 | `tests/test_marc.c`                 | ISO 2709 write               | Record encoding               |
|   10 | `tests/test_marc.c`                 | ISO 2709 read                | Record decoding               |
|   11 | `tests/test_marc.c`                 | Loaded structure             | Decoded field structure       |
|   12 | `tests/test_marc.c`                 | Control round-trip           | Control-field preservation    |
|   13 | `tests/test_marc.c`                 | `100` round-trip             | Main-entry preservation       |
|   14 | `tests/test_marc.c`                 | `245` round-trip             | Title preservation            |
|   15 | `tests/test_marc.c`                 | `264` round-trip             | Publication preservation      |
|   16 | `tests/test_marc.c`                 | Repeated-field round-trip    | Repeated `650` preservation   |
|   17 | `tests/test_marc.c`                 | Special-character round-trip | Character preservation        |
|   18 | `tests/test_marc.c`                 | Field lookup                 | Existing/nonexistent fields   |
|   19 | `tests/test_empty.c`                | Empty record                 | Zero-field handling           |
|   20 | `tests/test_control_only.c`         | Control-only record          | Control-only MARC record      |
|   21 | `tests/test_empty_subfields.c`      | Empty variable field         | Zero-subfield field           |
|   22 | `tests/test_empty_subfield_value.c` | Empty subfield               | Empty value preservation      |
|   23 | `tests/test_repeated_subfields.c`   | Repeated subfields           | Repeated subfield codes       |
|   24 | `tests/test_long_subfield.c`        | Long subfield                | 1,000-character value         |
|   25 | `tests/test_many_subfields.c`       | Many subfields               | 20 subfields in one field     |
|   26 | `tests/test_many_fields.c`          | Many fields                  | 100-field record              |
|   27 | `tests/test_truncated.c`            | Truncated record             | Incomplete ISO 2709 input     |
|   28 | `tests/test_bad_record_length.c`    | Bad record length            | Leader/file mismatch          |
|   29 | `tests/test_bad_base_address.c`     | Bad base address             | Invalid leader offset         |
|   30 | `tests/test_missing_terminator.c`   | Missing terminator           | Missing `0x1D`                |
|   31 | `tests/test_bad_directory.c`        | Bad directory                | Out-of-bounds directory entry |

---

# Current Results

The current core suite passes all 18 tests contained in:

```text
tests/test_marc.c
```

The standalone regression tests covering tests 19–31 also pass.

Current single-record ISO 2709 robustness milestone:

**Complete.**

The current suite demonstrates successful handling of:

* MARC 21 record creation
* Control fields
* Variable fields
* Indicators
* Subfields
* Repeated fields
* Repeated subfields
* Empty values
* Long values
* Larger records
* ISO 2709 encoding
* ISO 2709 decoding
* ISO 2709 round trips
* Multiple malformed single-record conditions

Passing these tests does **not** imply complete MARC 21 or ISO 2709 standards conformance.

---

# Planned Tests

The next testing phase will extend ISO 2709 testing from individual records to files containing multiple records.

## 32. Two Records

A file containing two valid ISO 2709 records will be used to verify:

* First record decoding
* Second record decoding
* Correct stream position after the first record
* Correct stream position after the second record

---

## 33. Three Records

A three-record ISO 2709 file will provide an additional sequential-reading test.

The test will verify that each record can be decoded independently and in the correct order.

---

## 34. Sequential EOF Handling

The reader will be tested after the final record has been consumed.

The expected behaviour is a clean end-of-file result rather than an incorrectly classified malformed record.

---

## 35. Truncated Second Record

A multi-record file will contain:

```text
Valid record 1
Valid beginning of record 2
Truncated record 2
```

The test should verify that:

1. Record 1 is successfully decoded.
2. Record 2 is detected as incomplete.
3. The incomplete second record is rejected cleanly.

---

# Future Interoperability Testing

After multi-record support has been tested, ViiewLib should be tested against externally generated MARC 21 / ISO 2709 data.

Potential sources include:

* Library catalogue exports
* Library system-generated `.mrc` files
* Public MARC 21 sample data
* Records generated by established MARC software

External records are important because records generated by ViiewLib itself can share assumptions or bugs with the ViiewLib reader.

Future interoperability testing should therefore include externally generated records rather than relying exclusively on ViiewLib-generated `.mrc` files.

A typical interoperability test will follow:

```text
External MARC source
        │
        ▼
     .mrc file
        │
        ▼
ViiewLib ISO 2709 reader
        │
        ▼
MARC record structure
```

Where appropriate, ViiewLib-generated records should also be compared against records produced by established MARC software.

---

# Future Validation Areas

Additional testing may eventually cover:

* Full leader validation
* Leader status and implementation codes
* Directory edge cases
* Maximum record length
* Maximum field length
* Maximum field position
* Large directories
* Large numbers of repeated fields
* Additional control fields
* More MARC 21 field types
* Indicators containing unusual values
* Additional subfield edge cases
* Character encoding and MARC-8 considerations
* UTF-8 interoperability
* Multiple records per `.mrc` file
* EOF handling
* Invalid numeric directory values
* Invalid field positions
* Invalid field lengths
* Missing field terminators
* Invalid subfield delimiters
* Unexpected bytes inside variable fields
* Memory-allocation failures
* API misuse and `NULL` arguments
* Fuzz testing
* External `.mrc` interoperability

---

# Testing Philosophy

ViiewLib follows a layered testing approach.

### Layer 1 — API Behaviour

Verify that the public C API can create, manipulate, inspect, and free MARC records correctly.

### Layer 2 — Serialization

Verify that MARC records can be encoded into ISO 2709 and decoded back into equivalent MARC structures.

### Layer 3 — Boundary Conditions

Test empty values, repeated structures, long values, and larger records.

### Layer 4 — Malformed Input

Deliberately corrupt ISO 2709 records and verify that the reader rejects invalid structures.

### Layer 5 — Interoperability

Test ViiewLib against MARC data generated outside the library.

This approach is intended to make failures easier to isolate while gradually increasing confidence in the implementation.

---

# Development Status

ViiewLib is an experimental, actively developed C library.

The current test suite provides meaningful coverage of the implemented MARC 21 and ISO 2709 functionality, but it should not be interpreted as a complete conformance test suite for either standard.

Tests should be added alongside new functionality and bug fixes.

Regression tests should remain in the repository so that previously fixed issues are not accidentally reintroduced.

---

# Summary

Current status:

| Area                       | Status              |
| -------------------------- | ------------------- |
| MARC 21 record API tests   | PASS                |
| Control field tests        | PASS                |
| Variable field tests       | PASS                |
| Repeated field tests       | PASS                |
| Repeated subfield tests    | PASS                |
| Special-character tests    | PASS                |
| Large-value tests          | PASS                |
| Large-record tests         | PASS                |
| ISO 2709 encoding tests    | PASS                |
| ISO 2709 decoding tests    | PASS                |
| ISO 2709 round-trip tests  | PASS                |
| Malformed-record tests     | PASS                |
| Single-record robustness   | **COMPLETE**        |
| Multi-record testing       | PLANNED             |
| External interoperability  | PLANNED             |
| Full standards conformance | **NOT YET CLAIMED** |

ViiewLib's testing effort will continue to expand alongside the API and ISO 2709 implementation.
