<img width="305" height="66" alt="ViiewLib logo" src="https://github.com/user-attachments/assets/03c9e97a-c0ab-46e0-bcd5-0da25d0c03b6" />

# ViiewLib

ViiewLib is a lightweight, portable C library for working with MARC 21 records and ISO 2709-encoded bibliographic data. It is designed to provide a simple API with minimal dependencies for applications that need to read, manipulate, validate, and write MARC records.

## Current Status

**Early-stage functional / active development.**

ViiewLib is a lightweight C API for working with MARC 21 records and ISO 2709 data.

Current functionality includes:

* MARC 21 record creation and manipulation
* Control field support (`001`, `005`, `008`, etc.)
* Variable field support
* Indicators and subfields
* Repeated field support with preserved field order
* Field lookup
* MARC 21 record parsing
* ISO 2709 encoding and decoding
* ISO 2709 record writing and reading
* MARC 21 ↔ ISO 2709 round-trip support
* Validation and rejection of malformed ISO 2709 records
* Regression and robustness testing
* Basic API usage examples
* ISO 2709 API usage examples

The current test suite covers record creation, control fields, variable fields, repeated fields, special characters, field lookup, ISO 2709 encoding and decoding, round-trip integrity, large field/subfield values, and malformed or truncated ISO 2709 input.

The library currently builds and tests successfully with a standard C11 toolchain. Portability testing with other toolchains and platforms, including devkitPPC/Wii, is planned.

ViiewLib remains under active development. API design, validation, error handling, documentation, standards coverage, interoperability, and additional MARC 21 / ISO 2709 functionality will continue to evolve.

ViiewLib does **not** currently claim complete MARC 21 or ISO 2709 standards conformance or production readiness.

## Requirements

* GCC
* GNU Make
* MSYS2 UCRT64 (Windows)
* C11-compatible compiler

## Building

Clone the repository:

```bash
git clone <repository-url>
cd ViiewLib
```

Build the library:

```bash
make
```

The default build produces the static library:

```text
libviiewlib.a
```

## Running Tests

Run the main test suite with:

```bash
make test
```

The core test suite currently covers MARC 21 record creation, field handling, ISO 2709 encoding and decoding, and MARC ↔ ISO 2709 round-trip behaviour.

Additional standalone regression tests cover edge cases and malformed ISO 2709 records.

Generated test executables and `.mrc` files are excluded from version control through `.gitignore`.

## Testing Documentation

Detailed information about the ViiewLib test suite is available in:

**[TESTINGDOCUMENTATION.md](TESTINGDOCUMENTATION.md)**

The testing documentation includes:

* Test organisation
* Individual test descriptions
* Test source files
* MARC 21 API coverage
* ISO 2709 encoding and decoding coverage
* Round-trip testing
* Boundary-condition testing
* Malformed ISO 2709 input testing
* Current test results
* Planned tests
* Future interoperability testing

The current testing milestone is:

**Single-record ISO 2709 robustness testing: complete.**

Planned testing includes multi-record ISO 2709 files, sequential record reading, EOF handling, truncated multi-record files, and interoperability with externally generated MARC data.

## Build Outputs

The main build produces:

```text
libviiewlib.a
```

The test suite may additionally generate:

```text
tests/test_marc.exe
*.mrc
```

These generated files are ignored by Git and are not intended to be committed to the repository.

## Examples

The `examples/` directory contains small programs demonstrating
how to use ViiewLib.

### Basic MARC 21 usage

`examples/basic_marc.c` demonstrates basic record, field, and
subfield creation and access.

### ISO 2709 encoding and decoding

`examples/marc_iso2709.c` demonstrates writing a MARC 21 record
to ISO 2709 format and reading it back into a `MARC_Record`.

The examples are intentionally small and are intended to serve
as both API demonstrations and starting points for applications
using ViiewLib.

Example usage documentation and API examples will be expanded as the public API stabilises.

## Project Structure

```text
ViiewLib/
├── include/
│   └── viiewlib/
│       ├── marc.h
├── source/
│   ├── marc_field.c
│   ├── marc_iso2709.c
│   ├── marc_record.c
│   ├── marc_subfield.c
├── tests/
│   ├── test_marc.c
│   ├── test_empty.c
│   ├── test_control_only.c
│   ├── test_empty_subfields.c
│   ├── test_empty_subfield_value.c
│   ├── test_repeated_subfields.c
│   ├── test_long_subfield.c
│   ├── test_many_subfields.c
│   ├── test_many_fields.c
│   ├── test_truncated.c
│   ├── test_bad_record_length.c
│   ├── test_bad_base_address.c
│   ├── test_missing_terminator.c
│   └── test_bad_directory.c
├── Makefile
├── TESTING.md
├── LICENSE
└── README.md
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
