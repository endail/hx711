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

CXX := g++
AR := ar
INCDIR := include
SRCDIR := src
BUILDDIR := build
BINDIR := bin
SRCEXT := cpp
LIBS := -llgpio -pthread
INC := -I $(INCDIR)
CFLAGS :=	-O2 \
			-fomit-frame-pointer \
			-pipe \
			-Wall \
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

########################################################################

# https://stackoverflow.com/a/39895302/570787
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

ifeq ($(OS),Windows_NT)
	IS_WIN := 1
endif

ifeq ($(GITHUB_ACTIONS),true)
	IS_GHA := 1
endif

ifneq ($(IS_WIN),1)
	IS_PI := $(shell test -f /proc/device-tree/model && grep -qi "raspberry pi" /proc/device-tree/model)
endif

########################################################################

ifeq ($(IS_WIN),1)
# overwrite binaries for dev
# NOTE: toolchain available here: https://gnutoolchains.com/raspberry/
# set RPI_TOOLCHAIN environment variable to directory of toolchain
	CXX = $(RPI_TOOLCHAIN)/bin/arm-linux-gnueabihf-g++.exe
	AR = $(RPI_TOOLCHAIN)/bin/arm-linux-gnueabihf-ar.exe
endif

ifeq ($(IS_PI),1)
# only include these flags on rpi
	CFLAGS :=	-march=native \
				-mfpu=vfp \
				-mfloat-abi=hard \
				$(CFLAGS)
endif

ifeq ($(IS_GHA),1)
# gha needs these additional libs
	LIBS := $(LIBS) -lrt -lcrypt
endif

CXXFLAGS :=		-std=c++11 \
				-fexceptions \
				$(CFLAGS)

########################################################################

.PHONY: all
all:	dirs \
		build \
		execs

.PHONY: build
build: $(BUILDDIR)/static/libhx711.a $(BUILDDIR)/shared/libhx711.so

.PHONY execs:
execs: hx711calibration test

.PHONY: clean
clean:
ifeq ($(IS_WIN),1)
	del /S /Q $(BUILDDIR)\*
	del /S /Q $(BINDIR)\*
else
	rm -r $(BUILDDIR)/*
	rm -r $(BINDIR)/*
endif

.PHONY: dirs
dirs:
ifeq ($(IS_WIN),1)
	if not exist $(BINDIR) mkdir $(BINDIR)
	if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	if not exist $(BUILDDIR)\static mkdir $(BUILDDIR)\static
	if not exist $(BUILDDIR)\shared mkdir $(BUILDDIR)\shared
else
	mkdir -p $(BINDIR)
	mkdir -p $(BUILDDIR)/static
	mkdir -p $(BUILDDIR)/shared
endif


$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

$(BUILDDIR)/static/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

$(BUILDDIR)/shared/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) -fPIC $(INC) -c -o $@ $<


# Build static library
$(BUILDDIR)/static/libhx711.a:	$(BUILDDIR)/static/AbstractScale.o \
								$(BUILDDIR)/static/AdvancedHX711.o \
								$(BUILDDIR)/static/HX711.o \
								$(BUILDDIR)/static/Mass.o \
								$(BUILDDIR)/static/SimpleHX711.o \
								$(BUILDDIR)/static/Utility.o \
								$(BUILDDIR)/static/Value.o \
								$(BUILDDIR)/static/ValueStack.o \
								$(BUILDDIR)/static/Watcher.o

	$(AR) rcs	$(BUILDDIR)/static/libhx711.a \
				$(BUILDDIR)/static/AbstractScale.o \
				$(BUILDDIR)/static/AdvancedHX711.o \
				$(BUILDDIR)/static/HX711.o \
				$(BUILDDIR)/static/Mass.o \
				$(BUILDDIR)/static/SimpleHX711.o \
				$(BUILDDIR)/static/Utility.o \
				$(BUILDDIR)/static/Value.o \
				$(BUILDDIR)/static/ValueStack.o \
				$(BUILDDIR)/static/Watcher.o

# Build shared library
$(BUILDDIR)/shared/libhx711.so:		$(BUILDDIR)/shared/AbstractScale.o \
									$(BUILDDIR)/shared/AdvancedHX711.o \
									$(BUILDDIR)/shared/HX711.o \
									$(BUILDDIR)/shared/Mass.o \
									$(BUILDDIR)/shared/SimpleHX711.o \
									$(BUILDDIR)/shared/Utility.o \
									$(BUILDDIR)/shared/Value.o \
									$(BUILDDIR)/shared/ValueStack.o \
									$(BUILDDIR)/shared/Watcher.o
	$(CXX)	-shared \
		$(CXXFLAGS) \
		$(INC) \
		-o $(BUILDDIR)/shared/libhx711.so \
			$(BUILDDIR)/shared/AbstractScale.o \
			$(BUILDDIR)/shared/AdvancedHX711.o \
			$(BUILDDIR)/shared/HX711.o \
			$(BUILDDIR)/shared/Mass.o \
			$(BUILDDIR)/shared/SimpleHX711.o \
			$(BUILDDIR)/shared/Utility.o \
			$(BUILDDIR)/shared/Value.o \
			$(BUILDDIR)/shared/ValueStack.o \
			$(BUILDDIR)/shared/Watcher.o \
		$(LIBS)


.PHONY: hx711calibration
hx711calibration: $(BUILDDIR)/Calibration.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/hx711calibration \
		$(BUILDDIR)/Calibration.o \
		-L $(BUILDDIR)/static \
		-lhx711 $(LIBS)

.PHONY: test
test: $(BUILDDIR)/SimpleHX711Test.o $(BUILDDIR)/AdvancedHX711Test.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/simplehx711test \
		$(BUILDDIR)/SimpleHX711Test.o \
		-L $(BUILDDIR)/static \
		-lhx711 $(LIBS)

	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/advancedhx711test \
		$(BUILDDIR)/AdvancedHX711Test.o \
		-L $(BUILDDIR)/static \
		-lhx711 $(LIBS)
	

.PHONY: discovery
discovery: $(BUILDDIR)/DiscoverTiming.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/discovery \
		$(BUILDDIR)/DiscoverTiming.o \
		-L $(BUILDDIR)/static \
		-lhx711 -lgsl $(LIBS)



.PHONY: install
install: $(BUILDDIR)/static/libhx711.a $(BUILDDIR)/shared/libhx711.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/static/libhx711.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/shared/libhx711.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/hx711
	install -m 644 $(INCDIR)/*.h $(DESTDIR)$(PREFIX)/include/hx711
	ldconfig

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libhx711.*
	rm -rf $(DESTDIR)$(PREFIX)/include/hx711
	ldconfig
