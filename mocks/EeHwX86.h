/** @file  EeHwX86.h
 * Low-level hardware-dependent functions to read and write EEPROM
 */

#ifndef __ee_hw_x86_h__
#define __ee_hw_x86_h__

#include <stdint.h>

#define EEPROM_PAGE_SIZE 128    // in bytes
#define EEPROM_NUM_PAGES 64
#define EEPROM_SIZE (EEPROM_PAGE_SIZE * EEPROM_NUM_PAGES) // 64 * 128 = 8kB


void dumpFakeEeByte(uint16_t byteToDump);
void dumpFakeEe(const uint16_t startLine,
                const uint16_t numLines,
                const uint8_t bytesPerLine);
void dumpEeTable(const uint8_t tableId,
                 const uint8_t bytesPerRecord);

void assertEeAddressExists(const uint16_t eeAddress);
bool wasAddressValid(void);
void disableAssertAddress(void);
void enableAssertAddress(void);

#endif
