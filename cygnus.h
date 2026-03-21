/** # Conventions
 **
 ** Documentation is given in the source file with special comments.
 ** The line must match `^[/ ]\*\*[ /]`, which is also stripped.
 ** The resulting lines are treated as github-flavored markdown[^why-gfm].
 **
 ** [why-gfm]: Why GFM? Because it allows linking to headers. It's also nice to have footnotes.
 **
 ** There are several API constructs that I attach documentation to.
 ** - **module**: Just an organizational structure.
 **   However, documented symbols in a module should have a common prefix.
 ** - **type**: Just the type.
 **   There are no ABI guarantees.
 **   If there are fields available (as for a struct), the public ones are documented.
 **/
#ifndef CYGNUS_H
#define CYGNUS_H

/** # API
 **/

#include <stddef.h>

// # SLOP assume 8-bit byte? Well, I at least assume ASCII

/** ## Bytes module
 **
 ** Because C-style strings suck.
 **
 ** The Bytes type holds a length alongside a `char*`.
 ** This allows to hold arbitrary bytes in an array, rather than using NUL as a sentinal
 ** (NUL must be kept available for use in binary strings).
 **/

/** ###### `Bytes` type
 **/
typedef struct Bytes {
/** - `.len : size_t` length of the byte string
  */ size_t len;
/** - `.bytes : char*` pointer to arbitrary bytes.
 **   At least `.len` bytes must be valid memory.
 **   TODO: I'm uncertain if this is owned or borrowed. It may depend on context.
  */ char* bytes;
/**/
} Bytes;

typedef struct Sexpr Sexpr;
struct Sexpr {
  enum SexprTag {
    SexprATOM, SexprNIL, SexprCONS
  } _tag;
  union {
    /* nothing for NIL */
    Bytes atom;
    struct Cons {
      Sexpr* car;
      Sexpr* cdr;
    } cons;
  } _as;
};

#endif /* CYGNUS_H */

#if defined(CYGNUS_IMPL) && !defined(CYGNUS_C)
#define CYGNUS_C
#include <stdio.h>

/** main() -> int
 ** prints hello
 **/
int main() {
  printf("Hello, cygnus!\n");
  return 0;
}

#endif /* CYGNUS_IMPL */
