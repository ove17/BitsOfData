// TestRecordTables.cpp

#include "CppUTest/TestHarness.h"

extern "C" {
    #include "EeHw.h"
    #include "EeHwX86.h"
    #include "RecordTables.h"
}


TEST_GROUP(RecordTables) {
    void setup() {
    }

    void teardown() {
    }
};

/*
 * TODO:
 *
 * setup git
 *
 * openRecordTablesListReturnsFalseIfItDoesNotExist
 * openRecordTablesListReturnsFalseIfItIsInvalid (test all edge cases?)
 * openRecordTablesListOnAValidListCopiesItToRamAndReturnsTrue
 *
 * storeRecordTablesListWritesItToEE (testable?)
 * deleteRecordTablesListRemovesAnExistingRecordTableFromEE (testable?)
 * closeRecordTablesListClearsItsRamCopy
 *
 * createTableOnAnEmptyRecordTableCreatesANewRecordTableAndReturns1
 * createTableOnAnExistingRecordTableCreatesANewRecordAndReturnsItsTableId
 * createTableOnAFullRecordTableDoesNothingAndReturns0
 * createTableWhenThereIsNoStorageSpaceLeftReturns0
 *
 * useTableOnAnEmptyRecordTableDoesNothingAndReturnsFalse
 * useTableOnAnIncompleteRecordTableDoesNohtingAndReturnsFalse
 * useTableReturnsFalseIfItsNumRecordsOrRecordSizeIsIncorrect
 * useTableOnAValidRecordTableAllocatesSpaceForReturnRecordAndReturnsTrue
 *
 * getNumRecordsOnATableWith0RecordsReturns0
 * getNumRecordsOnATableWith1RecordsReturns1
 * getNumRecordsOnATableWithMaxNumRecordsReturnsMax
 *
 * appendRecordOnAnEmptyTableCreatesTheFirstRecordAndReturnsTrue
 * appendRecordFailsIfTheMaxNumberOfRecordsWasReachedAndReturnsFalse
 * appendRecordCreatesANewRecordAfterTheLastOneAndReturnsTrue
 * appendRecordIncreasesTheNumberOfRecordsByOneAndReturnsTrue
 * appeldRecordWhenThereAreMaxMinus1RecordsReturnsTrue
 *
 * deleteRecordFailsIfThereIsOnlyOneRecordLeftAndReturnsFalse
 * deleteRecordFailsIfTheRecordIdDoesNotExistAndReturnsFalse
 * deleteRecordDeletesTheFirstRecordFromTheListAndReturnsTrue
 * deleteRecordDeletesTheLastRecordFromTheListAndReturnsTrue
 * deleteRecordDeletesTheMiddleRecordFromTheListAndReturnsTrue
 *
 * insertRecordAfterReturnsFalseIfTheMaxNumberOfRecordsWasReached
 * insertRecordAfterIncreasesTheNumberOfRecordsByOneAndReturnsTrue
 * insertRecordAfterTheFirstRecordReturnsTrue
 * insertRecordAfterTheLastRecordReturnsTrue
 * insertRecordAfterWhenThereAreMaxMinus1RecordsReturnsTrue
 *
 * setRecordFailsIfLengthDoesNotMatch
 * setRecordChangesValueOfRecordDataAndReturns1
 *
 * getRecordOnANewRecordReturnsAnArrayOf0xFF
 *
 * getRecordRetrievesExactlyWhatSetRecordWroteOnFullEeWrite
 *
 */


TEST(RecordTables, initDirectoryCreatesEmptyDirectoryIfOneDoesNotExist) {
    FAIL("hallo");
}

