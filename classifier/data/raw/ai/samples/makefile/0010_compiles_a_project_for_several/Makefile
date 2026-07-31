APP_NAME := app
CMD      := ./cmd/$(APP_NAME)
DIST     := dist
VERSION  := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)

TARGETS := \
	linux/amd64 \
	linux/arm64 \
	linux/arm \
	darwin/amd64 \
	darwin/arm64 \
	windows/amd64 \
	windows/arm64

.PHONY: all build clean

all: build

build:
	@mkdir -p $(DIST)
	@for target in $(TARGETS); do \
		OS=$${target%/*}; \
		ARCH=$${target#*/}; \
		EXT=; \
		if [ "$$OS" = "windows" ]; then EXT=.exe; fi; \
		echo "Building $$OS/$$ARCH"; \
		GOOS=$$OS GOARCH=$$ARCH CGO_ENABLED=0 \
			go build -trimpath -ldflags="-s -w -X main.version=$(VERSION)" \
			-o "$(DIST)/$(APP_NAME)-$$OS-$$ARCH$$EXT" $(CMD); \
	done

clean:
	rm -rf $(DIST)
