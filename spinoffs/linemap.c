#include <stddef.h>

////// Public ABI //////

typedef enum EolType EolType;
enum EolType {
  Eol_LF, // the correct one
  Eol_CRLF,
  // perhaps support for standalone CR would be ok?
  Eol_EOF
};

static inline // SLOP this needs to be either extern or static, I guess
char const* EolType2CStr(EolType eol) {
  switch (eol) {
    case Eol_LF: return "LF";
    case Eol_EOF: return "EOF";
    case Eol_CRLF: return "CRLF";
  }
}

typedef struct Line Line;
struct Line {
  size_t fileOffset; // offset from the start of the input (usually a file), in bytes
  size_t contentLen; // number of bytes from start of the line up to (but not including) the end of the line
  EolType eol;
};

typedef struct Lines Lines;
Lines* emptyLines();
void addLine(Lines* lines, Line* const line_in);

typedef struct LineBuffer LineBuffer;
LineBuffer* startLines();
void feedLines(LineBuffer* buf, size_t n, char inp[n]);
Lines* finishLines(LineBuffer* buf);

typedef struct LineIter LineIter;
LineIter* iterLines(Lines* const over);
// returns NULL when at end of the iterator
Line const* nextLine(LineIter* iter); // TODO perhaps return a bool and use an out parameter

////// Internal Declarations //////

#define LINES_PER_BLOCK 1000
struct Lines {
  Line block[LINES_PER_BLOCK];
  short used; // number of valid line entries in this block, allocated from the start
  Lines* next; // NULL at end of entries
  Lines* last; // only initialized for the first block, all others should be NULL
};

struct LineIter {
  size_t blockIx;
  Lines* curBlock;
};

////// Implementation //////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static _Noreturn
void panic(char* const msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

static inline
void* alloc(size_t sz) {
  void* ptr = malloc(sz);
  if (ptr == NULL) { panic("malloc failure"); }
  return ptr;
}

Lines* emptyLines() {
  Lines* new = alloc(sizeof(Lines));
  new->used = 0; new->next = NULL; new->last = new;
  return new;
}

void addLine(Lines* lines, Line* const line_in) {
  Lines* last = lines->last;
  assert(last != NULL);
  if (last->used >= LINES_PER_BLOCK) { // we need a new last block
    last = alloc(sizeof(Lines));
    last->used = 0; last->next = NULL; last->last = NULL; // initialize the new last block
    lines->last = last; // update the first block so its link to the last block is correct
  }
  memmove(&last->block[last->used++], line_in, sizeof(Line)); // push a copy of the line to the next entry in block
  // terse C can read as bad as assembly, and that's why I put the pseudocode in
}

LineIter* iterLines(Lines* const over) {
  LineIter* iter = alloc(sizeof(LineIter));
  iter->blockIx = 0; iter->curBlock = over;
  return iter;
}

Line const* nextLine(LineIter* iter) {
  while (iter->blockIx >= iter->curBlock->used) { // we need to go to the next block
    iter->curBlock = iter->curBlock->next;
    if (iter->curBlock == NULL) { return NULL; } // iterator ends
    iter->blockIx = 0;
  }
  return &iter->curBlock->block[iter->blockIx++]; // return the current line and advance
}

////// The Core Algorithm //////

struct LineBuffer {
  Lines* buf;
  size_t fileOffset;
  size_t lineLen;
  char lastChar;
};

// initialize a line buffer
LineBuffer* startLines() {
  Lines* lines = emptyLines();
  LineBuffer* buf = alloc(sizeof(LineBuffer));
  *buf = (LineBuffer){ .buf = lines, .fileOffset = 0, .lineLen = 0, .lastChar = '\0' };
  return buf;
}

/*DEBUG*/ #include <stdio.h>
// process bytes of input, adding lines to the Lines buffer
void feedLines(LineBuffer* buf, size_t n, char inp[n]) {
  // retrieve current buffer state
  size_t off = buf->fileOffset;
  size_t len = buf->lineLen;
  char last = buf->lastChar;
  // process
  for (size_t i = 0, dOff = 0; i < n; i++) {
    dOff++;
    if (inp[i] == '\n') {
      EolType eol = Eol_LF;
      if (last == '\r') {
        len--;
        eol = Eol_CRLF;
      }
      Line new = (Line){ .fileOffset = off, .contentLen = len, .eol = eol };
      addLine(buf->buf, &new);
      len = 0; off += dOff; dOff = 0;
    }
    else {
      len++;
    }
    last = inp[i];
  }
  // update the buffer state
  buf->fileOffset += n;
  buf->lineLen = len;
  buf->lastChar = last;
}

// destroys a line buffer, returning the accumulated lines
Lines* finishLines(LineBuffer* buf) {
  Line final = (Line){
    .fileOffset = buf->fileOffset,
    .contentLen = buf->lineLen,
    .eol = Eol_EOF
  };
  addLine(buf->buf, &final); // the final line in a map should always be an EOF
  return buf->buf;
}

////// Main //////

int main(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    char* path = argv[i];
    // process
    Lines* lines; {
      FILE* fp = fopen(path, "rb");
      if (!fp) { panic("error opening file"); } // slop: probably use errno
      LineBuffer* lbuf = startLines();
      do {
        #define main__BUFSZ 4096
        static char cbuf[main__BUFSZ];
        size_t bytesRead = fread(&cbuf, 1, main__BUFSZ, fp);
        if (ferror(fp)) { panic("error reading file"); }
        feedLines(lbuf, bytesRead, cbuf);
      } while (!feof(fp));
      lines = finishLines(lbuf);
      fclose(fp);
    }
    // print
    {
      printf("# %s\n", path);
      LineIter* iter = iterLines(lines);
      for (Line const* line = nextLine(iter); line; line = nextLine(iter)) {
        printf("%zu %zu %s\n", line->fileOffset, line->contentLen, EolType2CStr(line->eol));
      }
    }
  }
  return 0;
}
