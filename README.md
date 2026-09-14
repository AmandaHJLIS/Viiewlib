<img width="305" height="66" alt="ViiewLib logo" src="https://github.com/user-attachments/assets/03c9e97a-c0ab-46e0-bcd5-0da25d0c03b6" />

## ViiewLib

ViiewLib is a lightweight, portable C library for working with MARC 21 records and ISO 2709-encoded bibliographic data. It is designed to provide a simple API with minimal dependencies for applications that need to read, manipulate, validate, and write MARC records.

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

This project is licensed under the MIT License. See the (LICENSE) file for details.
