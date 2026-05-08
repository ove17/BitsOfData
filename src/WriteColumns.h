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
                     uint8_t numDigits,
                     bool leading0);
void wc_writeDecimal(uint16_t value,
                     uint8_t numDigits,
                     uint8_t decimalShift);
void wc_writeChar(const char charIndex,
                  const char* charSet);


#endif
