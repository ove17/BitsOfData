// WriteColumns.h


#ifndef WRITE_COLUMNS_H
#define WRITE_COLUMNS_H

#include <stdint.h>
#include <stdbool.h>


void wc_initBuffer(const uint8_t size);
void wc_freeBuffer(void);

char* wc_getWriteBuffer(void);
void wc_setCursorPosition(const uint8_t position);

void wc_writeInteger(uint16_t value,
                     const uint8_t numDigits,
                     const bool leading0);
void wc_writeDecimal(uint16_t value,
                     const uint8_t numDigits,
                     const uint8_t decimalShift);
void wc_writeChar(const char charIndex,
                  const char* charSet);
void wc_writeIntZeroTxt(uint16_t value,
                        const uint8_t numDigits,
                        const char* txt);
void wc_writeTxt(const char* txt,
                 const uint8_t width);

#endif
