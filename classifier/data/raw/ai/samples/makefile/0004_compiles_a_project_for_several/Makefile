.RECIPEPREFIX := >

PROGRAM := app
BUILD   := build
TARGETS := linux-amd64 linux-arm64 linux-armv7

CC_linux-amd64 := x86_64-linux-gnu-gcc
CC_linux-arm64 := aarch64-linux-gnu-gcc
CC_linux-armv7  := arm-linux-gnueabihf-gcc

CPPFLAGS :=
CFLAGS   := -O2 -Wall -Wextra
LDFLAGS  :=
LDLIBS   :=

BINARIES := $(TARGETS:%=$(BUILD)/%/$(PROGRAM))

.PHONY: all clean

all: $(BINARIES)

$(BUILD)/%/$(PROGRAM): $(BUILD)/%/main.o
>mkdir -p $(@D)
>$(CC_$*) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/%/main.o: src/main.c
>mkdir -p $(@D)
>$(CC_$*) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
>rm -rf $(BUILD)
