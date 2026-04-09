/* RecordTables.h
 *
 * This is an interface to access tables with records of bytes, e.g. in EE.
 *
 * Between 1 and 0xFE tables can be created/used.
 * Each table may have between 1 and 0xFE records.
 * Record size may be between 1 and 0xFE bytes.
 *
 * The database schema is static:
 *      The number of tables cannot be modified
 *      The (maximum) number of records cannot be modified
 *      Record size cannot be modified
 *
 * The entire record is returned as struct containing an array of bytes and its
 *   length, columns do not exist.
 *
 */

#ifndef __record_list_h__
#define __record_list_h__

#include <stdint.h>
#include <stdbool.h>


typedef struct {
    uint8_t length;
    uint8_t * rawRecord;
} rl_rawRecordT;


bool rl_openRecordTablesList(const uint8_t numTables);
/*
init i2c & ee,
allocates ram for numTables entries

als nt klopt (1 erna = 0xFF)
    & tabel klopt (optelling = correct & getallen lopen op)
        return true
*/

void rl_storeRecordTablesList();    // copy RAM to EE
void rl_deleteRecordTablesList();   // clear RAM & EE
void rl_closeRecordTablesList();    // de-allocate

uint8_t rl_createTable(uint8_t numRecords, uint8_t recordSize);
bool rl_useTable(const uint8_t tableId, uint8_t numRecords, uint8_t recordSize);

uint8_t rl_getNumRecords(const uint8_t tableId);

bool rl_appendRecord(const uint8_t tableId);
bool rl_deleteRecord(const uint8_t tableId, const uint8_t recordId);
bool rl_insertRecordAfter(const uint8_t tableId, const uint8_t recordId);

bool rl_setRecord(const uint8_t tableId, const uint8_t recordId, rl_rawRecordT rawRecord);
rl_rawRecordT rl_getRecord(const uint8_t tableId, const uint8_t recordId);

#endif
