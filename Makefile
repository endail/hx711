CXX := g++
INCDIR := include
SRCDIR := src
BUILDDIR := build
BINDIR := bin
SRCEXT := cpp
LIBS := -lwiringPi
INC := -I $(INCDIR)
CXXFLAGS := -std=c++11 \
			-Wall \
			-Wno-psabi \
			-O2 \
			-D_FORTIFY_SOURCE=2 \
			-D_GLIBCXX_ASSERTIONS \
			-fexceptions \
			-fstack-clash-protection \
			-pipe \
			-Werror=format-security \
			-Wl,-z,defs	\
			-Wl,-z,now \
			-Wl,-z,relro \
			-fwrapv

# https://stackoverflow.com/a/39895302/570787
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

# Add additional libs for building on github actions
ifeq ($(GITHUB_ACTIONS),true)
	LIBS := $(LIB) -lrt -lcrypt -pthread
endif

.PHONY: all
all: dirs $(BUILDDIR)/static/libhx711.a $(BUILDDIR)/shared/libhx711.so hx711calibration test

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
			-o $(BUILDDIR)/libhx711.so



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
		-lhx711  $(LIBS)

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
