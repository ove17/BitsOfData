/* BitsOfDataTemplates.h
 *
 * Public API of BitsOfData:
 * A simple embedded database - the formatting template language
 *
 */


#ifndef BITS_OF_DATA_TEMPLATES_H
#define BITS_OF_DATA_TEMPLATES_H


/*
 * Each record type may define a format string. This format string determines
 * how record content is rendered as text. The BDB template language (BDB-TL)
 * is used to customise the output format.
 *
 * BDB-TL is intentionally minimal and provides only limited text-generation
 * features. All BDB-TL tags are enclosed in curly brackets '{' and '}'.
 * Tags cannot be nested.
 *
 * In the following, i and j denote integers that may contain multiple digits.
 *
 * tag             resolves to:
 * ---             ------------
 * {&i}            string representation of column i in the current record,
 *                 right-aligned and padded on the left with spaces
 *
 * {o&i}           string representation of column i in the current record,
 *                 right-aligned and padded on the left with zeros
 *
 * {&i+}           string representation of column i in the current record,
 *                  offset by 1
 *
 * {#}             index of the current record, {o#} and {#+} are allowed
 *
 * {*}             total number of records in this table
 *
 * {^i}            string representation of column i in the parent table, record
 *                  of the current table
 *
 * {@} TODO        string representation of a custom value provided by the caller
 *                  through char* fmt and uint8* value? how? array necessary? single value?
 *
 * {?&i==RHS}      begins a conditional expression:
 *                     true  if the value of column i equals RHS
 *                     false otherwise
 *                 NOTE: nested if's are NOT allowed
 *
 *                 The following comparison operators are supported:
 *                     ==  !=  >  >=  <  <=
 *
 *                 RHS may be one of the following:
 *                      $j  a literal value
 *                      &j  the value of another column
 *                      #   the index of the current record
 *
 * {:}             separates the true and false branches of a condition
 *                 (equivalent to 'else')
 *
 * {;}             ends a conditional expression (equivalent to #ENDIF}
 *
 *
 * Examples:
 *
 *     static const char format[] = " x={?&2==$0}n/a{:}S{o&2}{;} ";
 *     	renders as:
 *     		" x=n/a "   if column 2 == 0
 *     	or:
 *			" x=S04 "   if column 2 == 4
 *
 *     static const char format[] = "this is REC{o#+}";
 *     	may render as:
 *			"this is REC05"	if recordId == 4
 *
 */

#endif
