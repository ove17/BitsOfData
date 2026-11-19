/* WriteValues.h
 *
 * This is a private API from BitsOfData, it is NOT intended to be called
 *  directly or used stand-alone.
 *
 * Writes string representations of database columns to a string buffer.
 * It has no knowledge of the database or its schema.
 * The cursor position in the writeBuffer is advanced by all writing functions
 *  and persistant between calls. It must be reset to 0 explicitly by the
 *  caller.
 *
 * There is no error checking, except for an assert on the write index (cursor
 *  position) in the writeBuffer to guard against programming errors in using
 *  higher level text functions in BitsOfData.
 */

#ifndef WRITE_VALUES_H
#define WRITE_VALUES_H

#include <stdint.h>
#include <stdbool.h>


/*
 * Allocates the writeBuffer
 */
void wv_initBuffer(const uint8_t size);

/*
 * Frees the writeBuffer
 */
void wv_freeBuffer(void);

/*
 * Returns a pointer to the writeBuffer.
 * This buffer can also be written to by the caller.
 */
char* wv_getWriteBuffer(void);

/*
 * Sets the writeBuffer cursor position.
 */
void wv_setCursorPosition(const uint8_t position);

/*
 * Returns the current writeBuffer cursor position.
 */
uint8_t wv_getCursorPosition(void);

/*
 * Writes an integer value to the writeBuffer at the current cursor position.
 * The value is written right-aligned and filled on the left with spaces or
 *  zeros, depending on leading0.
 * If the integer does not fit in the same number of digits as maxValue, it will
 *  be truncated on the left.
 * On exit, the cursor position is at the next index.
 */
void wv_writeInteger(uint16_t value,
                     const uint16_t maxValue,
                     const bool leading0);

/*
 * Writes an integer value to the writeBuffer at the current cursor position.
 * The value is written in hexadecimal format, filling as much space as
 * maxValue would.
 * If the integer does not fit, it will be truncated on the left.
 * On exit, the cursor position is at the next index.
 */
void wv_writeHex(uint16_t value,
                 const uint16_t maxValue);

/*
 * Writes a decimal value to the writeBuffer at the current cursor position.
 * The value of decimalShift determines where the decimal point is placed,
 *  where 1 means it is shifted 1 digit to the left.
 * The value is written right-aligned and filled on the left with spaces.
 * If the number does not fit in numDigits, it will be truncated on the left.
 * On exit, the cursor position is at the next index.
 */
void wv_writeDecimal(uint16_t value,
                     const uint16_t maxValue,
                     const uint8_t decimalShift,
                     const bool leading0);

/*
 * Writes a character from a character set to the writebuffer at the current
 *  cursor position.
 * The value of charIndex determines which character is selected from the
 *  character set, which is a 0-terminated string.
 * On exit, the cursor position is at the next index.
 */
void wv_writeChar(const char charIndex,
                  const char* charSet);

/*
 * Writes length characters of a string to the writebuffer at the current
 *  cursor position.
 * If the string is shorter than length, it will be padded with spaces.
 * On exit, the cursor position is at the next index.
 */
void wv_writeTxt(const char* txt,
                 const uint8_t length);

#endif
