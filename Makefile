CC := g++
SRCDIR := src
BUILDDIR := build
BINDIR := bin
TESTDIR := test
OUTPUTFILE := libhx711.a
INSTALLDIR := .

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -g -Wall
LIB := -lwiringPi
INC := -I include


.PHONY: all
all: $(OUTPUTFILE)

$(OUTPUTFILE): $(BUILDDIR)/HX711.o
	@echo " ar rcs $(BUILDDIR)/$(OUTPUTFILE) $(BUILDDIR)/HX711.o"; ar rcs $(BUILDDIR)/$(OUTPUTFILE) $(BUILDDIR)/HX711.o

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@echo " Compiling..."
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

.PHONY: clean
clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR)"; $(RM) -r $(BUILDDIR)

.PHONY: install
install:
	@echo " Installing..."
	@echo " mkdir -p $(INSTALLDIR)"; mkdir -p $(INSTALLDIR)
	@echo " cp -p $(BUILDDIR)/$(OUTPUTFILE) $(INSTALLDIR)"; cp -p $(BUILDDIR)/$(OUTPUTFILE) $(INSTALLDIR)

.PHONY: test
test:
	$(CC) $(CFLAGS) $(INC) -c $(TESTDIR)/main.$(SRCEXT) -o $(BUILDDIR)/main.o
	$(CC) $(CFLAGS) $(INC) -o $(BINDIR)/test $(BUILDDIR)/main.o -L. -l hx711 $(LIB)	
