/** # Cygnus
 **
 ** I am disappointed in C.
 ** I am often disapointed in "C-replacements": they're too complex, to unstable, or too immature.
 ** The best I have to hope for is it's too niche, but then it's probably also too ambitious for me to casually hack on.
 **
 ** Cygnus has the _lack_ of ambition as its goal.
 ** A simplified syntax, a little sugar, a basic typechecker.
 ** Reuse existing C compilers as code generators.
 ** Cygnus should be maintanable as a single-header library.
 **
 ** TODO:
 ** - minimal s-expression parser
 ** - interpret sexprs as a c-like programming language augmented with
 **   - namespaces
 **   - limited parametric polymorphism (all type variables introduce opaque types)
 **   - extra control-flow: defer, exit-when
 ** - generates C code, both translation unit and header files
 **
 ** TODO: as time goes on I may add more, but there are no guarantees for these:
 ** - target C89 or C99 for codegen and the compiler itself.
 **   I would especially like to build with tinycc or onramp, so that it's low-down in the bootstrapping process.
 ** - opt-in resource types and lifetimes
 ** - native code generation (big if)
 **/

/** # Usage
 **
 ** This is a single-header library.
 ** This means you can include the header as normal to compile agains the API.
 ** To include the implementations in a translation unit,
 ** include the header after defining `CYGNUS_IMPL`.
 ** Your project must not do this in more than once translation unit.
 **/

#ifndef CYGNUS_H
#define CYGNUS_H

/** # ABI
 **
 ** There is no gurantee of ABI stability, not until libcygnus matures.
 ** For now, you can only rely on API stability, but there may come a day.
 **/

/** # API
 **/

#include <stddef.h>

// # SLOP assume 8-bit byte? Well, I at least assume ASCII, or do I?

/** ## Bytes module
 **
 ** Because C-style strings suck.
 ** I don't want to use them internally.
 ** So I won't. So there.
 **
 ** The Bytes type holds a length alongside a `char*`.
 ** This allows to hold arbitrary bytes in an array, rather than using NUL as a sentinal
 ** (NUL must be kept available for use in binary strings).
 **
 ** The ByteSlc (byte slice) type is the same, but its pointer is borrowed.
 **/

/** ###### Bytes type
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
 **/
typedef struct ByteSlc ByteSlc;/**!*//**/
struct ByteSlc {
  size_t len/**!- field *//*: length of the byte string*/;
  char* buf/**!- field *//*: **borrowed** pointer to arbitrary bytes*/;
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

/** ## S-Expressions module
 **
 ** We use a minimal type for s-expressions: atoms are simple byte strings, and combinations are accessed via cons-cells (as a linked list).
 ** Of course, the inner implementation may be more efficient, but for the moment it is not.
 ** After all, this is the first draft of this compiler, and I'm trying to keep it as simple as possible rather than as efficient as possible.
 **
 ** Likewise, this implementation simply uses `malloc` to create s-expressions.
 ** This lack of abstraction is probably fine, honestly.
 **/

/** ###### Sexpr type
 **/
typedef struct Sexpr Sexpr/**!*//* tagged union*/;
/**/

/* I think I need this defined early so the elimination form will work */
struct Sexpr {
  enum SexprTag {
    SexprATOM, SexprNIL, SexprCONS
  } _tag;
  union {
    /* nothing for NIL */
    ByteSlc atom;
    struct Cons {
      Sexpr* car;
      Sexpr* cdr;
    } cons;
  } _as;
};

/** ###### Sexpr introduction forms
 **
 ** All of these take ownership of their arguments.
 ** The caller owns the return value, which must be `free`d.
 **/
Sexpr* mkNil()/**!*//**/;
/**/
Sexpr* mkAtom(ByteSlc atom)/**!*//**/;
/**/
Sexpr* mkCons(Sexpr* car, Sexpr* cdr)/**!*//**/;
/** `car, cdr`: allowed to be `NULL`,
 **   but this should only happen when the s-expression is intended to be uninitialized.
 **/

/** ###### Sexpr elimination form
 **
 ** `matchSexpr(Sexpr tok) { <arcs...> }` macro.
 ** Begins destructuring of a token.
 ** The macro call itself must be followed by a block whose contents are "arc"s:
 **   uses of the `with{Nil,Atom,Cons}` macros.
 ** The order does not matter, but they should all be present.
 **/
#define matchSexpr(e) \
  for (bool _matchSexpr__arcs = true; _matchSexpr__arcs; ) \
  for (Sexpr* _matchSexpr__it = &(e); _matchSexpr__arcs; _matchSexpr__arcs = false) \
  switch (_matchSexpr__it->_tag)

/** The following are all valid "arc"s.
 ** Each one takes a list of variables which are declared and initialized as pointers to the data fields valid for the sexpr's constructor.
 ** Each call must be followed by a block of statements, which are executed when the sexpr's constructor matches the name in the arc macro.
 **
 ** Using these macros outside the body of a `matchSexpr` macro will probably cause a compiler error, but hopefully some sort of error.
 */
/** - `withNil() { <body> }` macro
 */
#define withNil() \
  break; case SexprNIL: \
  for (bool _matchSexpr__arc = true; _matchSexpr__arc; _matchSexpr__arc = false)
/** - `withAtom($str) { <body> }` macro
 **   - `ByteSlc* $str` is in-scope for the body
 */
#define withAtom(str) \
  break; case SexprATOM: \
  for(bool _matchSexpr__arc = true; _matchSexpr__arc; ) \
  for(ByteSlc* str = &_matchSexpr__it->_as.atom; \
      _matchSexpr__arc; _matchSexpr__arc = false)
/** - `withCons($car, $cdr) { <body> }` macro
 **   - `Sexpr *$car, *$cdr` are in-scope for the body
 */
#define withCons(hd, tl) \
  break; case SexprCONS: \
  for(bool _matchSexpr__arc = true; _matchSexpr__arc; ) \
  for(Sexpr *hd = &_matchSexpr__it->_as.cons.car, \
            *tl = &_matchSexpr__it->_as.cons.cdr; \
      _matchSexpr__arc; _matchSexpr__arc = false)
/**/

#endif /* CYGNUS_H */

#if defined(CYGNUS_IMPL) && !defined(CYGNUS_C)
#define CYGNUS_C

/** # Developer Docs
 **/

/** ## Conventions
 **
 ** ### Documentation
 **
 ** The code itself already specifies the _how_ of the code.
 ** Documentation is here to give the intent --- the _why_ --- of the code.
 ** Documentation is neccesary for the codebase's survival.
 ** Without the intent, maintenence is impossible: how does one decide between feature and bug?
 ** **Always document**, and do it well.
 **
 ** Documentation is given in the source file with special comments.
 ** The line must match `^[/ ]\*\*[ /]`, which is also stripped.
 ** The resulting lines are treated as github-flavored markdown[^why-gfm].
 **
 ** To include existing code in documentation, follow it with two comments.
 ** The first should start with `/**!`.
 ** That line will the be generated as:
 **   verbatim from the bang-comment,
 **   then the code left-trimmed and surrounded in backticks,
 **   and finally the content of the second comment.
 ** This structure allows the code to be wrapped in markdown on either side,
 **   for example, to introduce a list item with a colon after the code.
 **
 ** [why-gfm]: Why GFM? Because it allows linking to headers. It's also nice to have footnotes.
 **
 ** ### Techniques
 **
 ** The library `malloc`'s. It panics on allocation failure.
 ** I think that's fine for now, since I expect the library to only be used in a simple compiler,
 ** also implemented here.
 ** I'll revisit this later, _if_ there's demand for allocator-polymorphism.
 ** For now, use [`alloc`](#alloc-function) to allocate on the heap.
 **
 ** My approach to design is based in type theory, where there are five things to consider.
 ** - Formation Rule: In C, this is just the fact that the type exists, and perhaps its layout (size/alignment/abi).
 ** - Introduction Rule(s): The "constructors".
 **   For ordinary structs, it's ok to just use structure initializers.
 ** - Elimination Rule(s): The "accessors".
 **   For ordinary structs, this is usually just the members.
 **   However, we might also expose certain "views" through function calls.
 ** - Computation Rule(s): What happens when a deconstructor is called with a constructor?
 **   This simple description of behavior (assuming all side-effects have been computed)
 **   determines the behavior in all other cases, even the most complex.
 ** - Equality Rule(s): These are not the ordinary, decidable equality predicate.
 **   Instead it specifies how you might optimize a constructor that wraps a deconstructor.
 **   For example, a function (constructor) whose body (the wrapped expression) just delegates its arguments in-order to another function
 **   could be replaced by a call directly to the delegate function.
 **   This usually doesn't come up in C, or even outside proof assistants.
 **
 ** For plain-old-data structs, these rules are already built-in.
 ** A `typedef struct Foo Foo` is enough of a formation rule.
 ** Its constructor is just initializing all its members.
 ** Its deconstructor is just accessing all its members.
 ** The computation rule is just that accessing a member results in the same value it has been constructed with;
 **   of course, the mutability of struct members complicates this, but I'll ignore it for now.
 ** The short story is that plain old data just holds data.
 **
 ** Tagged unions is where things get more interesting.
 ** I try to keep these as opaque types to not expose their internals.
 ** The formation rule is just the declaration of an opaque type.
 ** These have multiple constructors; they take the form `<Name>* mk<Ctor>(<args...>)`.
 ** The deconstructor is effectively a branch with one case for each constructor.
 ** The computation rule just selects the branch corresponding to the constructor
 **   and executes it with the arguments in scope (pervasive mutability makes the nature of arguments subtle again).
 ** Unfortunately, the best way I've found to deal with this in C is through black macro magic.
 ** It's exposed in the public API for [s-expressions](#s-expressions-module),
 **   but a smaller example is in the [token type](#token-type)
 **
 ** There are some limitations to this approach.
 ** First, tagged unions are always accessed through pointers, and I `malloc` them (see allocation conventions).
 ** See the allocation conventions for why I consider this ok in this library's context.
 ** Second, since we don't include the type name in the constructor/deconstructor,
 **   the (d)ctors all exist in the same namespace.
 ** This library should be small enough that it won't cause a problem.
 ** I'll revisit the naming convention _if_ it becomes one.
 **
 ** ### Style
 **
 ** Minimize how many macros are exposed in the public interface.
 ** They're a pain to bind against, and generally can get out of control quickly.
 **
 **/

/** ## Includes
 **/
#include <assert.h>/**!*//**/
#include <stdlib.h>/**!*//**/
#include <stdio.h>/**!*//**/
#include <string.h>/**!*//**/
#include <ctype.h>/**!*//**/
#include <stdbool.h>/**!*//**/
/**/

/***************/
/** ## Helpers
 **/
/***************/

/** ###### panic function
 **/
__attribute__ ((noreturn)) // SLOP I think syntax differs bt compilers and standards
static
void panic(char* msg)/**!*//*:*/{
/**   print a message and exit.
 **/
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static inline
/** ###### alloc function
 **/
void* alloc(size_t sz)/**!*//*: allocate on the heap.*/ {
/** - `sz`: size in bytes to allocate
 ** - `return`: non-nullable; will `panic` if unable to allocate.
 **/
  void* out = malloc(sz);
  if (out == NULL) { panic("allocation failure\n"); }
  return out;
}

#define MEMFAIL(ptr) do { if (!(ptr)) { panic("allocation failure\n"); } } while (0)

/**************************/
/****** Bytes module ******/
/**************************/

ByteSlc ByteSlc_of_Bytes(Bytes b) {
  return (ByteSlc){ .len = b.len, .buf = b.buf };
}

/**************************/
/****** Sexpr module ******/
/**************************/

Sexpr* mkAtom(ByteSlc atom) {
  Sexpr* out = alloc(sizeof(Sexpr));
  MEMFAIL(out);
  out->_tag = SexprATOM;
  out->_as.atom = atom;
  return out;
}
Sexpr* mkNil() {
  Sexpr* out = alloc(sizeof(Sexpr));
  MEMFAIL(out);
  out->_tag = SexprNIL;
  return out;
}
Sexpr* mkCons(Sexpr* car, Sexpr* cdr) {
  Sexpr* out = alloc(sizeof(Sexpr));
  MEMFAIL(out);
  out->_tag = SexprNIL;
  out->_as.cons.car = car;
  out->_as.cons.cdr = cdr;
  return out;
}

/** ## Internal API
 **/

/** ### Lexer
 **/

/** ###### Token type
 **/
typedef struct Token Token/**!*//**/;
typedef struct Token {
  enum TokenTag {
    TokenOPEN,
    TokenCLOSE,
    TokenSYMBOL,
    TokenEND
  } _tag;
  union {
    /* no data for open/close or end */
    ByteSlc symbol;
  } _as;
} Token;

/** ###### Token elimination form
 **
 ** I found a couple of macro techniques, but their implementations aren't very readable,
 **   so I'll try to explain what's going on here.
 **
 ** Recall that the API should involve writing blocks after the macros.
 ** However, I want certain variables defined _only_ within that scope.
 ** This leaves me with a `for` loop as the only option.
 ** For example, in `matchToken`, the second for loop defines a variable that holds the token we are matching against.
 **
 ** The next issue is getting this for loop to execute only once.
 ** For this, we need a boolean variable which begins `true` and which we have to set `false` after the block is executed.
 ** The update step offers us the way to set it `false`.
 ** To initialize it to `true`, we need another for loop to declare this temporary flag.
 ** This means we need another for loop, now the outermost/first one.
 ** Every loop must be conditioned on this flag (or else that loop would go forever).
 ** Exactly the innermost/last loop sets it to false (or else further-inward loops would not execute).
 ** We can again see this structure in `matchToken`.
 **
 ** Note that I am relying heavily on C's ability to omit a control structure's braces
 **   when it contains exactly one statement.
 ** I normally dislike this, but it's needed here so that the user doesn't end up
 **   needing to close multiple open braces (mis-matched in the source, or use a special closing macro).
 **
 ** The next piece of useful ugly I found was that it is possible to put a `break` statement before the first `case`. 
 ** I'm not sure exactly how _legal_ this is, but my compiler accepts it be default.
 ** Since there's nowhere for the final case of a switch to fall through to, the last one doesn't need it.
 ** That's why each `with<Ctor>` macro starts with `break`, so the user doesn't need to bother.
 **
 ** To tie it all together, I need some special names.
 ** I'm reserving the pattern `_match<Type>__<var>`, which shouldn't cause any conflict.
 ** I use `arcs` to control the one-shot behavior of the overall match construct,
 **   `it` to hold the token being matched against,
 **   and `arc` to control the one-shot behavior of each branch.
 **
 ** Finally, I realized I needed a healthy dose of indirection.
 ** We store the matched-against token and it's ctor members by-reference.
 ** This is so that we can mutate the token from inside a branch, rather than mutating some copy.
 **
 ** So: multiple nested for loops for local variables,
 **   controlled by booleans to be one-shot,
 **   with the top-level match holding a switch,
 **   each branch causing the last to break out,
 **   and a smattering of specially-named (sometimes pointer) variables to hold it together.
 ** Yuck --- on the inside. But if the user gets it right, it looks great!
 ** I'm not yet sure how awful the error messages will be if the user gets it wrong.
 **/

/** ###### Token elimination form
 **
 ** `matchToken(Token tok) { <arcs...> }` macro.
 ** Begins destructuring of a token.
 ** The macro call itself must be followed by a block whose contents are "arc"s:
 **   uses of the `with{End,Open,Close,Symbol}` macros.
 ** The order does not matter, but they should all be present.
 **/
#define matchToken(tok) \
  for (bool _matchToken__arcs = true; _matchToken__arcs; ) \
  for (Token* _matchToken__it = &(tok); _matchToken__arcs; _matchToken__arcs = false) \
  switch (_matchToken__it->_tag)

/** The following are all valid "arc"s.
 ** Each one takes a list of variables which are declared and initialized as pointers to the data fields valid for the token's constructor.
 ** Each call must be followed by a block of statements, which are executed when the token's constructor matches the name in the arc macro.
 **
 ** Using these macros outside the body of a `matchToken` macro will probably cause a compiler error, but hopefully some sort of error.
 */
/** - `withEnd() { <body> }` macro
 */
#define withEnd() \
  break; case TokenEND: \
  for (bool _matchToken__arc = true; _matchToken__arc; _matchToken__arc = false)
/** - `withOpen() { <body> }` macro
 */
#define withOpen() \
  break; case TokenOPEN: \
  for (bool _matchToken__arc = true; _matchToken__arc; _matchToken__arc = false)
/** - `withClose() { <body> }` macro
 */
#define withClose() \
  break; case TokenCLOSE: \
  for (bool _matchToken__arc = true; _matchToken__arc; _matchToken__arc = false)
/** - `withSymbol($str) { <body> }` macro
 **   - `ByteSlc* $str` is in-scope for the body
 */
#define withSymbol(str) \
  break; case TokenSYMBOL: \
  for(bool _matchToken__arc = true; _matchToken__arc; ) \
  for(ByteSlc* str = &_matchToken__it->_as.symbol; \
      _matchToken__arc; _matchToken__arc = false)
/**/

static
bool isSexprSymbol(char c)/**!*//**/ {
  // we just list all the characters one-by-one, so we don't run afoul of encoding issues
  static const char* allowList =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    "_." "<=>" "!?&|^~" "*/%+-"
    ;
  return strchr(allowList, c) != NULL;
}

static
Token nextToken(ByteSlc* inp)/**!*//**/ {
  ByteSlc cur = *inp;
  #define TokenAdv(amt) do { \
    int _TokenAdvAmt = (amt); \
    cur.len-=_TokenAdvAmt; \
    cur.buf+=_TokenAdvAmt; \
  } while (0)

  // skip any whitespace and comments
  {
    enum { space, comment, finish } mode = space;
    while (cur.len > 0) {
      switch (mode) {
        case space: {
          if      (cur.buf[0] == '#')    { mode = comment; }
          else if (!isspace(cur.buf[0])) { mode = finish;  }
        } break;
        case comment: {
          if      (cur.buf[0] == '\n')   { mode = space; }
        } break;
      }
      if (mode == finish) { break; }
      TokenAdv(1);
    }
  }

  // detect token type
  Token out;
  if (cur.len == 0) {
    out._tag = TokenEND;
  } else if (cur.buf[0] == '(') {
    out._tag = TokenOPEN;
  } else if (cur.buf[0] == ')') {
    out._tag = TokenCLOSE;
  } else if (isSexprSymbol(cur.buf[0])) {
    out._tag = TokenSYMBOL;
  } else {
    panic("lexer error");
  }

  // extract the token
  matchToken(out) {
    withEnd() {}
    withOpen() { TokenAdv(1); }
    withClose() { TokenAdv(1); }
    withSymbol(atom) {
      // FIXME the atom's string should go into a strtab
      *atom = (ByteSlc){ .len = 0, .buf = cur.buf };
      for (int i = 0; i < cur.len; i++) {
        if (isSexprSymbol(cur.buf[i])) {
          atom->len++;
        } else {
          break;
        }
      }
      TokenAdv(atom->len);
    }
  }

  *inp = cur;
  return out;
  #undef TokenAdv
}

void test_lexer(ByteSlc inp)/**!*//**/ {
  Token tok;
  do {
    tok = nextToken(&inp);
    switch (tok._tag) {
      case TokenOPEN: { printf("+OPEN\n"); break; }
      case TokenCLOSE: { printf("+CLOSE\n"); break; }
      case TokenSYMBOL: {
        printf("+SYMBOL: %.*s\n", tok._as.symbol.len, tok._as.symbol.buf);
        break;
      }
      case TokenEND: { break; }
    }
  } while (tok._tag != TokenEND);
}

// TODO: I should likely have ifdef-guarded executables here after the implementation.
// I could have `CYGNUS_EXE` set to one of `test` or `exe`. Perhaps `perf` someday.
// Anyway, these macros would enable the appropriate main function (and its helpers),
// but you'd still need `IMPL` somewhere in your project.

/** ###### readFile
 **
 ** Read an entire file from a file handle.
 ** Takes ownership of the file (ie closes it).
 ** The resulting `Bytes` owns its buffer, which is `malloc`'d.
 **/
static
Bytes readFile(FILE* fp)/**!*//**/ {
  Bytes out;
  assert(fp);
  // get the size of the file
  fseek(fp, 0, SEEK_END); // SLOP lach of error handling after seek
  out.len = ftell(fp); // SLOP lach of error handling after ftell
  fseek(fp, 0, SEEK_SET); // SLOP lach of error handling after seek
  // read that many bytes into a new buffer
  out.buf = alloc(out.len);
  size_t bytesRead = fread(out.buf, 1, out.len, fp); // SLOP lack of error handling after read
  assert(bytesRead == out.len);
done:
  fclose(fp);
  return out;
}

/** main() -> int
 ** prints hello
 **/
int main()/**!*//**/ {
  Bytes file = readFile(fopen("bigtest.cyg", "rb"));
  printf("======\n%.*s\n======\n%zu\n", file.len, file.buf, file.len);
  test_lexer(ByteSlc_of_Bytes(file));
  free(file.buf);
  return 0;
}

#endif /* CYGNUS_IMPL */
