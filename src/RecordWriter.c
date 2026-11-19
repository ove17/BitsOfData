// RecordWriter.c

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "BitsOfData.h"
#include "BitsOfDataTypes.h"

#include "MathUtils.h"

#include "RecordWriter.h"
#include "TemplateLanguage.h"
#include "WriteValues.h"
#include "RecordStore.h"
#include "DbaseEngine.h"


#define NOTHING_TO_SKIP 0xFF  // for traversing tokens

static uint8_t SkipFromTokenId = NOTHING_TO_SKIP;
static uint8_t SkipToTokenId = 0;

static BDB_recordFieldPropertiesT FieldProps;

static BDB_txtHandlerFunction GetTxtPtr = NULL;


BDB_txtHandlerFunction rw_getTxtPtrFn(void) {
    return GetTxtPtr;
}


void rw_setTxtPtrFn(BDB_txtHandlerFunction getTxtPtr) {
    GetTxtPtr = getTxtPtr;
}


/* TODO:
 * 1) parse format (without DB) -> tokens
 * 2) use tokens to generate recordString
 * 3) use tokens to generate RecordFieldProperties
 * 4) traverse FieldProperties while editing db record (via app / Maze)
 */


static uint8_t getMaxLengthFromTxtList(const uint8_t* const txtList,
                                       const uint8_t listSize) {
    uint8_t maxLength = 0;
    for (uint8_t i = 0; i < listSize; i++) {
        const char** dummyPtr = NULL;
        const uint8_t index = txtList[i];
        const uint8_t length = GetTxtPtr(dummyPtr, index);
        if (length > maxLength) {
            maxLength = length;
        }
    }
    return maxLength;
}


static void writeTxtListValue(const BDB_columnT *maxValueColDef,
                              const BDB_columnT *txtListColDef,
                              const uint16_t value) {
    assert(GetTxtPtr != NULL); // column relies on static text
    const uint8_t maxValue = (uint8_t)maxValueColDef->maxValue;
    const uint8_t maxLen = getMaxLengthFromTxtList(txtListColDef->txt.list,
                                                   maxValue);
    const char* textFromList = NULL;
    GetTxtPtr(&textFromList, txtListColDef->txt.list[value]);
    wv_writeTxt(textFromList, maxLen);
}


void rw_writeValue(const uint8_t tableId,
                       const uint8_t recordId,
                       const uint8_t columnId,
                       const tl_valuePropT properties) {
    const bool leading0 = properties.leading0;
    const uint16_t value = BDB_getValue(tableId, recordId, columnId);
    const BDB_columnT* columnDef = de_getColumnDef(tableId, recordId, columnId);
    switch(columnDef->colType) {

        case BDB_COLUMN_INTEGER : {
            uint16_t maxValue = columnDef->maxValue;
            if (properties.asHex) {
                wv_writeHex(value, maxValue);
            } else if (properties.asPercentage) {
                const uint16_t maxValue = columnDef->maxValue;
                wv_writeInteger((100 * value)/maxValue, 100, leading0);
            } else {
                const bool offset1 = properties.offset1;
                if (offset1) maxValue += 1;
                wv_writeInteger(value + offset1, maxValue, leading0);
            }
            break;
        }
        case BDB_COLUMN_DECIMAL : {
            const uint16_t maxValue = columnDef->maxValue;
            wv_writeDecimal(value, maxValue, columnDef->dec.shift, leading0);
            break;
        }

        case BDB_COLUMN_TXT_LIST : {
            writeTxtListValue(columnDef, columnDef, value);
            break;
        }
        case BDB_COLUMN_TXT_LIST_COPY : {
            // write value of the current column, using properties of another
            //  (table, record,) column
            const BDB_columnT* altColumnDef = de_getAltColumnDef(tableId,
                                                              recordId,
                                                              columnDef);
            writeTxtListValue(altColumnDef, altColumnDef, value);
            break;
        }
        case BDB_COLUMN_TXT_LIST_CLONE : {
            // write the value of another column using the properties of the
            //  current column
            const uint8_t valueColumn = columnDef->clone.valueColumn;
            const uint16_t newValue = BDB_getValue(tableId,
                                                   recordId,
                                                   valueColumn);
            const BDB_recordT* recordDef = de_getRecordDef(tableId, recordId);
            const BDB_columnT * maxValueColDef = &recordDef->columns[valueColumn];
            writeTxtListValue(maxValueColDef, columnDef, newValue);
            break;
        }

        case BDB_COLUMN_CHAR : {
            wv_writeChar((uint8_t)value, columnDef->chr.set);
            break;
        }
        case BDB_COLUMN_STRING : {
            for (uint8_t c = 0; c < columnDef->str.length; c++) {
                const uint8_t columnId = columnDef->str.firstChar + c;
                rw_writeValue(tableId, recordId, columnId, properties);
            }
            break;
        }

        case BDB_COLUMN_CHILD_TABLE : {
            const uint16_t maxNumRecords = de_getDbaseDef()->tables[value].maxNumRecords;
            const uint8_t numRecords = rs_getNumRecords((uint8_t)value);
            wv_writeInteger(numRecords, maxNumRecords, false);
            break;
        }
        case BDB_COLUMN_REFERENCE : {
            const uint8_t refTableId = columnDef->ref.tableId;
            const uint8_t refColumnId = columnDef->ref.columnId;
            if (refTableId == BDB_SELF_REFERENCE
                    || de_isRecordTypeColumn(refTableId, refColumnId)) {
                uint16_t maxValue = columnDef->maxValue;
                const bool offset1 = properties.offset1;
                if (offset1) maxValue += 1;
                wv_writeInteger(value + offset1,  maxValue, leading0);
            } else {
                const uint8_t refRecordId = (uint8_t)value;
                rw_writeValue(refTableId, refRecordId, refColumnId, properties);
            }
            break;
        }
        case BDB_COLUMN_VIRTUAL : {
            uint8_t virtTable = tableId;
            uint8_t virtRecord = recordId;
            uint8_t virtColumn = columnId;
            de_resolveVirtualColumn(&virtTable, &virtRecord, &virtColumn);
            rw_writeValue(virtTable, virtRecord, virtColumn, properties);
            break;
        }
        default :
            assert(0 && "Invalid column type");
    }
}


static void setBranchToSkip(uint8_t *tokenId,
                            const tl_tokenT token,
                            const bool condition) {
    if (condition) {
        // prepare for skipping false branch later
        SkipFromTokenId = token.lhs.elseIndex;
        SkipToTokenId = token.lhs.endIndex;
    } else {
        // skip true branch directly
        *tokenId = token.lhs.elseIndex - 1;
    }
}


static uint8_t increaseTokenId(uint8_t tokenId) {
    tokenId++;
    if (tokenId >= SkipFromTokenId) {
        tokenId = SkipToTokenId;
        SkipFromTokenId = NOTHING_TO_SKIP;
    }
    return tokenId;
}


static uint16_t evaluateTokenValue(const tl_tokenT *token,
                                   const uint8_t tableId,
                                   const uint8_t recordId) {
    const tl_valueTypeT valueType = token->valType;
    switch (valueType) {
        case tl_value_literal :
            return token->value;
        case tl_value_column_id :
            const uint8_t columnId = token->value;
            return BDB_getValue(tableId, recordId, columnId);
        case tl_value_record_id :
            return recordId;
        default :
            assert(0 && "invalid tokenValue type");
    }
    return 0;
}


static bool evaluateCondition(const uint8_t tableId,
                              const uint8_t recordId,
                              uint8_t *tokenId,
                              const tl_tokenT *tokens) {
    tl_tokenT token = tokens[*tokenId];
    tl_tokenT nextToken = tokens[*tokenId + 1];
    const uint8_t columnId = token.value;
    const uint16_t value = BDB_getValue(tableId, recordId, columnId);
    if (nextToken.type != tl_if_rhs) {
        return value;
    }
    *tokenId += 1; // skip rhs
    const ou_operatorT ifOperator = nextToken.rhs.ifOperator;
    const uint16_t number = evaluateTokenValue(&nextToken, tableId, recordId);
    return ou_evalCondition(value, ifOperator, number);
}


static void writeRecordID(const uint8_t tableId,
                          uint8_t recordId,
                          const tl_valuePropT properties) {
    const uint16_t maxNumRecords = de_getMaxNumRecords(tableId);
    if (properties.offset1) {
        recordId++;
    }
    wv_writeInteger(recordId, maxNumRecords, properties.leading0);
}


static void writeNumRecords(const uint8_t tableId,
                            const tl_valuePropT properties) {
    const uint16_t maxNumRecords = de_getMaxNumRecords(tableId);
    const uint8_t numRecords = rs_getNumRecords(tableId);
    wv_writeInteger(numRecords, maxNumRecords, properties.leading0);
}


static void writeTokenValue(const tl_tokenT *token,
                            const uint8_t tableId,
                            const uint8_t recordId) {
    const tl_valuePropT *properties = &token->properties;
    const uint8_t columnId = token->value;
    switch(token->valType) {
        case tl_value_column_id :
            rw_writeValue(tableId, recordId, columnId, *properties);
            break;
        case tl_value_record_id :
            writeRecordID(tableId, recordId, *properties);
            break;
        case tl_value_num_records :
            writeNumRecords(tableId, *properties);
            break;
        case tl_value_parent_column_id :
            const uint8_t recId = BDB_getParentRecord(tableId);
            rw_writeValue(de_getParentTableId(), recId, columnId, *properties);
            break;
        default :
            break;
    }
}


/* token interpreter
 * assumes token syntax is correct
 */
void rw_writeRecordWithFormat(const uint8_t tableId,
                               const uint8_t recordId,
                               const char* txtFormat) {
    tl_tokenT *tokens = tl_parseFormat(txtFormat);
    wv_setCursorPosition(0);
    uint8_t tokenId = 0;
    while (tokenId < tl_MAX_NUM_TOKENS) {
        const tl_tokenT token = tokens[tokenId];
        switch (token.type) {

            case tl_text :
                const uint8_t start = token.text.start;
                const uint8_t length = token.text.length;
                wv_writeTxt(txtFormat + start, length);
                break;

            case tl_value :
                if (!token.properties.isHidden) {
                    writeTokenValue(&token, tableId, recordId);
                }
                break;

            case tl_if_lhs : {
                const bool condition = evaluateCondition(tableId, recordId,
                                                         &tokenId, tokens);
                setBranchToSkip(&tokenId, token, condition);
                break;
            }
            case tl_invalid :
                tokenId = tl_MAX_NUM_TOKENS;
                break;

            default :
                assert(0 && "invalid token type");
        }
        tokenId = increaseTokenId(tokenId);
    }
    const uint8_t maxLen = de_getMaxStringBufferSize();
    while (wv_getCursorPosition() < maxLen) {
        wv_writeTxt(" ", 1); // pad with spaces
    }
}



/* This function:
 *  - counts number of fields
 *  - for each field it determines:
 *      - columnId
 *      - editStep
 *      - cursorPos (TODO)
 */
BDB_recordFieldPropertiesT *rw_getRecordFieldProperties(const uint8_t tableId,
                                                        const uint8_t recordId,
                                                        const char* txtFormat) {
    const tl_tokenT *tokens = tl_parseFormat(txtFormat);
    uint8_t editableFieldId = 0;
    uint8_t tokenId = 0;
    uint8_t cursorPos = 0;
    while (tokenId < tl_MAX_NUM_TOKENS) {
        tl_tokenT token = tokens[tokenId];
        BDB_fieldPropertiesT *fieldProps = &FieldProps.fields[editableFieldId];
        switch(token.type) {
            case tl_invalid :
                tokenId = tl_MAX_NUM_TOKENS;
                break;
            case tl_text :
                cursorPos += token.text.length;
                break;
            case tl_if_lhs : {
                const bool condition = evaluateCondition(tableId, recordId,
                                                         &tokenId, tokens);
                setBranchToSkip(&tokenId, token, condition);
                if (condition && !token.properties.isFixed) {
                    const uint8_t columnId = token.value;
                    fieldProps->columnId = columnId;
                    fieldProps->editStep = 1;
                    editableFieldId++;
                }
                break;
            }
            case tl_value : {
                if (token.valType != tl_value_column_id
                        || token.properties.isFixed) {
                    break;
                }
                const uint8_t columnId = token.value;
                const BDB_columnT* columnDef = de_getColumnDef(tableId,
                                                               recordId,
                                                               columnId);

                if (columnDef->colType == BDB_COLUMN_STRING) {
                    const uint8_t len = columnDef->str.length;
                    const uint8_t char1 = columnDef->str.firstChar;
                    for (uint8_t i = 0; i < len; i++) {
                        fieldProps->columnId = char1 + i;
                        fieldProps->editStep = 1;
                        fieldProps->cursorPos = cursorPos + i;
                        editableFieldId++;
                        fieldProps = &FieldProps.fields[editableFieldId];
                    }
                    break;
                }

                const uint8_t editPer = token.val.editPer;
                if (editPer == 0) {
                    fieldProps->columnId = columnId;
                    fieldProps->editStep = 1;
                    editableFieldId++;
                    break;
                }

                const uint16_t maxValue = columnDef->maxValue;
                const uint8_t numDigits = mu_getNumDigits(maxValue);
                const uint8_t numEditPos =
                            (editPer == 2) ? (numDigits + 1) / 2 : numDigits;
                uint16_t editStep = 1;
                uint8_t j = numEditPos;
                while (j > 0) {
                    j--;
                    fieldProps = &FieldProps.fields[editableFieldId + j];
                    fieldProps->columnId = columnId;
                    fieldProps->editStep = editStep;
                    editStep *= (editPer == 2) ? 100 : 10;
                }
                editableFieldId += numEditPos;
                break;
            }
            default :
                break;
        }
        tokenId = increaseTokenId(tokenId);
    }
    FieldProps.numFields = editableFieldId;
    return &FieldProps;
}
