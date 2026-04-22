/*
 * TestRecordCodec.cpp
 */

#include "CppUTest/TestHarness.h"

extern "C" {
    #include "BitsOfDataTypes.h"
    #include "RecordCodec.h"
}


TEST_GROUP(RecordCodec) {
    void setup() {
    }

    void teardown() {
    }
};



TEST(RecordCodec, getRecordSizeOnOneByteReturns1) {
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = {
            {.maxValue = 255}
        },
    };
    BYTES_EQUAL(1, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOnTwoBytesReturns2) {
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = {
            {.minValue = 10, .maxValue = 265},
            {.maxValue = 255}
        },
    };
    BYTES_EQUAL(2, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOn9bitsReturns2) {
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = {
            {.maxValue = 256}
        },
    };
    BYTES_EQUAL(2, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOn3plus5bitsReturns1) {
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = {
            {.minValue = 10, .maxValue = 17},
            {.maxValue = 31}
        },
    };
    BYTES_EQUAL(1, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOn4plus5bitsReturns2) {
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = {
            {.minValue = 9, .maxValue = 17},
            {.maxValue = 31}
        },
    };
    BYTES_EQUAL(2, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, encodeRecordReturnsValueOnRecordWith8bitColumn) {
    uint8_t value = 100;
    const uint16_t recordData[1] = {value,};
    uint8_t rawRecord[1] = {0,};
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = {
            {.maxValue = 255}
        },
    };
    rc_encodeRecord(recordData, rawRecord, &recordDef);
    BYTES_EQUAL(value, rawRecord[0]);
}


TEST(RecordCodec, encoding12bitsThenDecodingReturnsSame12bits) {
    uint16_t value = 2050; // 12 bits
    const uint16_t recordData[1] = {value,};
    uint8_t rawRecord[2] = {0,};
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = {
            {.maxValue = 4000}
        },
    };
    rc_encodeRecord(recordData, rawRecord, &recordDef);
    rawRecord[1] &=  0b011110000;
    uint16_t recordDataOut[1] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut, &recordDef);
    LONGS_EQUAL(value, recordDataOut[0]);
}


TEST(RecordCodec, encodingMultipleColumnsThenDecodingReturnsSameColumnValues) {
    const uint16_t recordData[6] = {1, 2345, 9, 123, 4321, 431,};
    uint8_t rawRecord[7] = { 0 };
    static const BDB_recordT recordDef = {
        .numColumns = 6,
        .columns = {
            {.maxValue = 2},
            {.maxValue = 4000},
            {.maxValue = 19},
            {.maxValue = 259},
            {.maxValue = 9999},
            {.maxValue = 559},
        },
    };
    rc_encodeRecord(recordData, rawRecord, &recordDef);
    rawRecord[6] &= 0b11110000; // wipe excess bits
    uint16_t recordDataOut[6] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut, &recordDef);
    MEMCMP_EQUAL(recordData, recordDataOut, 2*6);
}
