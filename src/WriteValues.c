// WriteValues.c

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "MathUtils.h"


static char* StringBuffer = NULL;
static uint8_t Size = 0;
static uint8_t CursorPosition = 0;


static void increaseCursorPosition(void) {
    assert(CursorPosition < Size);
    CursorPosition++;
}


void wv_initBuffer(const uint8_t size) {
    Size = size;
    StringBuffer = calloc(size, sizeof(uint8_t));
}


void wv_freeBuffer(void) {
    if (StringBuffer) {
        free(StringBuffer);
    }
    StringBuffer = NULL;
    Size = 0;
    CursorPosition = 0;
}


char* wv_getWriteBuffer(void) {
    return StringBuffer;
}


void wv_setCursorPosition(const uint8_t position) {
    assert(position < Size);
    CursorPosition = position;
}


uint8_t wv_getCursorPosition(void) {
    return CursorPosition;
}


// writes in reverse order, truncates if value does not fit in numDigits
void wv_writeInteger(uint16_t value,
                     const uint16_t maxValue,
                     const bool leading0) {
    const uint8_t numDigits = mu_getNumDigits(maxValue);
    wv_setCursorPosition(CursorPosition + numDigits);
    char fillChar = leading0 ? '0' : ' ';
    for (uint8_t i = CursorPosition; i > CursorPosition - numDigits; i--) {
        if (value > 0) {
            const uint8_t digit = (uint8_t)(value % 10);
            StringBuffer[i - 1] = '0' + digit;
        } else if (i == CursorPosition) { // i.e. if inputvalue was 0
            StringBuffer[i - 1] = '0';
        } else {
            StringBuffer[i - 1] = fillChar;
        }
        value /= 10;
    }
}


void wv_writeHex(uint16_t value,
                 uint16_t maxValue) {
    uint8_t numDigits = mu_getHexNumDigits(maxValue);
    wv_setCursorPosition(CursorPosition + numDigits);
    for (uint16_t i = CursorPosition; i > CursorPosition - numDigits; i--) {
        const uint8_t digit = value & 0x0F;
        if (digit < 10) {
            StringBuffer[i - 1] = '0' + digit;
        } else {
            StringBuffer[i - 1] = 'A' + digit - 10;
        }
        value >>= 4;
    }
}


// writes in reverse order, truncates if value does not fit in numDigits
void wv_writeDecimal(uint16_t value,
                     const uint16_t maxValue,
                     const uint8_t decimalShift,
                     const bool leading0) {
    uint8_t numDigits = mu_getNumDigits(maxValue) + 1; // +1 for decimal point
    if (decimalShift >= numDigits) {
        numDigits = decimalShift + 2; // extra lead0's after pt
    }
    wv_setCursorPosition(CursorPosition + numDigits);
    char fillChar = '0';
    for (uint8_t i = 0; i < numDigits; i++) {
        const uint8_t position = CursorPosition - i - 1;
        if (decimalShift == i) {
            StringBuffer[position] = '.';
            continue;
        }
        if (value > 0) {
            const uint8_t digit = (uint8_t)(value % 10);
            StringBuffer[position] = '0' + digit;
        } else {
            if (i > decimalShift + 1) {
                fillChar = leading0 ? '0' : ' ';
            }
            StringBuffer[position] = fillChar;
        }
        value /= 10;
    }
}


void wv_writeTxt(const char* txt,
                 const uint8_t maxLength) {
    bool txtFinished = false;
    for (uint8_t i = 0; i < maxLength; i++) {
        if (txt[i] == '\0') txtFinished = true;
        StringBuffer[CursorPosition] = txtFinished ? ' ' : txt[i];
        increaseCursorPosition();
    }
}


void wv_writeChar(const uint8_t charIndex,
                  const char* charSet) {
    StringBuffer[CursorPosition] = charSet[charIndex];
    increaseCursorPosition();
}
