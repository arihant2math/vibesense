CC              ?= cc
INSTALL          ?= install
PREFIX           ?= /usr/local
LIBDIR           ?= $(PREFIX)/lib
INCLUDEDIR       ?= $(PREFIX)/include
DESTDIR           ?=

LIBNAME          ?= example
VERSION          ?= 1.0.0
MAJOR            := $(firstword $(subst ., ,$(VERSION)))

CPPFLAGS         ?= -Iinclude
CFLAGS           ?= -O2
LDFLAGS          ?=
LDLIBS           ?=

SOURCES          := $(wildcard src/*.c)
OBJECTS          := $(patsubst src/%.c,build/%.o,$(SOURCES))
DEPS             := $(OBJECTS:.o=.d)
HEADERS          := $(wildcard include/*.h)

UNAME_S          := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
SHARED_FLAGS     := -dynamiclib -install_name @rpath/lib$(LIBNAME).$(VERSION).dylib
LIBFILE          := lib$(LIBNAME).$(VERSION).dylib
SONAME_LINK      := lib$(LIBNAME).dylib
COMPAT_LINK      := lib$(LIBNAME).$(MAJOR).dylib
else
SHARED_FLAGS     := -shared -Wl,-soname,lib$(LIBNAME).so.$(MAJOR)
LIBFILE          := lib$(LIBNAME).so.$(VERSION)
SONAME_LINK      := lib$(LIBNAME).so.$(MAJOR)
COMPAT_LINK      := lib$(LIBNAME).so
endif

TARGET           := build/$(LIBFILE)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(SHARED_FLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -fPIC -c $< -o $@

install: $(TARGET)
	$(INSTALL) -d "$(DESTDIR)$(LIBDIR)" "$(DESTDIR)$(INCLUDEDIR)"
	$(INSTALL) -m 755 "$(TARGET)" "$(DESTDIR)$(LIBDIR)/$(LIBFILE)"
	ln -sf "$(LIBFILE)" "$(DESTDIR)$(LIBDIR)/$(SONAME_LINK)"
	ln -sf "$(SONAME_LINK)" "$(DESTDIR)$(LIBDIR)/$(COMPAT_LINK)"
	@for header in $(HEADERS); do \
		$(INSTALL) -m 644 "$$header" "$(DESTDIR)$(INCLUDEDIR)/"; \
	done

clean:
	rm -rf build

-include $(DEPS)
