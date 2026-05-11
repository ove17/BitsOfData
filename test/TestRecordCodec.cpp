/*
 * TestRecordCodec.cpp
 */

#include "CppUTest/TestHarness.h"
#include "cpputestUtils.h"

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


TEST(RecordCodec, getRecordSizeOnZeroByteReturns0) {
    static const BDB_columnT columns[] = {
        {.maxValue = 0}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = columns,
    };
    BYTES_EQUAL(0, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOnOneByteReturns1) {
    static const BDB_columnT columns[] = {
        {.maxValue = 255}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = columns,
    };
    BYTES_EQUAL(1, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOnTwoBytesReturns2) {
    static const BDB_columnT columns[] = {
        {.minValue = 10, .maxValue = 265},
        {.maxValue = 255}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = columns,
    };
    BYTES_EQUAL(2, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOn9bitsReturns2) {
    static const BDB_columnT columns[] = {
        {.maxValue = 256}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = columns,
    };
    BYTES_EQUAL(2, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOn3plus5bitsReturns1) {
    static const BDB_columnT columns[] = {
        {.minValue = 10, .maxValue = 17},
        {.maxValue = 31}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = columns,
    };
    BYTES_EQUAL(1, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOn4plus5bitsReturns2) {
    static const BDB_columnT columns[] = {
        {.minValue = 9, .maxValue = 17},
        {.maxValue = 31}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = columns,
    };
    BYTES_EQUAL(2, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, getRecordSizeOnDecimalRecordWithStepIsReduced) {
    static const BDB_columnT columns[] = {
        {.colType = BDB_COLUMN_DECIMAL, .minValue = 5, .maxValue = 1280, .decStep = 5},
    };
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = columns,
    };
    // expected: (1280 - 5)/5 = 255, so 1 byte
    BYTES_EQUAL(1, rc_getRecordSize(&recordDef));
}


TEST(RecordCodec, encodeRecordReturnsValueOnRecordWith8bitColumn) {
    uint8_t value = 100;
    const uint16_t recordData[1] = {value,};
    uint8_t rawRecord[1] = {0,};
    static const BDB_columnT columns[] = {
        {.maxValue = 255}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = columns,
    };
    static const BDB_recordT recordDefs[] = {recordDef};
    BDB_tableT tableDef = {
        .numRecordDefs = 1,
        .recordDefs = recordDefs,
    };

    rc_encodeRecord(recordData, rawRecord, &tableDef);
    BYTES_EQUAL(value, rawRecord[0]);
}


TEST(RecordCodec, encoding12bitsThenDecodingReturnsSame12bits) {
    uint16_t value = 2050; // 12 bits
    const uint16_t recordData[1] = {value,};
    uint8_t rawRecord[2] = {0,};
    static const BDB_columnT columns[] = {
        {.maxValue = 4000}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 1,
        .columns = columns,
    };
    static const BDB_recordT recordDefs[] = {recordDef};
    BDB_tableT tableDef = {
        .numRecordDefs = 1,
        .recordDefs = recordDefs,
    };

    rc_encodeRecord(recordData, rawRecord, &tableDef);
    rawRecord[1] &=  0b011110000;
    uint16_t recordDataOut[1] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut, &tableDef);
    LONGS_EQUAL(value, recordDataOut[0]);
}


TEST(RecordCodec, encodingMultipleColumnsThenDecodingReturnsSameColumnValues) {
    const uint16_t recordData[6] = {1, 2345, 9, 123, 4321, 431,};
    uint8_t rawRecord[7] = { 0 };
    static const BDB_columnT columns[] = {
        {.maxValue = 2},
        {.maxValue = 4000},
        {.maxValue = 19},
        {.maxValue = 259},
        {.maxValue = 9999},
        {.maxValue = 559},
    };
    static const BDB_recordT recordDef = {
        .numColumns = 6,
        .columns = columns,
    };
    static const BDB_recordT recordDefs[] = {recordDef};
    BDB_tableT tableDef = {
        .numRecordDefs = 1,
        .recordDefs = recordDefs,
    };

    rc_encodeRecord(recordData, rawRecord, &tableDef);
    rawRecord[6] &= 0b11110000; // wipe excess bits
    uint16_t recordDataOut[6] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut, &tableDef);
    CHECK_UINT16_ARRAY_EQUAL(recordData, recordDataOut, 6);
}


TEST(RecordCodec, encodingVariableRecordThenDecodingReturnsSame) {

    uint16_t recordData1[] = {0, 100};
    uint16_t recordData2[] = {1, 3000, 260};

    uint8_t rawRecord[4] = {0};

    static const BDB_columnT columns1[] = {
        {.colType = BDB_COLUMN_RECORD_TYPE, .maxValue = 1},
        {.maxValue = 123}
    };
    static const BDB_recordT recordDef1 = {
        .numColumns = 2,
        .columns = columns1,
    };
    static const BDB_columnT columns2[] = {
        {.colType = BDB_COLUMN_RECORD_TYPE, .maxValue = 1},
        {.maxValue = 4000},
        {.maxValue = 299}
    };
    static const BDB_recordT recordDef2 = {
        .numColumns = 3,
        .columns = columns2,
    };
    static const BDB_recordT recordDefs[] = {recordDef1, recordDef2};
    BDB_tableT tableDef = {
        .numRecordDefs = 2,
        .recordDefs = recordDefs,
    };

    rc_encodeRecord(recordData1, rawRecord, &tableDef);
    uint16_t recordDataOut1[2] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut1, &tableDef);
    LONGS_EQUAL(0, recordDataOut1[0]);
    LONGS_EQUAL(100, recordDataOut1[1]);

    rc_encodeRecord(recordData2, rawRecord, &tableDef);
    uint16_t recordDataOut2[3] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut2, &tableDef);
    LONGS_EQUAL(1, recordDataOut2[0]);
    LONGS_EQUAL(3000, recordDataOut2[1]);
    LONGS_EQUAL(260, recordDataOut2[2]);
}


TEST(RecordCodec, encodeRecordSkipsVirtualColumn) {
    uint8_t value1 = 0b01010101; // 0x55
    uint8_t value2 = 0b10101010; // 0xAA
    uint16_t recordData[3] = {value1, 0xFFFF, value2};

    uint8_t rawRecordTarget[3] = {0};
    rawRecordTarget[0] = value1;
    rawRecordTarget[1] = value2;
    static const BDB_columnT columns[] = {
        {.maxValue = 255},
        {.colType = BDB_COLUMN_VIRTUAL},
        {.maxValue = 255}
    };
    static const BDB_recordT recordDef = {
        .numColumns = 3,
        .columns = columns,
    };
    static const BDB_recordT recordDefs[] = {recordDef};
    BDB_tableT tableDef = {
        .numRecordDefs = 1,
        .recordDefs = recordDefs,
    };

    uint8_t rawRecord[3] = {0};
    rc_encodeRecord(recordData, rawRecord, &tableDef);
    MEMCMP_EQUAL(rawRecordTarget , rawRecord, 3);

    uint16_t recordDataOut[3] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut, &tableDef);
    CHECK_UINT16_ARRAY_EQUAL(recordData, recordDataOut, 3);
}


TEST(RecordCodec, encodingDecimalColumnsThenDecodingReturnsSameValue) {
    const uint16_t recordData[] = {995, 170};
    uint8_t rawRecord[3] = { 0 };
    static const BDB_columnT columns[] = {
        {.colType = BDB_COLUMN_DECIMAL, .minValue = 5, .maxValue = 1280, .decStep = 5},
        {.maxValue = 255},
    };
    static const BDB_recordT recordDef = {
        .numColumns = 2,
        .columns = columns,
    };
    static const BDB_recordT recordDefs[] = {recordDef};
    BDB_tableT tableDef = {
        .numRecordDefs = 1,
        .recordDefs = recordDefs,
    };
    rc_encodeRecord(recordData, rawRecord, &tableDef);
    rawRecord[2] = 0; // wipe excess bits to make sure record fits in 2 bytes
    uint16_t recordDataOut[2] = { 0 };
    rc_decodeRecord(rawRecord, recordDataOut, &tableDef);
    CHECK_UINT16_ARRAY_EQUAL(recordData, recordDataOut, 2);
}



