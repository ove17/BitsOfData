// WriteColumns.h


#ifndef WRITE_COLUMNS_H
#define WRITE_COLUMNS_H

#include <stdint.h>
#include "BitsOfDataTypes.h"


void wc_initBuffer(const uint8_t size);
void wc_freeBuffer(void);
char* wc_getWriteBuffer(void);
void wc_setCursorPosition(const uint8_t position);
void wc_writeInteger(uint16_t value,
                     uint8_t numDigits,
                     const BDB_formatT* format);
void wc_writeChar(const char charIndex,
                  const char* charSet);


#endif
