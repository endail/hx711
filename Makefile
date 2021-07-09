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
			-pg \
			-pipe \
			-Wall \
			-Wfatal-errors \
			-Werror=format-security \
			-Wl,-z,relro \
			-Wl,-z,now \
			-Wl,-z,defs	\
			-Wl,--hash-style=gnu \
			-Wl,--as-needed \
			-D_FORTIFY_SOURCE=2 \
			-fstack-clash-protection \
			-DNDEBUG=1

#			-fomit-frame-pointer \

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
	CXX = $(RPI_TOOLCHAIN)/bin/arm-linux-gnueabihf-g++.exe
	AR = $(RPI_TOOLCHAIN)/bin/arm-linux-gnueabihf-ar.exe
endif

ifeq ($(IS_PI),1)
# only include these flags on rpi
	CFLAGS := 	-march=native \
				-mfpu=vfp \
				-mfloat-abi=hard \
				$(CFLAGS)
endif

ifeq ($(IS_GHA),1)
# gha needs these additional libs
	LIBS := $(LIBS) -lrt -lcrypt -pthread
endif

CXXFLAGS := -std=c++11 \
			-fexceptions \
			$(CFLAGS)

########################################################################


.PHONY: all
all: 	dirs \
		$(BUILDDIR)/static/libhx711.a \
		$(BUILDDIR)/shared/libhx711.so \
		hx711calibration \
		test

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
$(BUILDDIR)/static/libhx711.a:	$(BUILDDIR)/static/HX711.o \
								$(BUILDDIR)/static/Mass.o \
								$(BUILDDIR)/static/SimpleHX711.o
	$(AR) rcs 	$(BUILDDIR)/static/libhx711.a \
				$(BUILDDIR)/static/HX711.o \
				$(BUILDDIR)/static/Mass.o \
				$(BUILDDIR)/static/SimpleHX711.o

# Build shared library
$(BUILDDIR)/shared/libhx711.so:	$(BUILDDIR)/shared/HX711.o \
								$(BUILDDIR)/shared/Mass.o \
								$(BUILDDIR)/shared/SimpleHX711.o
	$(CXX)	-shared \
			$(CXXFLAGS) \
			$(INC) \
			$(LIBS) \
			-o $(BUILDDIR)/shared/libhx711.so

.PHONY: hx711calibration
hx711calibration: $(BUILDDIR)/Calibration.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/hx711calibration \
		$(BUILDDIR)/Calibration.o \
		-L $(BUILDDIR)/static \
		-lhx711 $(LIBS)

.PHONY: test
test: $(BUILDDIR)/SimpleHX711Test.o
	$(CXX) $(CXXFLAGS) $(INC) \
		-o $(BINDIR)/simplehx711test \
		$(BUILDDIR)/SimpleHX711Test.o \
		-L $(BUILDDIR)/static \
		-lhx711 $(LIBS)

.PHONY: clean
clean:
ifeq ($(IS_WIN),1)
	del /S /Q $(BUILDDIR)\*
	del /S /Q $(BINDIR)\*
else
	rm -r $(BUILDDIR)/*
	rm -r $(BINDIR)/*
endif

.PHONY: install
install: $(BUILDDIR)/static/libhx711.a $(BUILDDIR)/shared/libhx711.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/static/libhx711.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/shared/libhx711.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/hx711
	install -m 644 $(INCDIR)/*.h $(DESTDIR)$(PREFIX)/include/hx711

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/libhx711.*
	rm -rf $(DESTDIR)$(PREFIX)/include/hx711
