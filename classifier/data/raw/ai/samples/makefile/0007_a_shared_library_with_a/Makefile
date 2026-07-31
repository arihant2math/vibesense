CC := cc
CFLAGS := -Wall -Wextra -O2 -fPIC -Iinclude
LDFLAGS := -shared
PREFIX := /usr/local
LIBDIR := $(PREFIX)/lib
INCLUDEDIR := $(PREFIX)/include/foo

LIB := libfoo
VERSION := 1.0.0
SONAME := 1
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:src/%.c=build/%.o)

.PHONY: all clean install uninstall

all: build/$(LIB).so.$(VERSION)

build/$(LIB).so.$(VERSION): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -Wl,-soname,$(LIB).so.$(SONAME) -o $@ $^
	ln -sf $(LIB).so.$(VERSION) build/$(LIB).so.$(SONAME)
	ln -sf $(LIB).so.$(SONAME) build/$(LIB).so

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)

install: all
	install -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR)
	install -m 755 build/$(LIB).so.$(VERSION) $(DESTDIR)$(LIBDIR)/
	ln -sf $(LIB).so.$(VERSION) $(DESTDIR)$(LIBDIR)/$(LIB).so.$(SONAME)
	ln -sf $(LIB).so.$(SONAME) $(DESTDIR)$(LIBDIR)/$(LIB).so
	install -m 644 include/*.h $(DESTDIR)$(INCLUDEDIR)/

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB).so.$(VERSION)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB).so.$(SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB).so
	rm -rf $(DESTDIR)$(INCLUDEDIR)

clean:
	rm -rf build
