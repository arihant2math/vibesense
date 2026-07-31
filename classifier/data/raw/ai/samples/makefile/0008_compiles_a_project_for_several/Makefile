PROJECT := app
SRC_DIR := src
BUILD_DIR := build

CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

TARGETS := \
	linux-amd64 \
	linux-arm64 \
	linux-armv7 \
	darwin-amd64 \
	darwin-arm64

CC_linux-amd64  := x86_64-linux-gnu-gcc
CC_linux-arm64  := aarch64-linux-gnu-gcc
CC_linux-armv7  := arm-linux-gnueabihf-gcc
CC_darwin-amd64 := x86_64-apple-darwin23-clang
CC_darwin-arm64 := arm64-apple-darwin23-clang

SRC := $(wildcard $(SRC_DIR)/*.c)

.PHONY: all clean $(TARGETS)

all: $(TARGETS)

define BUILD_template
OBJ_$(1) := $$(patsubst $(SRC_DIR)/%.c,$$(BUILD_DIR)/$(1)/%.o,$$(SRC))
BIN_$(1) := $$(BUILD_DIR)/$(1)/$(PROJECT)

$(1): $$(BIN_$(1))

$$(BIN_$(1)): $$(OBJ_$(1))
	@mkdir -p $$(@D)
	$$(CC_$(1)) $$(LDFLAGS) -o $$@ $$^ $$(LDLIBS)

$$(BUILD_DIR)/$(1)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $$(@D)
	$$(CC_$(1)) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
endef

$(foreach target,$(TARGETS),$(eval $(call BUILD_template,$(target))))

clean:
	rm -rf $(BUILD_DIR)
