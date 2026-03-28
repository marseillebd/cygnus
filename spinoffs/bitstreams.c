#include <stdlib.h>
#include <limits.h>
#include <assert.h>

char encode1(char bits, size_t bitdepth, const char alphabet[1 << bitdepth]) {
  assert(bitdepth > 0); // zero bitdepth doesn't make sense
  assert(bitdepth <= CHAR_BIT); // the bitdepth can only be as large as a character on the host machine (LIMITATION)
  _Static_assert(sizeof(size_t) > 1, ""); // and just to be pedantic, make sure the size_t type can hold 2^CHAR_BIT

  /* assert(bits < (1 << bitdepth)); // make sure the input bits don't exceed the size of the alphabet */
  bits &= (1 << bitdepth) - 1; // truncate so the index is always in range

  return alphabet[bits];
}

// returns a uchar cast up to a signed int, with all negative values indicating error
// a valid decode will be in the range [0, 2*bitdepth - 1)
int decode1(char letter, size_t bitdepth, const char alphabet[1 << bitdepth]) {
  for (size_t i = 0; i < (1 << bitdepth); i++) {
    if (letter == alphabet[i]) {
      return i;
    }
  }
  return -1;
}

typedef struct CodecBuffer CodecBuffer;
struct CodecBuffer {
  unsigned short buf; _Static_assert(sizeof(short) >= 2*sizeof(char), "oddball size of short on this platform");
  short nbits;
};

// TODO encodeBuf

// returns a char cast to a non-negative integer when a byte of output is ready
// returns a negative integer when no new output is ready
int decodeBuf(CodecBuffer* buf, char letter, size_t bitdepth, const char alphabet[1 << bitdepth]) {
  assert(buf->nbits < CHAR_BIT);
  assert(bitdepth > 0);
  int machine = decode1(letter, bitdepth, alphabet);
  if (machine >= 0) { // the letter decodes ok
    buf->buf = (buf->buf << bitdepth) | machine;
    buf->nbits += bitdepth;
    if (buf->nbits >= CHAR_BIT) { // we have a full bit batch
      int out = buf->buf >> (buf->nbits - CHAR_BIT);
      buf->nbits -= CHAR_BIT;
      return out;
    }
  }
  return -1; // current bit batch not full
}

// call with a zero bitdepth to flush any remaining bits (also resets the buffer to an initial state)
// returns a negative number if there are no remaining bits to flush
//
// the bits is an integer in the same way as the return type of decodeBuf.
// the npad is the number of padding bits that should be trimmed from the end of the returned bits.
struct CodecFlush { int bits; size_t npad; };
struct CodecFlush decodeFlush(CodecBuffer* buf) {
  assert(buf->nbits < CHAR_BIT);
  struct CodecFlush out = {0};
  if (buf->nbits == 0) {
    out.bits = -1;
    return out;
  } else {
    out.npad = CHAR_BIT - buf->nbits;
    out.bits = buf->buf << out.npad;
    return out;
  }
}

#include <stdio.h>

// right now, this is just emitting each group of bits in a whole byte
// it needs to pack bits into a buffer until it can emit a full byte
// LATER: refactor the buffering so that it leaves printing to the caller
// LATER: a -r flag to turn machine into human
// LATER: flags/args to control adding spaces and newlines to -r (but probably default to four per group and eight? groups per line
// LATER: flags/arguments to set up bitdepth&alphabet
int main(int argc, char** argv) {
  const char* const alphabet = "0123456789";
  CodecBuffer buf = {0};
  for (int human; (human = fgetc(stdin)) != EOF; ) {
    int bits = decodeBuf(&buf, human, 3, alphabet);
    if (bits >= 0) {
      fputc(bits, stdout);
    }
  }
  if (ferror(stdin)) {
    fputs("error reading input", stderr);
    return 1;
  }
  struct CodecFlush last = decodeFlush(&buf);
  if (last.bits >= 0) {
    fputc(last.bits, stdout);
  }
  fputc(last.npad, stdout); // the last byte in the output indicates how many bits to slice off the end of the rest of the message
                            //+but TODO: how does the reciever know the number of bits per byte the sender assumed?
                            //+I think I do need to send octets rather than bytes
  return 0;
}
