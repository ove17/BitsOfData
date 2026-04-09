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

void eeWriteUint8(const eeAddress_t eeAddress, const uint8_t eeData);

void eeWriteUint16(const eeAddress_t eeAddress, const uint16_t eeData);

uint8_t eeReadUint8(const eeAddress_t eeAddress);

uint16_t eeReadUint16(const eeAddress_t eeAddress);

void eeReadUint8Array(const eeAddress_t startAddress,
                      uint8_t *buffer,
                      const uint16_t numBytes);

void eeReadPage(const uint16_t page,
                uint8_t *buffer);

void eeClear();

eeAddress_t eeGetPageSize();

eeAddress_t eeGetSize();

#endif
