.PHONY: all docs package clean

DOCS_DIR := docs
BUILD_DIR := build
PACKAGE_NAME := project-docs.tar.gz
DOC_SOURCES := $(shell find . -type f \( -name '*.md' -o -name '*.rst' \) -not -path './$(BUILD_DIR)/*' -not -path './.git/*')

all: package

docs: $(BUILD_DIR)/docs/index.html

$(BUILD_DIR)/docs/index.html: $(DOC_SOURCES)
	@mkdir -p $(BUILD_DIR)/docs
	@if command -v pandoc >/dev/null 2>&1; then \
		pandoc README.md -s -o $@; \
	else \
		printf '%s\n' '<!doctype html><html><body><pre>' > $@; \
		cat README.md >> $@; \
		printf '%s\n' '</pre></body></html>' >> $@; \
	fi

package: docs
	@tar -czf $(PACKAGE_NAME) -C $(BUILD_DIR) docs
	@echo "Created $(PACKAGE_NAME)"

clean:
	@rm -rf $(BUILD_DIR) $(PACKAGE_NAME)

