/* EeHw.h
 * Low-level hardware-dependent functions to read and write EEPROM
 */

#ifndef __ee_hw_h__
#define __ee_hw_h__

#include <stdint.h>

#define EE_CLEAR_VALUE 0xFF

/// the type of the EE address can be changed to suit EE size
typedef uint16_t eeAddress_t;

void eeInit();
void eeClear();

void eeWriteUint8(const eeAddress_t eeAddress,
                  const uint8_t eeData);
void eeWriteUint16(const eeAddress_t eeAddress,
                   const uint16_t eeData);
void eeWriteUint8Array(const eeAddress_t startAddress,
                       const uint8_t *eeData,
                       const uint16_t numBytes);

uint8_t eeReadUint8(const eeAddress_t eeAddress);
uint16_t eeReadUint16(const eeAddress_t eeAddress);
void eeReadUint8Array(const eeAddress_t startAddress,
                      uint8_t *buffer,
                      const uint16_t numBytes);
void eeReadPage(const uint16_t page,
                uint8_t *buffer);

eeAddress_t eeGetPageSize();
eeAddress_t eeGetSize();

// NOTE: this is only used for testing, replaced with a dummy on target hardware
void assertEeAddressExists(const uint16_t eeAddress);

#endif
