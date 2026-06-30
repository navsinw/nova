CC ?= clang
CFLAGS ?= -g -O1 -fsanitize=address,fuzzer-no-link -Isrc
FUZZ_LINK = -fsanitize=address,fuzzer

CORE = $(wildcard src/*.c)
HARNESSES = cart savestate audio sprite font gpu
BINS = $(addprefix build/,$(addsuffix _fuzzer,$(HARNESSES)))

all: $(BINS)

build:
	mkdir -p build

build/%_fuzzer: fuzz/%_fuzzer.c $(CORE) | build
	$(CC) $(CFLAGS) $(FUZZ_LINK) $< $(CORE) -o $@

TOOL_CC ?= clang
TOOL_FLAGS ?= -g -O1 -Isrc

tools: build
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_info.c  $(CORE) -o build/nova_info
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_dis.c   $(CORE) -o build/nova_dis
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_run.c   $(CORE) -o build/nova_run
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_as.c    $(CORE) -o build/nova_as
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_cc.c    $(CORE) -o build/nova_cc
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_trace.c $(CORE) -o build/nova_trace
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_view.c  $(CORE) -o build/nova_view
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_bench.c $(CORE) -o build/nova_bench
	$(TOOL_CC) $(TOOL_FLAGS) tools/nova_pack.c  $(CORE) -o build/nova_pack

test: build
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_main.c $(CORE) -o build/test_main
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_engine.c $(CORE) -o build/test_engine
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_data.c $(CORE) -o build/test_data
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_gen.c $(CORE) -o build/test_gen
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_misc.c $(CORE) -o build/test_misc
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_extra.c $(CORE) -o build/test_extra
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_integration.c $(CORE) -o build/test_integration
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_systems.c $(CORE) -o build/test_systems
	$(TOOL_CC) $(TOOL_FLAGS) -fsanitize=address tests/test_struct.c $(CORE) -o build/test_struct
	./build/test_main
	./build/test_engine
	./build/test_data
	./build/test_gen
	./build/test_misc
	./build/test_extra
	./build/test_integration
	./build/test_systems
	./build/test_struct

clean:
	rm -rf build

.PHONY: all clean tools test
