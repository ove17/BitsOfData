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
Every table may consist of one or more records. Records can be inserted and deleted, up to a pre-defined maximum. The record definition (i.e. its columns) can be fixed for one table, but can also be variable. In that case the first column holds the record type.
Every record may consist of one or more columns. Every column has a column type and a number of properties.

The following column types have been implemented:
- integer (default)
- record type - allows multiple record types in one table
- reference - refers to a record in another table
- virtual - holds no data: returns a column value from a referenced table

The database schema is defined using the prototypes in BitsOfDataTypes.h, see TestBistsOfData.cpp for usage examples.

## Language

The embedded code is written in C and all test code is in C++.

## Testing
This project consists of two parts:
- Embedded library code
- Host-based unit tests for core logic validation

Unit tests are implemented using CppUnitTest and run on the host system.
Tests validate core database logic and are not part of the embedded firmware build.

## Notes
This project is not intended as a plug-and-play library. It assumes familiarity with low-level embedded development and driver implementation.

## License
MIT License
