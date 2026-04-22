/*
 * BitsOfData.c
 *
 */

#include "BitsOfData.h"
#include <stdint.h>
#include <stdbool.h>


/*
 * record dBaseDef ptr
 * if !TryToOpen()
 *		createTable() // numRecords, recordSize komen uit tableDefsList
 * 		...
 * 		commitTables()
 * 		setDefaultRecord() ?
 */
void BDB_openDataBase(const BDB_dbaseDefT* dbaseDef) {

}

uint8_t BDB_getNumRecords(const uint8_t table) {
    return 1;
}




/*
 *
 s t*atic inline uint8_t getNumRecordsIn(BDB_tableT* table) {
 return sizeof(table->recordTypes / sizeof(table->recordTypes[0]));
 }

 static inline bool recordhasVariableType(dbTable* table) {
 return dbGetNumRecordsIn(table) > 1;
 }

 static uint8_t getRecordType(const uint8_t table,
 const uint16_t record) {
 getValue(table, record, 0);
 }
 */
