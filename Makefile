CC := g++
SRCDIR := src
BUILDDIR := build
BINDIR := bin

# https://stackoverflow.com/a/39895302/570787
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

SRCEXT := cpp
#SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
#OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -g -Wall
LIB := -l wiringPi
INC := -I include

# make all
# make install

.PHONY: all
all: dirs $(BUILDDIR)/libhx711.a test hx711calibration

.PHONY: dirs
dirs:
	mkdir -p $(BINDIR)
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/libhx711.a: $(BUILDDIR)/HX711.o
	ar rcs $(BUILDDIR)/libhx711.a $(BUILDDIR)/HX711.o

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

.PHONY: hx711calibration
hx711calibration: $(BUILDDIR)/Calibration.o
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $(BINDIR)/hx711calibration $(BUILDDIR)/Calibration.o -L $(BUILDDIR)/ -l hx711

.PHONY: test
test: $(BUILDDIR)/test.o
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $(BINDIR)/test $(BUILDDIR)/test.o -L $(BUILDDIR)/ -l hx711	

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)/*
	$(RM) -r $(BINDIR)/*

.PHONY: install
install: $(BUILDDIR)/$(LIBFILE)
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(BUILDDIR)/$(LIBFILE) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(INC)/$(CLASSNAME).h $(DESTDIR)$(PREFIX)/include/
	install -d /usr/local/bin
	install -m 755 $(BINDIR)/hx711calibration /usr/local/bin



#.PHONY: all
#all: $(LIBFILE)

#$(OUTPUTFILE): $(BUILDDIR)/HX711.o
#	ar rcs $(BUILDDIR)/$(LIBFILE) $(BUILDDIR)/HX711.o

#$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
#	mkdir -p $(BUILDDIR)
#	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

#.PHONY: hx711calibration
#hx711calibration: $(BUILDDIR)/Calibration.o
#	mkdir -p $(BINDIR)
#	$(CC) $(CFLAGS) $(INC) -o $(BINDIR)/hx711calibration $(BUILDDIR)/Calibration.o -L. -l hx711 $(LIB)

#.PHONY: clean
#clean:
#	$(RM) -r $(BUILDDIR)/*
#	$(RM) -r $(BINDIR)/*

#.PHONY: install
#install:
#	mkdir -p $(INSTALLDIR)
#	cp -p $(BUILDDIR)/$(OUTPUTFILE) $(INSTALLDIR)

#.PHONY: install
#install: $(BUILDDIR)/$(LIBFILE)
#	install -d $(DESTDIR)$(PREFIX)/lib/
#	install -m 644 $(BUILDDIR)/$(LIBFILE) $(DESTDIR)$(PREFIX)/lib/
#	install -d $(DESTDIR)$(PREFIX)/include/
#	install -m 644 $(INC)/HX711.h $(DESTDIR)$(PREFIX)/include/


#.PHONY: test
#test:
#	mkdir -p $(BINDIR)
#	$(CC) $(CFLAGS) $(INC) -c $(TESTDIR)/main.$(SRCEXT) -o $(BUILDDIR)/main.o
#	$(CC) $(CFLAGS) $(INC) -o $(BINDIR)/test $(BUILDDIR)/main.o -L. -l hx711 $(LIB)	

