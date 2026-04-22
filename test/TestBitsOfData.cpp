// TestBitsOfData.cpp

/*
 * TDD approach:
 * 1)   implement explicit data model tests
 * 2)   implement pure API tests
 * 3)   rewrite explicit data model tests, using API functions where possible
 * 4)   remove tests from 1) that cannot fail independently from 2) or 3)
 * 5)   mark all remaining tests from 1) as "data model dependent"
 *
 */

#include "CppUTest/TestHarness.h"

extern "C" {
    #include "EeHw.h"
    #include "EeHwX86.h"
    #include "RecordStore.h"
    #include "BitsOfData.h"
}


TEST_GROUP(OpenDbase) {
    void setup() {
        eeClear();
    }

    void teardown() {
        rs_closeTableCatalog();
    }
};


TEST(OpenDbase, getNumRecordsReturns1onNewTable) {

    BYTES_EQUAL(1, BDB_getNumRecords(0));
}

