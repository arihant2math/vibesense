.PHONY: all build vendor clean test

BINARY       ?= $(notdir $(CURDIR))
MAIN         ?= ./cmd/$(BINARY)
BIN_DIR      ?= bin
VERSION_PKG  ?= main
VERSION      ?= $(shell git describe --tags --always --dirty 2>/dev/null || printf 'dev')
COMMIT       ?= $(shell git rev-parse --short HEAD 2>/dev/null || printf 'unknown')
BUILD_TIME   ?= $(shell date -u '+%Y-%m-%dT%H:%M:%SZ')
GO           ?= go
GOFLAGS      ?=
LDFLAGS      ?= -s -w \
	-X $(VERSION_PKG).Version=$(VERSION) \
	-X $(VERSION_PKG).Commit=$(COMMIT) \
	-X $(VERSION_PKG).BuildTime=$(BUILD_TIME)

all: build

build: vendor
	@mkdir -p "$(BIN_DIR)"
	$(GO) build $(GOFLAGS) -mod=vendor -trimpath -ldflags "$(LDFLAGS)" -o "$(BIN_DIR)/$(BINARY)" "$(MAIN)"

vendor: go.mod go.sum
	$(GO) mod vendor

test: vendor
	$(GO) test $(GOFLAGS) -mod=vendor ./...

clean:
	rm -rf "$(BIN_DIR)"

fmt:
	$(GO) fmt ./...

tidy:
	$(GO) mod tidy

deps:
	$(GO) mod download

rebuild: clean build
