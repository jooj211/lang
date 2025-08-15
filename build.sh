#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$ROOT/build"
GEN="$BUILD/generated"
SRC="$ROOT/src"
mkdir -p "$BUILD" "$GEN"

green() { printf '\033[1;32m%s\033[0m\n' "$*"; }
yellow(){ printf '\033[1;33m%s\033[0m\n' "$*"; }
red()   { printf '\033[1;31m%s\033[0m\n' "$*"; }

have() { command -v "$1" >/dev/null 2>&1; }

try_cmake() {
  if [ -f "$ROOT/CMakeLists.txt" ] && have cmake; then
    yellow "[build] Trying CMake (CMakeLists.txt detected) ..."
    set +e
    cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
    local ec=$?
    set -e
    if [ $ec -ne 0 ]; then
      yellow "[build] CMake configure failed (exit $ec); falling back to manual."
      return 1
    fi

    set +e
    cmake --build "$BUILD" -j
    ec=$?
    set -e
    if [ $ec -ne 0 ]; then
      yellow "[build] CMake build failed (exit $ec); falling back to manual."
      return 1
    fi

    green "[build] Done via CMake."
    return 0
  fi
  return 1
}

if try_cmake; then exit 0; fi

yellow "[build] Using manual path (Flex/Bison + g++)."

if [ ! -d "$SRC" ]; then
  red "[build] ERROR: src/ not found at $SRC"; exit 1
fi
for tool in bison flex g++; do
  if ! have "$tool"; then red "[build] ERROR: $tool not found in PATH"; exit 1; fi
done

PARSER_Y="$SRC/parser/parser.y"
LEXER_L="$SRC/lexer/lexer.l"
[ -f "$PARSER_Y" ] || { red "[build] ERROR: parser.y not found at $PARSER_Y"; exit 1; }
[ -f "$LEXER_L" ] || { red "[build] ERROR: lexer.l not found at $LEXER_L"; exit 1; }

green "[build] Generating parser/lexer with Bison/Flex ..."
bison -Wall -Wcounterexamples -d -o "$GEN/parser.tab.cpp" "$PARSER_Y"
flex  -o "$GEN/lexer.cpp" "$LEXER_L"

MAIN=""
if   [ -f "$SRC/main.cpp"  ]; then MAIN="$SRC/main.cpp"
elif [ -f "$SRC/main2.cpp" ]; then MAIN="$SRC/main2.cpp"
elif [ -f "$SRC/main3.cpp" ]; then MAIN="$SRC/main3.cpp"
fi
[ -n "$MAIN" ] || { red "[build] ERROR: no main.cpp found under src/"; exit 1; }

mapfile -t CPP < <(find "$SRC" -type f -name '*.cpp' ! -name 'main.cpp' ! -name 'main2.cpp' ! -name 'main3.cpp' | sort)

green "[build] Compiling (gnu++17, O2) ..."
OUT="$BUILD/lang"
g++ -std=gnu++17 -O2 -I"$SRC" -I"$GEN" "$MAIN" "${CPP[@]}" "$GEN/parser.tab.cpp" "$GEN/lexer.cpp" -o "$OUT" -lfl
green "[build] Done: $OUT"

if [ -x "$OUT" ]; then
  LFILE="$(find "$ROOT/instances" -type f -name 'fib*.lan' | head -n1 || true)"
  if [ -n "$LFILE" ]; then
    yellow "[test] Running -syn/-i on $(basename "$LFILE")"
    set +e
    "$OUT" -syn "$LFILE" >/dev/null 2>&1
    "$OUT" -i   "$LFILE" >/dev/null 2>&1
    set -e
  fi
fi
