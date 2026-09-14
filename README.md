<img width="305" height="66" alt="ViiewLib logo" src="https://github.com/user-attachments/assets/03c9e97a-c0ab-46e0-bcd5-0da25d0c03b6" />

ViiewLib is a lightweight C library providing reusable functionality
for MARC 21 and related library/information-science tooling.

## Current Status

Early development / experimental.

Current functionality:
- MARC 21 record handling
- MARC record parsing/encoding
- Unit tests

## Requirements

- GCC
- GNU Make
- MSYS2 UCRT64 (Windows)
- C11-compatible compiler

## Building

Clone the repository:

git clone <repository-url>
cd ViiewLib

Build the library:

make

## Running Tests

make test

## Build Outputs

The build produces:

`make` produces:

`libviiewlib.a` — static library

`make test` builds and runs the MARC test suite. The test tooling may also
produce `.mrc` files containing encoded MARC 21 records.

## Example

## Project Structure

include/    Public headers

src/        Library source

tests/      Test programs

Makefile    Build configuration

## License

MIT License
