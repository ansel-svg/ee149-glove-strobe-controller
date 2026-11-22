# Host tests for the receiver logic. Builds with gcc; no ESP32 toolchain needed.
#   make test    build and run every suite
#   make clean   remove build output

CC ?= gcc
CFLAGS := -Wall -Wextra -std=gnu11

RECV   := ..
STUBS  := stubs
BUILD  := build
INCS   := -I$(STUBS) -I$(RECV) -I$(RECV)/src -I.

# Receiver code under test (the parts that do not touch real hardware).
PRODUCT := $(RECV)/espnow_link.c $(RECV)/state.c $(RECV)/calibration.c \
           $(RECV)/dmx_line.c $(RECV)/light_map.c
# Fakes for ESP-IDF and Wi-Fi calls.
FAKES   := $(STUBS)/stub_impl.c $(STUBS)/stub_nvs.c $(STUBS)/stub_gpio.c

TESTS   := packet_intake mode_machine light_map
TARGETS := $(addprefix $(BUILD)/,$(TESTS))

.PHONY: test clean

test: $(TARGETS)
	@status=0; \
	for t in $(TESTS); do \
	  ./$(BUILD)/$$t > $(BUILD)/$$t.log; rc=$$?; \
	  grep -E '^(FAIL|RESULT)' $(BUILD)/$$t.log | sed "s/^/$$t: /"; \
	  [ $$rc -eq 0 ] || status=1; \
	done; \
	exit $$status

$(BUILD)/%: %_test.c $(PRODUCT) $(FAKES)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCS) -o $@ $^ -lm

clean:
	rm -rf $(BUILD)
