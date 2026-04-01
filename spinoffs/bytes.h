#ifndef BYTES_H
#define BYTES_H

/** # ABI
 **
 ** There is no gurantee of ABI stability, not until libcygnus matures.
 ** For now, you can only rely on API stability, but there may come a day.
 **/

// TODO hide behind a WANT_API

/** # API
 **
 ** If you compile against this, you must also make sure you are linking against this version.
 **/

#include <stddef.h>

/** ## Bytes module
 **
 ** Because C-style strings suck.
 ** I don't want to use them internally.
 ** So I won't. So there.
 **/

/** ###### Bytes type
 **
 ** The Bytes type holds a length alongside a `char*`.
 ** This allows to hold arbitrary bytes in an array, rather than using NUL as a sentinal
 ** (NUL must be kept available for use in binary strings).
 **
 ** If you would like to _borrow_ the underlying bytes, see `ByteSlc`.
 **/
typedef struct Bytes Bytes;/**!*//**/
struct Bytes {
  size_t len/**!- field *//*: length of the byte string*/;
  char* buf/**!- field *//*: **owned** pointer to arbitrary bytes*/;
/**   At least `.len` bytes must be valid memory.
 **   Borrowed pointers should use a slice type.
 **/
};

/** ###### Byte Slice type
 **
 ** The ByteSlc type is like `Bytes`: it holds a length and a pointer to the bytes.
 ** However, ByteSlc _borrows_ the pointer to the data.
 **/
typedef struct ByteSlc ByteSlc;/**!*//**/
struct ByteSlc {
  size_t len/**!- field *//*: length of the byte string*/;
  const char* buf/**!- field *//*: **borrowed** pointer to arbitrary bytes*/;
/**   At least `.len` bytes must be valid memory.
 **   Owned pointers should use the `Bytes` type or similar.
 **/
};

/** ###### Slices from Bytes
 **/
ByteSlc ByteSlc_of_Bytes(Bytes b)/**!*//* function.*/;
/** Convert a `Bytes` to ` ByteSlc`, thereby making ownership semantics a bit more explicit.
 ** - `b`
 ** - `return`: a slice containing the full contents of `b`.
 **   It's lifetime is bounded by the lifetime of `b.buf`.
 **/

#endif // BYTES_H

////// Implementation //////
// no promises

#ifdef WANT_BYTES_IMPL
#ifndef BYTES_C
#define BYTES_C

ByteSlc ByteSlc_of_Bytes(Bytes b) {
  return (ByteSlc){ .len = b.len, .buf = b.buf };
}

// TODO takePrefix, takeUpto, takeThru

#endif // BYTES_C
#endif // WANT_BYTES_C
