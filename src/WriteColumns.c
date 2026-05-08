// WriteColumns.c


#include <stdint.h>
#include <assert.h>
#include "BitsOfDataTypes.h"


static char* StringBuffer = NULL;
static uint8_t Size = 0;
static uint8_t CursorPosition = 0;

static void increaseCursorPosition(void);


void wc_initBuffer(const uint8_t size) {
    Size = size;
    StringBuffer = calloc(size, sizeof(uint8_t));
}


char* wc_getWriteBuffer(void) {
    return StringBuffer;
}


void wc_freeBuffer(void) {
    if (StringBuffer) {
        free(StringBuffer);
    }
    StringBuffer = NULL;
    Size = 0;
    CursorPosition = 0;
}


void wc_setCursorPosition(const uint8_t position) {
    assert(position < Size);
    CursorPosition = position;
}


// writes in reverse order
void wc_writeInteger(uint16_t value,
                     uint8_t numDigits,
                     const BDB_formatT* format) {
    wc_setCursorPosition(CursorPosition + numDigits);
    char fillChar = format->leading0 ? '0' : ' ';
    for (uint8_t i = CursorPosition ; i > CursorPosition - numDigits; i--) {
        uint8_t digit = (uint8_t)(value % 10);
        if (value > 0) {
            StringBuffer[i - 1] = '0' + digit;
        } else {
            StringBuffer[i - 1] = fillChar;
        }
        value /= 10;
    }
}


void wc_writeChar(const uint8_t charIndex,
                  const char* charSet) {
    StringBuffer[CursorPosition] = charSet[charIndex];
    increaseCursorPosition();
}


static void increaseCursorPosition(void) {
    CursorPosition++;
    if (CursorPosition >= Size) {
        CursorPosition = 0;
    }
}
