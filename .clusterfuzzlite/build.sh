#!/bin/bash -eu

cd "$SRC"

CORE=$(ls src/*.c)

for h in cart savestate audio sprite font gpu; do
  $CC $CFLAGS -Isrc $LIB_FUZZING_ENGINE \
      fuzz/${h}_fuzzer.c $CORE \
      -o "$OUT/${h}_fuzzer"
done

for h in cart savestate audio sprite font gpu; do
  d="fuzz/corpus/${h}_fuzzer"
  if [ -d "$d" ] && [ -n "$(ls -A "$d" 2>/dev/null)" ]; then
    (cd "$d" && zip -q -r "$OUT/${h}_fuzzer_seed_corpus.zip" .) || true
  fi
done

cp fuzz/dictionary.txt "$OUT/" 2>/dev/null || true
