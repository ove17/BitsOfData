// RecordWriter.h

#ifndef RECORD_WRITER_H
#define RECORD_WRITER_H


#include "BitsOfDataTypes.h"
#include "TemplateLanguage.h"


/* Stores pointer to the textHandler function in this module.
 */
void rw_setTxtPtrFn(BDB_txtHandlerFunction txtPtrFn);

/* Calls the stored textHandler function.
 */
BDB_txtHandlerFunction rw_getTxtPtrFn(void);

// writes column value to writeBuffer at the current cursorposition
void rw_writeValue(const uint8_t tableId,
                   const uint8_t recordId,
                   const uint8_t columnId,
                   const tl_valuePropT properties);

/* Writes the string representation of a database record to the write buffer.
 *
 * This function works identically to writeRecord, but instead of getting the
 *  format from the database schema, the user can provide a custom format. The
 *  caller is responsible for providing a \0 terminated string.
 */
void rw_writeRecordWithFormat(const uint8_t tableId,
                               const uint8_t recordId,
                               const char* txtFormat);

/* Returns a pointer to a BDB_recordFieldPropertiesT struct that contains the
 *  properties of all editable fields in txtFormat.
 * The result remains valid until the next call to this function with different
 *  arguments. The pointer will remain valid for the duration of the program.
 */
BDB_recordFieldPropertiesT *rw_getRecordFieldProperties(const uint8_t tableId,
                                                        const uint8_t recordId,
                                                        const char* txtFormat);

#endif
