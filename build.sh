#!/usr/bin/env sh
set -eu # o pipefail # pipefail not supported in posix until 2022

here="$0"
[ "${here##*/}" = "build.sh" ] || printf >&2 '[WARNING] Unexpected $0: %s\n\tOdd behavior may occur' "$0"
projdir="$(dirname "$0")"

# SLOP these environment variables need documentation
CC="${CC:-gcc}"
CFLAGS="${CFLAGS:--g}" # TODO but how am I gonna manage profiles??
SRCDIR="$projdir"
BUILDDIR="${BUILDDIR:-$projdir/.build}"
DISTDIR="${DISTDIR:-$projdir/dist}"
SRCDISTDIR="${DISTDIR:-$projdir/srcdist}"

build() #
{
  [ -e "$BUILDDIR" ] && rm -r "$BUILDDIR"
  mkdir -p "$BUILDDIR"
  # SLOP: non-atomic builds
  "$CC" $CFLAGS -c -DCYGNUS_IMPL \
     -xc "$SRCDIR/cygnus.h" -o "$BUILDDIR/cygnus.o"
  "$CC" \
     "$BUILDDIR/cygnus.o" -o "$BUILDDIR/cygnus.a"
  "$CC" $CFLAGS -fpic -c -DCYGNUS_IMPL \
     -xc "$SRCDIR/cygnus.h" -o "$BUILDDIR/cygnus.so"
  "$CC" $CFLAGS \
     "$BUILDDIR/cygnus.o" -o "$BUILDDIR/cygnus"
   grep '^[/ ]\*\*\([ /]\|$\)' "$SRCDIR/cygnus.h" \
     | sed -e 's/^[/ ]\*\*\([ /]\|$\)//' \
     > "$BUILDDIR/cygnus.md"
  # SLOP: compile/link/mklib/documentation outputs
  # SLOP: configurable targets
}

dist() #
{
  build
  [ -e "$DISTDIR" ] && rm -r "$DISTDIR"
  mkdir -p "$DISTDIR/bin" \
           "$DISTDIR/lib" \
           "$DISTDIR/include" \
           "$DISTDIR/doc"
  cp "$SRCDIR/cygnus.h" "$DISTDIR/include"
  cp "$BUILDDIR/cygnus.a" "$BUILDDIR/cygnus.so" "$DISTDIR/lib"
  cp "$BUILDDIR/cygnus" "$DISTDIR/bin"
  cp "$BUILDDIR/cygnus.md" "$DISTDIR/doc"
}

srcdist() #
{
  build
  [ -e "$SRCDISTDIR" ] && rm -r "$SRCDISTDIR"
  mkdir -p "SRCDISTDIR"
  cp "$SRCDIR/cygnus.h" "$SRCDISTDIR"
  cp "$BUILDDIR/cygnus.md" "$SRCDESTDIR"
}

# TODO an audit that looks for SLOP/FIXME/TODO/LATER/&c
# also, it should try to identify all commands this script calls
# may as well try to show what headers are included as well

help() #
### print a help message and exit
{
  echo "usage: $0 <COMMAND>"
  echo "    build system for the cygnus language"

  printf "\nCOMMANDS\n"
  help__cmdre='[a-z]\+()\s*#'
  help__docre='^\s*###\(\s\|$\)'
  grep "$help__cmdre"'\|'"$help__docre" "$here" \
    | sed -e 's/()\s*#\s*/\t/' \
          -e 's/^###\s*/\t/'
  exit 0
}

if [ "$#" -lt 1 ]; then
  help
else
  cmd="$1"
  # SLOP: check that the given command is documented
  shift
  set -x # SLOP: this should be controlled by verbosity
  "$cmd" $@
fi


# CONVENTIONS
#
# Posix shell doesn't support local.
# Instead, prefix a function-local variable with `<funcname>__`.
# Don't write recursive functions.
#
# Exposed commands are defined with `<cmdname>() #` with the opening brace/paren on the following line.
# The stuff after the hash describes the basic usage of the function/command.
# It is also documented when the `help` command is executed.
#
# Documentation is given with a triple-hash+space.
# This is searched and output when the `help` command is executed.
