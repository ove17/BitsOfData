/** @file EeHwX86.c
 *  Low-level hardware-dependent functions to read and write EEPROM
 *
 * NOTE: EE page size is not guarded against in asserts! Depending on
 *      hw implemenation this may be necessary.
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "EeHw.h"
#include "EeHwX86.h"


static uint8_t _fakeEE[EEPROM_SIZE];


static bool AssertAddres = true;
static bool AddressValid = true;


void eeInit() {}


// real EE is clear by default
void eeClear() {
	for (eeAddress_t i = 0; i < EEPROM_SIZE; i++)
		_fakeEE[i] = EE_CLEAR_VALUE;
}


void eeWriteUint8(const eeAddress_t eeAddress,
                  const uint8_t eeData) {
    assertEeAddressExists(eeAddress);
    _fakeEE[eeAddress] = eeData;
}


void eeWriteUint16(const eeAddress_t eeAddress,
                   const uint16_t value) {
    assertEeAddressExists(eeAddress + 1);
    eeWriteUint8(eeAddress, (uint8_t)(value & 0x0FF));
    eeWriteUint8(eeAddress + 1, (uint8_t)(value >> 8));
}


void eeWriteUint8Array(const eeAddress_t eeAddress,
                       const uint8_t *eeData,
                       const uint16_t numBytes) {
    assertEeAddressExists(eeAddress + numBytes);
    for (uint16_t i=0; i<numBytes; i++) {
        eeWriteUint8(eeAddress + i, eeData[i]);
    }
}


uint8_t eeReadUint8(const eeAddress_t eeAddress) {
    assertEeAddressExists(eeAddress);
    return _fakeEE[eeAddress];
}


uint16_t eeReadUint16(const eeAddress_t eeAddress) {
    assertEeAddressExists(eeAddress + 1);
    return (uint16_t)(eeReadUint8(eeAddress + 1) << 8) + eeReadUint8(eeAddress);
}


void eeReadUint8Array(const eeAddress_t eeAddress,
                      uint8_t *buffer,
                      const uint16_t numBytes) {
    assertEeAddressExists(eeAddress + numBytes);
    for (uint16_t i=0; i<numBytes; i++) {
        buffer[i] = eeReadUint8(eeAddress + i);
    }
}


void eeReadPage(const eeAddress_t eeAddress,
                uint8_t *buffer) {
    assertEeAddressExists(eeAddress);
    eeReadUint8Array(eeAddress, buffer, EEPROM_PAGE_SIZE);
}


uint16_t eeGetPageSize() {
    return EEPROM_PAGE_SIZE;
}


eeAddress_t eeGetSize() {
    return EEPROM_SIZE;
}


/////  TESTING FUNCTIONS ONLY:


void dumpFakeEeByte(uint16_t byteToDump) {
    printf("\nEE byte[%i] = %i\n", byteToDump, eeReadUint8(byteToDump));
}


void dumpFakeEe(uint16_t startByte, uint16_t numLines, uint8_t bytesPerLine) {
    printf("\nStartByte = %i\n", startByte);
    printf("line");
    for(int i=0; i<bytesPerLine; i++) {
        printf("\tB%i", i);
    }
    printf("\n");
    for (int line=0; line<numLines; line++) {
        printf("%3i\t", line);
        for (int col=0; col<bytesPerLine; col++) {
            eeAddress_t addr = startByte + (eeAddress_t)(col+bytesPerLine*line);
            printf("%3i\t", eeReadUint8(addr));
        }
        printf("\n");
    }
    printf("\n");
}


void dumpEeTable(uint8_t tableId, uint8_t bytesPerRecord) {
    printf("\nrecord");
    for (int i=0; i < bytesPerRecord; i++) {
        printf("\tB%i", i);
    }
    printf("\n");

    eeAddress_t eeAddress = 2 * tableId - 1;
    int start = (eeReadUint8(eeAddress + 1) << 8) + eeReadUint8(eeAddress);
    eeAddress += 2;
    int end =  (eeReadUint8(eeAddress + 1) << 8) + eeReadUint8(eeAddress);
    int numRecords = (end - start) / bytesPerRecord;

    printf("n=%i\n", numRecords);

    for (int record = 0; record < numRecords; record++) {
        printf("%3i\t", record);
        for (int col = 0; col < bytesPerRecord; col++) {
            eeAddress_t addr = (eeAddress_t)(start + col + bytesPerRecord*record);
            printf("%3i\t", eeReadUint8(addr));
        }
        printf("\n");
    }
    printf("\n");
}


// This assert can be switched off for testing, using enable/disable below
void assertEeAddressExists(const uint16_t eeAddress) {
    AddressValid = (eeAddress < EEPROM_SIZE);
    if (AssertAddres) {
        assert(AddressValid);
    }
}


void disableAssertAddress() {
    AssertAddres = false;
}


void enableAssertAddress() {
    AssertAddres = true;
}


bool wasAddressValid() {
    return AddressValid;
}
