/* RecordWriter.h
 *
 * Contains functions to write a BDB record to a string
 */

#ifndef RECORD_WRITER_H
#define RECORD_WRITER_H

#include <stdint.h>
#include "BitsOfDataTypes.h"


void rw_writeRecord(const uint16_t* values,
                    const char* txtFormat,
                    const BDB_recordT* recordDef);

#endif
