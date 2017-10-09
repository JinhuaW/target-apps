
TARGETS=fuse_hello

BUILDROOT_OUTPUT=$(BUILDROOT)/output
CC := $(TARGET_CROSS)gcc
AS := $(TARGET_CROSS)as
DESTDIR := $(BUILDROOT_OUTPUT)/target/usr/bin
STAGING_DIR ?= $(BUILDROOT_OUTPUT)/staging

override CFLAGS += -Wall -Werror -g -Os -D_GNU_SOURCE
override CFLAGS += -O2

fuse_hello: override LDFLAGS += -lfuse

all: build install

build: $(TARGETS)

clean:
	rm -rf $(TARGETS)

install:
	mkdir -p $(DESTDIR)
	cp --no-dereference $(TARGETS) $(DESTDIR)/
