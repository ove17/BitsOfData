/* conventions&style.h

 Conventions:
 ============

ONLY the public API is tested (and fully tested)

public API :
	functions:	 ACRONYM_camelCase()
	types:		 ACRONYM_camelCaseT
	variables:	 avoid at all cost
	defines:	 ACRONYM_UPPER_CASE
	enum values: ACRONYM_lower_case

internal api (module):
	functions:	 acronym_camelCase()
	types:		 acronym_camelCaseT
	variables:	 avoid at all cost
	defines:	 acronym_UPPER_CASE
	enum values: acronym_lower_case

static/local (file scope):
	functions:	 camelCase()
	types:		 camelCaseT
	variables:	 PascalCase	- note capitalisation!
	defines:	 UPPER_CASE
	enum values: lower_case

local (function scope):
	variables:	 camelCase

general:
	struct members: .camelCase

	BitsOfData/
	├── include/			// public API
	│   └── BitsOfData.h
	├── src/
	│   ├── BitsOfData.c
	│   ├── Cache.c
	│   ├── IndexList.c
	│   ├── RecordStore.c
	│   └── internal/		// internal headers, module-private
	│       ├── Cache.h
	│       ├── IndexList.h
	│       ├── RecordStore.h
	│       └── Utils.h
	├── tests/				// may replace external sources during tests
	│   └── mockRecord.c

 */


// no spaces for unary operators:
i++;
++i;

// use spaces around binary operators:
a = b + c;
if (x < y) { ... }
for (i = 0; i < n; i++) { ... }

// only exception: multiple inline initialisers:
	{.min=5, .max=234, .offset=93, ...},
	{.min=0, .max=12, .offset=0, ...},

// no space between function name and (
// no space after ( or before )
void function(void) {

// one line per function argument to allow comments if necessary
// for function declaration and definition
void function(const uint8* ptr,	// comment about argument (try to avoid)
			  const uint16 arg) {

// opening brace not on a line by itself:
	if (a == 6) {
		for (i = 0; i < max; i++) {
			...
		} else {
			...
		}
	}

// only exception: array of structs
	.array = [
		{
			...
		}, {
			...
		}
	],

// always use braces after if and for, even for a single line
	if (a == 6) {
		a++;
	}

// two blank lines between functions
// a single blank line may be used inside function for clarity or break between segments

// vertical alignment only if the lines are conceptually linked, so here:
#define RECORD_INDEX_OFFSET     0
#define RECORD_DATA_OFFSET      (1 * sizeof(eeAddress_t))
#define RECORD_SIZE_OFFSET      (2 * sizeof(eeAddress_t))
#define MAX_NUM_RECORDS_OFFSET  (2 * sizeof(eeAddress_t) + 1)
// but not for unrelated lines:
#define MIN_VALUE 5
#define PI_SHORT 3.14
