// DbaseEngine.h
/*
 * TODO: document functions
 *
 */

#ifndef DBASE_ENGINE_H
#define DBASE_ENGINE_H


#include <stdint.h>
#include <stdbool.h>

#include "BitsOfDataTypes.h"


void de_setDbaseDef(const BDB_dbaseDefT *dbaseDef);

const BDB_dbaseDefT* de_getDbaseDef(void);
const BDB_tableT *de_getTableDef(const uint8_t tableId);
const BDB_recordT* de_getRecordDef(const uint8_t tableId,
                                   const uint8_t recordId);
const BDB_columnT* de_getColumnDef(const uint8_t tableId,
                                   const uint8_t recordId,
                                   const uint8_t columnId);
const BDB_columnT* de_getAltColumnDef(const uint8_t tableId,
                                      const uint8_t recordId,
                                      const BDB_columnT* columnDef);

uint8_t de_getNumTables(void);
uint8_t de_getMaxStringBufferSize(void);
uint8_t de_getMaxNumRecords(const uint8_t tableId);
uint8_t de_getNumRecordDefs(const uint8_t tableId);

bool de_hasVariableRecordType(const uint8_t tableId);
bool de_isRecordTypeColumn(const uint8_t tableId,
                           const uint8_t columnId);

uint8_t de_getHeaderFormatString(const uint8_t tableId);
uint8_t de_getRecordFormatString(const uint8_t tableId,
                                 const uint8_t recordId);

void de_storeRecordBuffer(const uint8_t tableId);
void de_writeRecordBufferIfDirty(const uint8_t tableId);

bool de_validateColumn(const uint8_t tableId,
                       const uint8_t recordId,
                       const BDB_columnT* columnDef,
                       const uint16_t value);

const uint16_t* de_getRecord(const uint8_t tableId,
                             const uint8_t recordId);

uint16_t de_getMaxValue(const uint8_t tableId,
                        const uint8_t recordId,
                        const BDB_columnT* columnDef);

void de_setValue(const uint8_t tableId,
                 const uint8_t recordId,
                 const uint8_t columnId,
                 const uint16_t value);

/*
 * Returns a value from the database as a uint16_t, regardless of the
 * 	recordType (documented in BitsOfDataTypes.h)
 *
 * Interpretation of the value depends on the schema definition.
 */
uint16_t de_getValue(const uint8_t tableId,
                     const uint8_t recordId,
                     const uint8_t columnId);

/*
 * NOTE: the reference column is in the same table,record as the virtual
 *  column. This is why getColumnDef is called twice with the same recordDef,
 *  just with a different column. This is also why the .recordId output comes
 *  straight from RecordBuffers[tableId].columns[refColumId]
 */
void de_resolveVirtualColumn(uint8_t *tableId,
                             uint8_t *recordId,
                             uint8_t *columnId);


void de_allocateRecordBuffers(void);
void de_createFirstRecordInEveryTable(void);
void de_freeRecordBuffers(void);
void de_clearRecordBuffer(const uint8_t tableId);

void de_allocateParentRecordList(void);
void de_buildParentRecordList(void);
void de_freeParentRecordLists(void);

uint8_t de_getParentTableId(void);
uint8_t de_getParentChildColumnId(void);
uint8_t de_getParentRecordId(const uint8_t tableOffset);

void de_setRecordToDefaultValues(const uint8_t tableId,
                                 const uint8_t recordId);

bool de_copyAndValidateRecordDataToBuf(const uint8_t tableId,
                                       const uint8_t recordId,
                                       const uint16_t *data);

bool de_validateRecord(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint16_t* data);

#endif
