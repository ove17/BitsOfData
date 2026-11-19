// TestWriteColumns.cpp

#include "CppUTest/TestHarness.h"

extern "C" {
    #include "WriteValues.h"
}


TEST_GROUP(WriteColumns) {
    void setup() {
        wv_initBuffer(21);
    }

    void teardown() {
        wv_freeBuffer();
    }
};



// wc_getCursorPosition

TEST(WriteColumns, getCursorPositionReturnsPosition) {
    wv_setCursorPosition(5);
    BYTES_EQUAL(5, wv_getCursorPosition());
}


TEST(WriteColumns, writeIntegerIncreasesCursorPosition) {
    wv_setCursorPosition(5);
    wv_writeInteger(0, 999, false);
    BYTES_EQUAL(5 + 3, wv_getCursorPosition());
}


// wc_writeInteger

TEST(WriteColumns, writeIntegerWithValue0writes0) {
    wv_writeInteger(0, 999, false);
    STRNCMP_EQUAL("  0\0", wv_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeIntegerToLessSpaceThanNeeded_TruncatesHighSideValue) {
    wv_writeInteger(1234, 999, false);
    STRNCMP_EQUAL("234\0", wv_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeIntegerToMoreSpaceThanNeeded_FillsWithSpaces) {
    wv_writeInteger(1234, 10000, false);
    STRNCMP_EQUAL(" 1234\0", wv_getWriteBuffer(), 6);
}


TEST(WriteColumns, writeIntegerWithLeading0_FillsWith0s) {
    wv_writeInteger(123, 10000, true);
    STRNCMP_EQUAL("00123\0", wv_getWriteBuffer(), 6);
}


TEST(WriteColumns, writeIntegerWithLeading0_FillsWith0andLeavesCursorAtEnd) {
    wv_writeInteger(412, 1234, true);
    STRNCMP_EQUAL("0412\0", wv_getWriteBuffer(), 6);
    BYTES_EQUAL(4, wv_getCursorPosition());
}


// wc_writeHex


TEST(WriteColumns, writeHexWithValue0AndMax15writes0) {
    wv_writeHex(0, 15);
    STRNCMP_EQUAL("0\0", wv_getWriteBuffer(), 3);
}


TEST(WriteColumns, writeHexWithValue0AndMax16writes00) {
    wv_writeHex(0, 16);
    STRNCMP_EQUAL("00\0", wv_getWriteBuffer(), 3);
}


TEST(WriteColumns, writeHexToLessSpaceThanNeeded_TruncatesHighSideValue) {
    wv_writeHex(0x123, 0xFF);
    STRNCMP_EQUAL("23\0", wv_getWriteBuffer(), 4);
}


TEST(WriteColumns, writeHexToMoreSpaceThanNeeded_FillsWithLeadingZeros) {
    wv_writeHex(0xFF, 0x100);
    STRNCMP_EQUAL("0FF\0 ", wv_getWriteBuffer(), 5);
}


// wc_writeDecimal


TEST(WriteColumns, writeDecimalIncreasesCursorPosition) {
    wv_setCursorPosition(8);
    wv_writeDecimal(0, 9999, 1, false);
    BYTES_EQUAL(8 + 5, wv_getCursorPosition());
}


TEST(WriteColumns, writeDecimalWrites0pt0ifValueIs0) {
    wv_writeDecimal(0, 1111, 1, false);
    STRNCMP_EQUAL("  0.0\0", wv_getWriteBuffer(), 5);
}


TEST(WriteColumns, writeDecimalShiftedBy1_IntroducesPoint) {
    wv_writeDecimal(123, 1111, 1, false);
    STRNCMP_EQUAL(" 12.3\0", wv_getWriteBuffer(), 5);
}


TEST(WriteColumns, writeDecimalWithLeading0) {
    wv_writeDecimal(123, 1111, 1, true);
    STRNCMP_EQUAL("012.3\0", wv_getWriteBuffer(), 5);
}


TEST(WriteColumns, writeDecimalShiftedBy3_IntroducesZeroAndPoint) {
    wv_writeDecimal(123, 11111, 3, false);
    STRNCMP_EQUAL(" 0.123\0", wv_getWriteBuffer(), 5);

}


TEST(WriteColumns, writeDecimalShiftedBy3_ToLessSpaceThanNeededLeavesOut0) {
    wv_writeDecimal(123, 111, 3, false);
    STRNCMP_EQUAL(".123\0", wv_getWriteBuffer(), 5);

}


TEST(WriteColumns, writeDecimalShiftedBy4_IntroducesZeroPointZero) {
    wv_writeDecimal(123, 11111, 4, false);
    STRNCMP_EQUAL("0.0123\0", wv_getWriteBuffer(), 5);

}


// wc_writeChar


TEST(WriteColumns, writeCharIncreasesCursorPositionBy1) {
    wv_setCursorPosition(6);
    wv_writeChar(2, "charSet");
    BYTES_EQUAL(6 + 1, wv_getCursorPosition());
}


TEST(WriteColumns, writeCharWritesASinleCharacterFromACharSet) {
    const char* charSet = "ABCD\0";
    wv_writeChar(2, charSet);
    STRNCMP_EQUAL("C\0", wv_getWriteBuffer(), 2);
}


TEST(WriteColumns, callingWriteCharTwiceWritesTwoConsecutiveCharacters) {
    const char* charSet = "ABCD\0";
    wv_writeChar(3, charSet);
    wv_writeChar(1, charSet);
    STRNCMP_EQUAL("DB\0", wv_getWriteBuffer(), 3);
}


// wc_writeTxt


TEST(WriteColumns, writeTxtIncreasesCursorPosition) {
    wv_setCursorPosition(6);
    wv_writeTxt("txt", 5);
    BYTES_EQUAL(6 + 5, wv_getCursorPosition());
}


TEST(WriteColumns, writeTxtWithLessTextThanSpaceFillsWithSpaces) {
    const char* txt = "no\0";
    wv_writeTxt(txt, 5);
    STRNCMP_EQUAL("no   \0", wv_getWriteBuffer(), 6);
}
