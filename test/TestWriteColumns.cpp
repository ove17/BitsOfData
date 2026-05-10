// TestWriteColumns.cpp

#include "CppUTest/TestHarness.h"

extern "C" {
    #include "WriteColumns.h"
}


TEST_GROUP(WriteColumns) {
    void setup() {
        wc_initBuffer(21);
    }

    void teardown() {
        wc_freeBuffer();
    }
};

/*
 * TODO:
 *
 * setCursorPos - does nothing yet
 * getCursorPos - is it needed?
 *
 */


// wc_writeInteger


TEST(WriteColumns, getCursorPositionReturnsPosition) {
    wc_setCursorPosition(5);
    BYTES_EQUAL(5, wc_getCursorPosition());
}


TEST(WriteColumns, writeIntegerIncreasesCursorPosition) {
    wc_setCursorPosition(5);
    wc_writeInteger(0, 3, false);
    BYTES_EQUAL(5 + 3, wc_getCursorPosition());
}


TEST(WriteColumns, writeIntegerWithValue0writes0) {
    wc_writeInteger(0, 3, false);
    STRNCMP_EQUAL("  0\0", wc_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeIntegerToLessSpaceThanNeeded_TruncatesHighSideValue) {
    wc_writeInteger(1234, 3, false);
    STRNCMP_EQUAL("234\0", wc_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeIntegerToMoreSpaceThanNeeded_FillsWithSpaces) {
    wc_writeInteger(1234, 5, false);
    STRNCMP_EQUAL(" 1234\0", wc_getWriteBuffer(), 6);
}


TEST(WriteColumns, writeIntegerWithLeading0_FillsWith0s) {
    wc_writeInteger(123, 5, true);
    STRNCMP_EQUAL("00123\0", wc_getWriteBuffer(), 6);
}


// wc_writeDecimal


TEST(WriteColumns, writeDecimalIncreasesCursorPosition) {
    wc_setCursorPosition(8);
    wc_writeDecimal(0, 5, 1);
    BYTES_EQUAL(8 + 5, wc_getCursorPosition());
}


TEST(WriteColumns, writeDecimalWrites0pt0ifValueIs0) {
    wc_writeDecimal(0, 5, 1);
    STRNCMP_EQUAL("  0.0\0", wc_getWriteBuffer(), 5);
}


TEST(WriteColumns, writeDecimalShiftedBy1_IntroducesPoint) {
    wc_writeDecimal(123, 5, 1);
    STRNCMP_EQUAL(" 12.3\0", wc_getWriteBuffer(), 5);
}


TEST(WriteColumns, writeDecimalShiftedBy3_IntroducesZeroAndPoint) {
    wc_writeDecimal(123, 6, 3);
    STRNCMP_EQUAL(" 0.123\0", wc_getWriteBuffer(), 5);

}


TEST(WriteColumns, writeDecimalShiftedBy3_ToLessSpaceThanNeededLeavesOut0) {
    wc_writeDecimal(123, 4, 3);
    STRNCMP_EQUAL(".123\0", wc_getWriteBuffer(), 5);

}


TEST(WriteColumns, writeDecimalShiftedBy4_IntroducesZeroPointZero) {
    wc_writeDecimal(123, 7, 4);
    STRNCMP_EQUAL(" 0.0123\0", wc_getWriteBuffer(), 5);

}


// wc_writeIntZeroTxt


TEST(WriteColumns, writeIntZeroTxtIncreasesCursorPosition) {
    wc_setCursorPosition(2);
    wc_writeIntZeroTxt(1, 3, "txt");
    BYTES_EQUAL(2 + 3, wc_getCursorPosition());
}


TEST(WriteColumns, writeIntZeroTxtWritesIntIfValueIs1) {
    const char* txt = "no\0";
    wc_writeIntZeroTxt(1, 3, txt);
    STRNCMP_EQUAL("  1\0", wc_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeIntZeroTxtWritesStrIfValueIs0) {
    const char* txt = "no\0";
    wc_writeIntZeroTxt(0, 2, txt);
    STRNCMP_EQUAL("no\0", wc_getWriteBuffer(), 3);
}


TEST(WriteColumns, writeIntZeroTxtIsTruncated) {
    const char* txt = "nono\0";
    wc_writeIntZeroTxt(0, 3, txt);
    STRNCMP_EQUAL("non\0", wc_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeIntZeroTxtIsLeftAligned) {
    const char* txt = "no\0";
    wc_writeIntZeroTxt(0, 3, txt);
    STRNCMP_EQUAL("no \0", wc_getWriteBuffer(), 4);
}


// wc_writeChar


TEST(WriteColumns, writeCharIncreasesCursorPositionBy1) {
    wc_setCursorPosition(6);
    wc_writeChar(2, "charSet");
    BYTES_EQUAL(6 + 1, wc_getCursorPosition());
}


TEST(WriteColumns, writeCharWritesASinleCharacterFromACharSet) {
    const char* charSet = "ABCD\0";
    wc_writeChar(2, charSet);
    STRNCMP_EQUAL("C\0", wc_getWriteBuffer(), 2);
}


TEST(WriteColumns, callingWriteCharTwiceWritesTwoConsecutiveCharacters) {
    const char* charSet = "ABCD\0";
    wc_writeChar(3, charSet);
    wc_writeChar(1, charSet);
    STRNCMP_EQUAL("DB\0", wc_getWriteBuffer(), 3);
}


// wc_writeTxt


TEST(WriteColumns, writeTxtIncreasesCursorPosition) {
    wc_setCursorPosition(6);
    wc_writeTxt("txt", 5);
    BYTES_EQUAL(6 + 5, wc_getCursorPosition());
}


TEST(WriteColumns, writeTxtWithLessTextThanSpaceFillsWithSpaces) {
    const char* txt = "no\0";
    wc_writeTxt(txt, 5);
    STRNCMP_EQUAL("no   \0", wc_getWriteBuffer(), 6);
}
