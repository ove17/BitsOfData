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
 * setCursorPos
 * getCursorPos?
 *
 */


// wc_writeInteger

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

