CC := g++
SRCDIR := src
BUILDDIR := build
BINDIR := bin
TESTDIR := test
OUTPUTFILE := libhx711.a
INSTALLDIR := .

# https://stackoverflow.com/a/39895302/570787
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -g -Wall
LIB := -lwiringPi
INC := -I include


.PHONY: all
all: $(OUTPUTFILE)

$(OUTPUTFILE): $(BUILDDIR)/HX711.o
	ar rcs $(BUILDDIR)/$(OUTPUTFILE) $(BUILDDIR)/HX711.o

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

.PHONY: hx711calibration
hx711calibration: $(BUILDDIR)/Calibration.o
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(INC) -o $(BINDIR)/hx711calibration $(BUILDDIR)/Calibration.o -L. -l hx711 $(LIB)

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)/*

.PHONY: install
install:
	mkdir -p $(INSTALLDIR)
	cp -p $(BUILDDIR)/$(OUTPUTFILE) $(INSTALLDIR)

.PHONY: test
test:
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(INC) -c $(TESTDIR)/main.$(SRCEXT) -o $(BUILDDIR)/main.o
	$(CC) $(CFLAGS) $(INC) -o $(BINDIR)/test $(BUILDDIR)/main.o -L. -l hx711 $(LIB)	

