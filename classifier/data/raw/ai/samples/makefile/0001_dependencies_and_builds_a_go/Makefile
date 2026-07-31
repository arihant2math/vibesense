APP_NAME ?= app
BIN_DIR ?= bin
CMD_DIR ?= .
VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
BUILD_TIME ?= $(shell date -u +%Y-%m-%dT%H:%M:%SZ)
MODULE ?= $(shell go list -m -f '{{.Path}}')
GOFLAGS ?=
LDFLAGS := -s -w \
	-X '$(MODULE)/internal/version.Version=$(VERSION)' \
	-X '$(MODULE)/internal/version.Commit=$(COMMIT)' \
	-X '$(MODULE)/internal/version.BuildTime=$(BUILD_TIME)'

.PHONY: all vendor build clean test fmt

# Builds the Go binary with version, commit, and build-time metadata embedded.
all: build

# Vendors all Go module dependencies into the vendor directory.
vendor:
	go mod tidy
	go mod vendor

# Compiles the application binary using the vendored dependencies and version stamping.
build: vendor
	mkdir -p $(BIN_DIR)
	go build $(GOFLAGS) -mod=vendor -ldflags "$(LDFLAGS)" -o $(BIN_DIR)/$(APP_NAME) $(CMD_DIR)

# Runs the test suite using vendored dependencies.
test: vendor
	go test -mod=vendor ./...

# Formats all Go source files in the repository.
fmt:
	gofmt -w $$(find . -type f -name '*.go' -not -path './vendor/*')

# Removes generated binaries and vendored dependencies.
clean:
	rm -rf $(BIN_DIR) vendor

