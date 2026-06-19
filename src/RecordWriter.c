// RecordWriter.c


#include <stdint.h>
#include "BitsOfDataTypes.h"


/*
 * It is the responsability of the caller to make sure values has the right
 *  length.
 */
void rw_writeRecord(const uint16_t* values,
                    const char* txtFormat,
                    const uint8_t fmtLen);
