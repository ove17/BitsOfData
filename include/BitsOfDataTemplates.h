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
 * features. All BDB-TL tags are enclosed in curly braces '{' and '}'.
 * Tags cannot be nested.
 *
 * In the following, i and j denote integers that may contain multiple digits.
 * tags may have no more than 1 prefix. Note that ?& is considered a single
 * tag, which may have a prefix.
 *
 * tag          resolves to:
 * ---          ------------
 * {&i}         String representation of column i in the current record, right-
 *               aligned and padded on the left with spaces. By default the
 *               value is editable and visible. The cursor will appear on the
 *               last digit if the column is a number or on the first character
 *               if it is text.
 *          Allowed prefixes:
 *              o   left-padded with zeroes instead of spaces
 *              x   show as hexadecimal value instead of integer
 *              f   fixed value: cannot be changed / edited
 *              h   hidden: value is invisible, but can be edited, the cursor
 *                   will appear on the 1st character of the following item
 *          Allowed postfixes:
 *              +   offset value by +1
 *          Some pre- and postfixes may be combined:
 *                  f   with all but h
 *                  +   with all but h or x
 *                  any other combinations are invalid.
 *
 * {#}          Index of the current record, right-aligned and padded on the
 *               left with spaces
 *          Allowed prefixes:
 *              o   left-padded with zeroes instead of spaces
 *          Allowed postfixes:
 *              +   offset value by +1
 *          o and + may be combined.
 *
 * {*}          Total number of records in this table, right-aligned and padded
 *               on the left with spaces
 *          Allowed prefixes:
 *              o   left-padded with zeroes instead of spaces
 *
 * {^i}         String representation of column i in the parent table, record
 *               of the current table, no pre- or postfixes allowed
 *
 * {@} TODO        string representation of a custom value provided by the caller
 *                  through char* fmt and uint8* value? how? array necessary? single value?
 *
 * {?&i}        Starts a conditional expression, evaluates to false if the
 *               column value == 0, false otherwise. By default the column
 *               value is editable but hidden. The cursor will appear on the
 *               first character of the following item (= the true branch!).
 *              NOTE: nested if's are NOT allowed
 *          Allowed prefixes:
 *              f   fixed value: cannot be changed. NOTE format is {?f&i}
 *          Allowed postfixes:
 *              +   offset value by +1
 *
 * {?&i==RHS}   Starts a conditional expression, evaluates to true if the value
 *               of column i equals RHS, false otherwise. By default the column
 *               value is editable but hidden. The cursor will appear on the
 *               first character of the following item (= the true branch!).
 *              NOTE: nested if's are NOT allowed
 *          Allowed prefixes:
 *              f   fixed value: cannot be changed / edited
 *          Allowed postfixes:
 *              +   offset column value by +1
 *
 *          The following comparison operators are supported:
 *              ==  !=  >  >=  <  <=
 *
 *          RHS may be one of the following:
 *              j   a literal value
 *              &j  the value of another column
 *              #   the index of the current record
 *          Allowed postfixes:
 *              +   offset value by +1, not allowed for a literal value.
 *
 * {:}          Separates the true and false branches of a conditional
 *               expression (equivalent to 'else')
 *
 * {;}          Closes a conditional expression (equivalent to #ENDIF}
 *
 *
 * Examples:
 *
 * {o&i}        String representation of column i in the current record,
 *               right-aligned and padded on the left with zeros
 *
 * {&i+}        String representation of column i in the current record,
 *               offset by 1
 *
 * {x&i}        String representation of column i in the current record, in
 *               hexadecimal format, takes up the full width with leading 0's
 *
 * {o#}         Index of the current record, right-aligned and padded on the
 *               left with zeros
 *
 * {#+}         Index of the current record, offset by 1
 *
 * {o*}         Total number of records in this table, right-aligned and padded
 *               on the left with zeros
 *
 *     static const char format[] = " x={?&2==0}n/a{:}S{o&2}{;} ";
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
