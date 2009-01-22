#ifndef _TYPES_H_
#define _TYPES_H_

/* 
   The following is a selective portion of the m68k.h file from the
   gcc-2.0 distribution. I didn't want to include the whole file in this
   library compilation.
 */


/* target machine storage layout */

/* Define this if most significant bit is lowest numbered
   in instructions that operate on numbered bit-fields.
   This is true for 68020 insns such as bfins and bfexts.
   We make it true always by avoiding using the single-bit insns
   except in special cases with constant bit numbers.  */
#define BITS_BIG_ENDIAN 1

/* Define this if most significant byte of a word is the lowest numbered.  */
/* That is true on the 68000.  */
#define BYTES_BIG_ENDIAN 1

/* Define this if most significant word of a multiword number is the lowest
   numbered.  */
/* For 68000 we can decide arbitrarily
   since there are no machine instructions for them.
   So let's be consistent.  */
#define WORDS_BIG_ENDIAN 1

/* number of bits in an addressible storage unit */
#define BITS_PER_UNIT 8

/* Width in bits of a "word", which is the contents of a machine register.
   Note that this is not necessarily the width of data type `int';
   if using 16-bit ints on a 68000, this would still be 32.
   But on a machine with 16-bit registers, this would be 16.  */
#define BITS_PER_WORD 32

/* Width of a word, in units (bytes).  */
#define UNITS_PER_WORD 4

/* Width in bits of a pointer.
   See also the macro `Pmode' defined below.  */
#define POINTER_SIZE 32

/* Allocation boundary (in *bits*) for storing arguments in argument list.  */
#define PARM_BOUNDARY (TARGET_SHORT ? 16 : 32)

/* Boundary (in *bits*) on which stack pointer should be aligned.  */
#define STACK_BOUNDARY 16

/* Allocation boundary (in *bits*) for the code of a function.  */
#define FUNCTION_BOUNDARY 16

/* Alignment of field after `int : 0' in a structure.  */
#define EMPTY_FIELD_BOUNDARY 16

/* No data type wants to be aligned rounder than this.  */
#define BIGGEST_ALIGNMENT 16

/* Set this nonzero if move instructions will actually fail to work
   when given unaligned data.  */
#define STRICT_ALIGNMENT 1

#define INT_TYPE_SIZE 32

/* Define this as 1 if `char' should by default be signed; else as 0.  */
#define DEFAULT_SIGNED_CHAR 1
/* Define this if zero-extension is slow (more than one real instruction).  */
#define SLOW_ZERO_EXTEND

/* Nonzero if access to memory by bytes is slow and undesirable.  */
#define SLOW_BYTE_ACCESS 0

/* Define if shifts truncate the shift count
   which implies one can omit a sign-extension or zero-extension
   of a shift count.  */
#define SHIFT_COUNT_TRUNCATED
/* Value is 1 if truncating an integer of INPREC bits to OUTPREC bits
   is done just by pretending it is already truncated.  */
#define TRULY_NOOP_TRUNCATION(OUTPREC, INPREC) 1

/* We assume that the store-condition-codes instructions store 0 for false
   and some other value for true.  This is the value stored for true.  */

#define STORE_FLAG_VALUE -1


/*****************************************************************************/

typedef long int SFVALUE;
/* Define the C data type to use for an SImode value.  */

#ifndef SItype
#define SItype long int
#endif

/* Define the type to be used for returning an SF mode value
   and the method for turning a float into that type.
   These definitions work for machines where an SF value is
   returned in the same register as an int.  */

#ifndef FLOAT_VALUE_TYPE  
#define FLOAT_VALUE_TYPE int
#endif

#ifndef INTIFY
#define INTIFY(FLOATVAL)  (intify.f = (FLOATVAL), intify.i)
#endif

#ifndef FLOATIFY
#define FLOATIFY(INTVAL)  ((INTVAL).f)
#endif

#ifndef FLOAT_ARG_TYPE
#define FLOAT_ARG_TYPE union flt_or_int
#endif

union flt_or_value { FLOAT_VALUE_TYPE i; float f; };

union flt_or_int { int i; float f; };

#endif
