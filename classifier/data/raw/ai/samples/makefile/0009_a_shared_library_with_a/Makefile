CC ?= cc
CFLAGS ?= -Wall -Wextra -O2
CPPFLAGS ?= -Iinclude
LDFLAGS ?=
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include/foo

LIB_NAME := libfoo
VERSION := 1.0.0
SONAME := $(LIB_NAME).so.1
SHARED_LIB := $(LIB_NAME).so.$(VERSION)

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:src/%.c=build/%.o)
HEADERS := $(wildcard include/*.h)

.PHONY: all clean install uninstall

# Build the shared library and its versioned symlinks.
all: $(SHARED_LIB) $(SONAME) $(LIB_NAME).so

# Link position-independent object files into the versioned shared library.
$(SHARED_LIB): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,$(SONAME) -o $@ $^

# Create the soname symlink used by runtime linkers.
$(SONAME): $(SHARED_LIB)
	ln -sf $(SHARED_LIB) $@

# Create the unversioned linker symlink used during compilation.
$(LIB_NAME).so: $(SHARED_LIB)
	ln -sf $(SHARED_LIB) $@

# Compile each source file as position-independent code.
build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -MMD -MP -c $< -o $@

# Install the shared library, symlinks, and public headers.
install: all
	install -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR)
	install -m 755 $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/
	ln -sf $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/$(LIB_NAME).so
	install -m 644 $(HEADERS) $(DESTDIR)$(INCLUDEDIR)/

# Remove generated objects, dependency files, libraries, and symlinks.
clean:
	rm -rf build $(SHARED_LIB) $(SONAME) $(LIB_NAME).so

# Remove the installed library and public headers.
uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(SHARED_LIB)
	rm -f $(DESTDIR)$(LIBDIR)/$(SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME).so
	rm -rf $(DESTDIR)$(INCLUDEDIR)

-include $(OBJECTS:.o=.d)
