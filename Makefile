# MIT License
#
# Copyright (c) 2021 Daniel Robertson
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# set RASPI_TOOLCHAIN to cross compiler dir (ie. cross-pi-gcc-10.3.0-0/bin/arm-linux-gnueabihf)
# set RASPI_SYSROOT to rpi's root dir (ie. raspberrypi/rootfs)

SYSROOT_PATH		:= $(RASPI_SYSROOT)
TOOLCHAIN			:= $(RASPI_TOOLCHAIN)

TOOLCHAIN_CC		:= $(TOOLCHAIN)-gcc
TOOLCHAIN_CXX		:= $(TOOLCHAIN)-g++
TOOLCHAIN_LD		:= $(TOOLCHAIN)-ld
TOOLCHAIN_AR		:= $(TOOLCHAIN)-ar
TOOLCHAIN_RANLIB	:= $(TOOLCHAIN)-ranlib
TOOLCHAIN_STRIP		:= $(TOOLCHAIN)-strip
TOOLCHAIN_NM		:= $(TOOLCHAIN)-nm

CXX			:= $(TOOLCHAIN_CXX)
AR			:= $(TOOLCHAIN_AR)
STRIP		:= $(TOOLCHAIN_STRIP)
INCDIR		:= include
SRCDIR		:= src
BUILDDIR	:= build
BINDIR		:= bin
SRCEXT		:= cpp
PREFIX 		:= /usr/local
LIBS		:= -llgpio -pthread
INC			:= -I$(INCDIR)

# append paths for third-party libs (lgpio, etc...) from rpi sysroot
LIBS :=	$(LIBS) \
		-L$(SYSROOT_PATH)$(PREFIX)/lib -Wl,-rpath-link,$(SYSROOT_PATH)$(PREFIX)/lib
INC := 	$(INC) \
		-I$(SYSROOT_PATH)$(PREFIX)/include

CFLAGS :=	-O3 \
			-fomit-frame-pointer \
			-pipe \
			-Wall \
			-Wextra \
			-fstack-clash-protection \
			-Wfatal-errors \
			-Werror=format-security \
			-Wl,-z,relro \
			-Wl,-z,now \
			-Wl,-z,defs	\
			-Wl,--hash-style=gnu \
			-Wl,--as-needed \
			-D_FORTIFY_SOURCE=2 \
			-DNDEBUG=1

# link time optz
# https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/Raspberry%20Pi%20GCC%20Cross-Compiler%20Toolchains/Bullseye/GCC%2010.3.0/Raspberry%20Pi%203A%2B%2C%203B%2B%2C%204/#optimization-flags-involved
# these flags are for pi 0, A/B/A+/B+
CFLAGS :=	$(CFLAGS) \
			-march=armv6 \
			-mfloat-abi=hard \
			-mfpu=vfp

CXXFLAGS :=	-std=c++11 \
			-fexceptions \
			$(CFLAGS)

########################################################################

LIBNAME := hx711
SOVERSION := 2.4.0
SHARED_DIR := $(BUILDDIR)/shared
SHARED_LIB := $(SHARED_DIR)/lib$(LIBNAME).so

# generate a list of paths to .o object files
OBJS	:=	AbstractScale \
			AdvancedHX711 \
			HX711 \
			Mass \
			SimpleHX711 \
			Utility \
			Value \
			ValueStack \
			Watcher
OBJS 	:=	$(addsuffix .o,$(OBJS))
OBJS 	:=	$(addprefix $(SHARED_DIR)/,$(OBJS))

########################################################################

.PHONY: all
all:	dirs \
		build \
		execs

.PHONY: build
build: $(SHARED_LIB)

.PHONY execs:
execs: hx711calibration test

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/*
	rm -rf $(BINDIR)/*

.PHONY: dirs
dirs:
	mkdir -p $(BINDIR)
	mkdir -p $(SHARED_DIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

$(SHARED_DIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) -fPIC $(INC) -c -o $@ $<

$(SHARED_LIB): $(OBJS)
	$(CXX) -shared $(CXXFLAGS) $(INC) -o $(SHARED_LIB) $(OBJS) $(LIBS)
	$(STRIP) --strip-unneeded $(SHARED_LIB)
	file $(SHARED_LIB)
	size $(SHARED_LIB)
	mv $(SHARED_LIB) $(SHARED_LIB).$(SOVERSION)
	ln -fs $(SHARED_LIB).$(SOVERSION) $(SHARED_LIB)

.PHONY: hx711calibration
hx711calibration: $(BUILDDIR)/Calibration.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/hx711calibration \
		$(BUILDDIR)/Calibration.o \
		-L$(SHARED_DIR) \
		-l$(LIBNAME) $(LIBS)

.PHONY: test
test: $(BUILDDIR)/SimpleHX711Test.o $(BUILDDIR)/AdvancedHX711Test.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/simplehx711test \
		$(BUILDDIR)/SimpleHX711Test.o \
		-L$(SHARED_DIR) \
		-l$(LIBNAME) $(LIBS)

	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/advancedhx711test \
		$(BUILDDIR)/AdvancedHX711Test.o \
		-L$(SHARED_DIR) \
		-l$(LIBNAME) $(LIBS)

.PHONY: install
install: $(SHARED_LIB)
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(SHARED_LIB) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/$(LIBNAME)
	install -m 644 $(INCDIR)/*.h $(DESTDIR)$(PREFIX)/include/$(LIBNAME)
	ldconfig

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/lib$(LIBNAME).*
	rm -rf $(DESTDIR)$(PREFIX)/include/$(LIBNAME)
	ldconfig
