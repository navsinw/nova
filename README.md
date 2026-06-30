# NOVA-8

NOVA-8 is a small fantasy game console. A cartridge is a single `.nova` binary
container that bundles bytecode, sprite/tile graphics, palettes, a software
synthesizer, a tracker-style music sequencer, a bitmap font, and an optional
save-state snapshot. The runtime loads a cartridge and *runs* it: it executes
the bytecode CPU, advances the synth and sequencer per tick, renders sprites and
tilemaps into a framebuffer, and steps a small sprite/collision world.

## Layout

```
src/        console core (loader, CPU, memory, gfx, audio, font, collision, save-state)
fuzz/       libFuzzer harnesses + seed corpus + dictionary
tools/      mkcart/seed authoring helpers
.clusterfuzzlite/  build.sh + project.yaml
```

## Cartridge format

A `.nova` file is an 18-byte header (`NOVA` magic, version, flags, chunk count,
entry PC, header checksum) followed by a chunk directory and the chunk bodies.
Chunk tags: `CODE DATA SPRT MAP  PAL  SND  PATN FONT META SAVE`.

## Building & fuzzing

The ClusterFuzzLite entry point is `.clusterfuzzlite/build.sh`, which compiles
every harness in `fuzz/` into `$OUT`:

- `cart_fuzzer`      — full cartridge load + run
- `savestate_fuzzer` — save-state deserializer
- `audio_fuzzer`     — synth + tracker playback
- `sprite_fuzzer`    — sprite/tilemap/palette rendering
- `font_fuzzer`      — text/glyph rendering

Local build (clang + libFuzzer + ASan):

```
make
./build/cart_fuzzer fuzz/corpus/cart_fuzzer/
```

## Tools

`python3 tools/mkseed.py` regenerates the seed corpus. The console is intended to
be exercised through these entry points with structured cartridge input.
