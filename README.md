# BitsOfData

A minimal, space-efficient database designed for simple embedded systems.

The focus is on small memory footprint and low-level control rather than ease-of-use or framework integration.

## Features
- Minimal embedded database core
- Optimized for space efficiency
- Architecture-agnostic design
- Requires user-implemented storage drivers (e.g. EEPROM or other embedded storage devices)

## Architecture
This project provides a simple API to access a basic database with a fixed schema. 
Every table may consist of one or more records. Records can be inserted and deleted, up to a pre-defined maximum. The record definition (i.e. its columns) can be fixed for a table, but can also be variable. In that case the first column holds the record type.
Every record may consist of one or more columns. Every column has a column type and a set of properties.

The following column types have been implemented:
- integer (default)
- character - using a user-defined character set (subset of ascii)
- record type - allows multiple record definitions in one table
- reference - refers to a record in another table
- virtual - holds no data: returns a column value from a referenced table

The database schema is defined by the caller, using the prototypes in BitsOfDataTypes.h, see TestBistsOfData.cpp for usage examples.

## Prerequisites
The BDB library is low level and has very few prerequisites.
There is only one external dependency:
- The functions declarad in EeHw.h are platform dependent and must be defined by the user.
The only other requirements are:
- A BDB_dbaseDefT struct has to be populated with the desired database schema.
- AtxtHandler callback function has to be provided for supplying static text.

## Language
The embedded code is written in C and all test code is in C++, using CppuTest

## Testing
This project consists of two parts:
- Embedded library code
- Host-based unit tests for core logic validation

Unit tests are implemented using https://cpputest.github.io/ and run on the host system.
Tests validate core database logic and are not part of the embedded firmware build.

## Notes
This project is not intended as a plug-and-play library. It assumes familiarity with low-level embedded development and driver implementation.

## License
MIT License
