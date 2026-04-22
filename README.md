# BitsOfData

A minimal, space-efficient database designed for simple embedded systems.

The focus is on small memory footprint and low-level control rather than ease-of-use or framework integration.

## Features
- Minimal embedded database core
- Optimized for space efficiency
- Architecture-agnostic design
- Requires user-implemented storage drivers (e.g. EEPROM or other embedded storage devices)

## Architecture
This project provides a low-level RecordStore abstraction.
It is intentionally designed as a building block rather than a complete, standalone database system.
In addition, a RecordCodec is available for implementing columns in the records.

Higher-level abstractions can be built on top of this layer (e.g. structured data access layers), but they are not part of this repository and are not required for its use.

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
