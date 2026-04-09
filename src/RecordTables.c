/* RecordTables.c
 *
 * This is an interface to access tables with records of bytes, e.g. in EE
 *
 * Each table has a recordList that contains the positions of its records.
 *      The index in the recordList is the index of the record, its value
 *          is the position of the record in the dataArea.
 *      Records are appended, inserted or deleted by editing the recordlist.
 *      The current number of records lives in recordList[MAX_NUM_RECORDS]
 *      MAX_NUM_RECORDS is a table property and must be > 0 and <= 255
 *      Unused records are set to 0xFF in the recordList
 *
 * Each table has a dataArea that contains its records
 *      Records are accessed by using the recordList to look up their position
 *          in the dataArea
 *
 * Records are returned as a pointer to an array of bytes.
 *      The size of each record is a fixed number of bytes
 *      Record size >= 1 byte and <= 255 bytes
 *      Each table has its own data return array
 *      The length of this array is the record size in bytes + 1
 *      The last byte of the record contains its position in the data area, i.e.
 *          its recordList value
 *
 *
 * A list is a one-dimensional array of bytes, its size must be > 0.
 * NOTE: the index of the first list is 1
 *
 * EEPROM starts with a directory of all defined/reserved lists, its format is:
 *
 * EE location          value
 * 0                    (uint8_t) number of directory entries
 * 1 , 2                (uint16_t) start of list 1
 * ...
 * (2*i)-1 , (2*i)      (uint16_t) start of list i
 * ...
 * (2*n)+1 , (2*n)+2    (uint16_t) end of list n
 * 2*n + 3              (uint8_t) index of last added directory entry
 *
 * Notes:
 *      - the number "i" in the above table is also the unique index that
 *          identifies each list
 *      - the number "n" in the above table is number of directory entries
 *          (lists) and its associated eeLocation is the end of the used EE space
 *      - existing lists cannot be resized or destroyed, other thay destroying
 *          all lists by generating a new directory (by overwriting it)
 *
 *  TODO: reconsider input checking:
 *          user can only use changeValue - so automatic limit checking
 *          import uses setValue - so also automatic limit checking
 *          so only source for invalid input = the programmer
 *      SOLUTION:
 *          - assert only at the db level (valid table, record, column)
 *              switch asserts off in production code
 *          - OR: (TDD) test with full db write + read back using rnd data
 *
 * FIXME: even edListExists() protects against programmer errors!
 *          assumption: EE is clear OR has complete dir, OR dir is being built
 *              so: dir exits but invalid MUST BE IMPOSSIBLE
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "RecordTables.h"
#include "EeHw.h"

/*
#define EEADDR_OF_NUM_DIR_ENTRIES 0
#define EEADDR_OF_THE_EEADDR_OF_THE_1ST_LIST 1
#define DIR_ENTRY_SIZE 2
#define MAX_DIR_SIZE 250

static uint8_t *_eeCache;


static inline eeAddress_t eePtrToEeAddrOfList(const uint8_t listIndex);
static inline eeAddress_t eeAddrOfDirEntryCounter(const uint8_t numLists);
static inline eeAddress_t eePtrToTheFirstList(const uint8_t numLists);

static void createDirectory(const uint8_t numLists,
                            const eeAddress_t eeSize);
static void setListStartAddress(const uint8_t dirIndex,
                                const eeAddress_t address);
static void setNextListStartAddress(const uint8_t dirIndex,
                                    const eeAddress_t address);
static eeAddress_t getListStartAddress(const uint8_t dirEntry);

static bool directoryIsValid(const uint8_t numLists);

static void setNumDirEntries(const uint8_t numEntries);
static uint8_t getDirEntryCounter(const uint8_t numEntries);
static void setDirEntryCounter(const uint8_t numEntries,
                               const uint8_t index);
static bool isDirEntryInvalid(const uint8_t dirIndex);
static bool isDirectoryTableComplete();
static uint8_t getNumLists();
static int8_t getNumDirEntries();

static void copyEeToCache(const uint16_t eeSize);
static uint8_t readUint8fromEeOrCache(const eeAddress_t eeAddress);
static void writeUint8ToEeOrCache(const eeAddress_t eeAddress,
                                  const uint8_t value);
static uint16_t readUint16fromEeOrCache(const eeAddress_t eeAddress);
static uint16_t getUint16FromCache(const eeAddress_t eeAddress);

*/

/* Initialises EE and creates a directory table if one does not exist
 * Returns true if a valid directory was found in EE
 *
bool edInitEeDirectory(const uint8_t numLists) {
    assert(numLists > 0);
    assert(numLists <= MAX_DIR_SIZE);
    eeInit();
    _eeCache = NULL;
    if (!directoryIsValid(numLists)) {
        eeClear();
        createDirectory(numLists, eeGetSize());
        return false;
    }
    return true;
}*/


/* Allocates RAM for cache of EeDirectory and fills it with current(!) values
 * Returns the number of lists found in the table
 * If the Directory Entry Table is invalid, the function fails and returns 0
 *
bool edUseCache() {
    uint8_t numDirEntries = getNumLists();
    if (!numDirEntries) {
        return false;
    }
    // only allocate the amount of RAM to mirror EE that is in use:
    uint16_t eeInUse = getListStartAddress(numDirEntries + 1);
    _eeCache = (uint8_t*)calloc(eeInUse, sizeof(uint8_t));
    copyEeToCache(eeInUse);
    return true;
}


bool edIsCacheEnabled() {
    return (bool)_eeCache;
}


// De-allocates cache, so that all list access goes straight to EE
void edReleaseCache() {
    if (_eeCache) {
        free(_eeCache);
    }
    _eeCache = NULL;
}


// copy cached value to EE location
bool edCopyCachedUint8ToEe(const uint8_t dirIndex,
                           const uint16_t listIndex) {
    if (_eeCache) {
        eeAddress_t eeAddress = getListStartAddress(dirIndex) + listIndex;
        uint8_t value = _eeCache[eeAddress];
        eeWriteUint8(eeAddress, value);
        return true;
    }
    return false;
}*/

/*
// low-effort way to force edDirectoryIsValid to fail
void edDestroy() {
    setNumDirEntries(0);
}*/


/* Removes directory table by completely erasing EE
void edRemoveDirectory() {
    eeClear();
}*/


/* Returns true if a directory exists and its number of lists is complete
 * and the EE address pointers appear valid
 *
static bool directoryIsValid(const uint8_t numLists) {
    uint8_t numDirEntries = getNumLists();
    if (numDirEntries == 0 || numDirEntries > MAX_DIR_SIZE || numDirEntries != numLists) {
        return false;
    }
    uint16_t address = getListStartAddress(1);
    for (uint8_t i=2; i <= numDirEntries + 1; i++) {
        uint16_t nextAddress = getListStartAddress(i);
        if (nextAddress <= address) {
            return false;
        }
        address = nextAddress;
    }
    return (address < eeGetSize());
}


// Returns true if the list index is valid and if all lists have been defined.
bool edListExists(const uint8_t dirIndex) {
    return !isDirEntryInvalid(dirIndex);
}


// Creates a new list in the EE directory table and returns its dirIndex
uint8_t edNewList(const uint16_t listSize) {
    if (listSize == 0 || isDirectoryTableComplete()) {
        return 0;
    }
    uint8_t numDirEntries = getNumDirEntries();
    uint8_t previousEntry = getDirEntryCounter(numDirEntries);
    uint8_t currentEntry = previousEntry + 1;
    if (currentEntry > MAX_DIR_SIZE) {
        return 0;
    }
    eeAddress_t currentListStartAddr = getListStartAddress(currentEntry);
    if (listSize > edGetFreeSpace()) {
        return 0;
    }
    setNextListStartAddress(currentEntry, currentListStartAddr + listSize);
    setDirEntryCounter(numDirEntries, currentEntry);
    return currentEntry;
}


// Returns the number of bytes in a list, either from EE or cache
uint16_t edGetListSize(const uint8_t dirIndex) {
    if (isDirEntryInvalid(dirIndex)) {
        return 0;
    }
    return getListStartAddress(dirIndex + 1) - getListStartAddress(dirIndex);
}*/


/* Write a value to a list in cache (if it exists) or EE.
 * NOTE: if it is written to cache, it is NOT written to EE!
 *
bool edSetListValue(const uint8_t dirIndex,
                    const uint16_t listIndex,
                    const uint8_t value) {
    if (listIndex >= edGetListSize(dirIndex)) { // implicitly validates dirIndex
        return false;
    }
    eeAddress_t eeAddress = getListStartAddress(dirIndex) + listIndex;
    writeUint8ToEeOrCache(eeAddress, value);
    return true;
}


// Returns a value from a list, either from EE or from cache
uint8_t edGetListValue(const uint8_t dirIndex,
                       const uint16_t listIndex) {
    if (listIndex >= edGetListSize(dirIndex)) { // implicitly validates dirIndex
        return 0;
    }
    eeAddress_t eeAddress = getListStartAddress(dirIndex) + listIndex;
    return readUint8fromEeOrCache(eeAddress);
}


// Returns the remaining number of bytes of free space for lists.
uint16_t edGetFreeSpace() {
    uint8_t numDirEntries = getNumDirEntries();
    uint8_t currentDirIndex = getDirEntryCounter(numDirEntries) + 1;
    uint16_t lastListStartAddress = getListStartAddress(currentDirIndex);
    return eeGetSize() - lastListStartAddress;
}


///////////////////


// Returns the EE address of the EE location of the first item of a list
static inline eeAddress_t eePtrToEeAddrOfList(const uint8_t listIndex) {
    return EEADDR_OF_THE_EEADDR_OF_THE_1ST_LIST + DIR_ENTRY_SIZE * (listIndex - 1);
}


// Returns the EE address of the counter that tracks the number of lists entered
static inline eeAddress_t eeAddrOfDirEntryCounter(const uint8_t numLists) {
    return eePtrToEeAddrOfList(numLists + 2);
}


// Returns the EE address of the EE location of the first item of the first list
static inline eeAddress_t eePtrToTheFirstList(const uint8_t numLists) {
    return eeAddrOfDirEntryCounter(numLists) + 1;
}


// Generate an empty directory, destroys anything that may exist in EEPROM
static void createDirectory(const uint8_t numLists,
                            const eeAddress_t eeSize) {
    setNumDirEntries(numLists);
    setListStartAddress(1, eePtrToTheFirstList(numLists));
    setDirEntryCounter(numLists, 0);
}


// Sets the start address in EE of the current list
static void setListStartAddress(const uint8_t dirIndex,
                                const eeAddress_t address) {
    eeWriteUint16(eePtrToEeAddrOfList(dirIndex), address);
}


// Sets the start address in EE of the next list
static void setNextListStartAddress(const uint8_t dirIndex,
                                    const eeAddress_t address) {
    setListStartAddress(dirIndex + 1, address);
}


// Returns the start (EE) address of a list, either from EE or from cache
static eeAddress_t getListStartAddress(const uint8_t dirIndex) {
    uint16_t eeAddress = EEADDR_OF_THE_EEADDR_OF_THE_1ST_LIST
                                            + DIR_ENTRY_SIZE * (dirIndex - 1);
    return readUint16fromEeOrCache(eeAddress);
}


// Sets the total number of lists in the directory table (its capacity)
static void setNumDirEntries(const uint8_t numEntries) {
    eeWriteUint8(EEADDR_OF_NUM_DIR_ENTRIES, numEntries);
}


// Sets the number of lists that have been entered in the directory table
static void setDirEntryCounter(const uint8_t numLists,
                               const uint8_t index) {
    eeWriteUint8(eeAddrOfDirEntryCounter(numLists), index);
}


// Returns the number of lists that have been entered in the directory table
static uint8_t getDirEntryCounter(const uint8_t numEntries) {
    uint16_t eeAddress = eeAddrOfDirEntryCounter(numEntries);
    return readUint8fromEeOrCache(eeAddress);
}*/


/* Returns true if dirIndex is invalid, or if the directory table is not
 * complete yet.
 *
static bool isDirEntryInvalid(const uint8_t dirIndex) {
    return (dirIndex == 0) || (dirIndex > getNumLists());
}


static bool isDirectoryTableComplete() {
    return (getNumLists() > 0);
}*/


/* Returns the number of lists in the directory IF they are all defined, so it
 * is an implicit directory validity test.
 *
static uint8_t getNumLists() {
    uint8_t numLists = getNumDirEntries();
    if (getDirEntryCounter(numLists) != numLists) {
        return 0;
    }
    return numLists;
}


// Returns the total number of lists in the directory table (its capacity)
// Does not take into account how many lists are actually registered
static int8_t getNumDirEntries() {
    return readUint8fromEeOrCache(EEADDR_OF_NUM_DIR_ENTRIES);
}


static void copyEeToCache(const uint16_t eeSize) {
    uint16_t pageSize = eeGetPageSize();
    uint16_t numPages = eeSize / pageSize;
    // copy all complete pages:
	for (uint16_t page = 0; page < numPages; page++) {
        eeAddress_t eeAddress = (uint16_t)(page << 7);
        eeReadPage(eeAddress, _eeCache + eeAddress);
	}
	// remaining incomplete page:
	uint16_t remainingBytes = eeSize % pageSize;
    if (remainingBytes) {
        eeAddress_t eeAddress = (uint16_t)(numPages << 7);
        eeReadUint8Array(eeAddress, _eeCache + eeAddress, remainingBytes);
    }
}


static uint8_t readUint8fromEeOrCache(const eeAddress_t eeAddress) {
    if (_eeCache) {
        return _eeCache[eeAddress];
    }
    return eeReadUint8(eeAddress);
}


static void writeUint8ToEeOrCache(const eeAddress_t eeAddress,
                                  const uint8_t value) {
    if (_eeCache) {
        _eeCache[eeAddress] = value;
    } else {
        eeWriteUint8(eeAddress, value);
    }
}


static uint16_t readUint16fromEeOrCache(const eeAddress_t eeAddress) {
    if (_eeCache) {
        return getUint16FromCache(eeAddress);
    }
    return eeReadUint16(eeAddress);
}


static uint16_t getUint16FromCache(const eeAddress_t eeAddress) {
    return _eeCache[eeAddress] + (uint16_t)(_eeCache[eeAddress + 1] <<  8);
}*/
