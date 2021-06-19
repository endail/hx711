CXX := g++
INCDIR := include
SRCDIR := src
BUILDDIR := build
BINDIR := bin
SRCEXT := cpp
LIBS := -llgpio
INC := -I $(INCDIR)
CFLAGS :=	-O2 \
			-pipe \
			-fomit-frame-pointer \
			-Wall \
			-Wfatal-errors \
			-Werror=format-security \
			-Wl,-z,relro \
			-Wl,-z,now \
			-Wl,-z,defs	\
			-Wl,--hash-style=gnu \
			-Wl,--as-needed \
			-D_FORTIFY_SOURCE=2 \
			-fstack-clash-protection


########################################################################

# https://stackoverflow.com/a/39895302/570787
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif


ifeq ($(GITHUB_ACTIONS),true)
# gha needs these additional libs
	LIBS := $(LIBS) -lrt -lcrypt -pthread
else
# only include these flags on rpi, not gha
	CFLAGS := 	-march=native \
				-mfpu=vfp \
				-mfloat-abi=hard \
				$(CFLAGS)
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
	mkdir -p $(BINDIR)
	mkdir -p $(BUILDDIR)/static
	mkdir -p $(BUILDDIR)/shared

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
	ar rcs 		$(BUILDDIR)/static/libhx711.a \
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
	$(RM) -r $(BUILDDIR)/*
	$(RM) -r $(BINDIR)/*

.PHONY: install
install: $(BUILDDIR)/static/libhx711.a $(BUILDDIR)/shared/libhx711.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/static/libhx711.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/shared/libhx711.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/hx711
	install -m 644 $(INCDIR)/*.h $(DESTDIR)$(PREFIX)/include/hx711
