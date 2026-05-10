// WriteColumns.c


#include <stdbool.h>
#include <stdint.h>
#include <assert.h>


static char* StringBuffer = NULL;
static uint8_t Size = 0;
static uint8_t CursorPosition = 0;


static void increaseCursorPosition(void) {
    CursorPosition++;
    if (CursorPosition >= Size) {
        CursorPosition = 0;
    }
}


// api:


void wc_initBuffer(const uint8_t size) {
    Size = size;
    StringBuffer = calloc(size, sizeof(uint8_t));
}


void wc_freeBuffer(void) {
    if (StringBuffer) {
        free(StringBuffer);
    }
    StringBuffer = NULL;
    Size = 0;
    CursorPosition = 0;
}


char* wc_getWriteBuffer(void) {
    return StringBuffer;
}


void wc_setCursorPosition(const uint8_t position) {
    assert(position < Size);
    CursorPosition = position;
}


uint8_t wc_getCursorPosition(void) {
    return CursorPosition;
}


// writes in reverse order, truncates if value does not fit in numDigits
void wc_writeInteger(uint16_t value,
                     const uint8_t numDigits,
                     const bool leading0) {
    wc_setCursorPosition(CursorPosition + numDigits);
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


// writes in reverse order, truncates if value does not fit in numDigits
void wc_writeDecimal(uint16_t value,
                     const uint8_t numDigits,
                     const uint8_t decimalShift) {
    wc_setCursorPosition(CursorPosition + numDigits);
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
            char fillChar = (i > decimalShift + 1) ? ' ' : '0';
            StringBuffer[position] = fillChar;
        }
        value /= 10;
    }
}


void wc_writeTxt(const char* txt,
                 const uint8_t maxLength) {
    bool txtFinished = false;
    for (uint8_t i = 0; i < maxLength; i++) {
        if (txt[i] == '\0') txtFinished = true;
        StringBuffer[CursorPosition] = txtFinished ? ' ' : txt[i];
        increaseCursorPosition();
    }
}


void wc_writeIntZeroTxt(uint16_t value,
                        const uint8_t numDigits,
                        const char* txt) {
    if (value == 0) {
        wc_writeTxt(txt, numDigits);
        return;
    }
    wc_setCursorPosition(CursorPosition + numDigits);
    for (uint8_t i = CursorPosition; i > CursorPosition - numDigits; i--) {
        if (value > 0) {
            const uint8_t digit = (uint8_t)(value % 10);
            StringBuffer[i - 1] = '0' + digit;
        } else {
            StringBuffer[i - 1] = ' ';
        }
        value /= 10;
    }
}


void wc_writeChar(const uint8_t charIndex,
                  const char* charSet) {
    StringBuffer[CursorPosition] = charSet[charIndex];
    increaseCursorPosition();
}
