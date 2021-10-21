# Specify extensions of files to delete when cleaning
CLEANEXTS  = o so

# Specify the source files, the target files, 
# and the install directory 
CSOURCES = awn.c statistics.c tree.c debug.c mem.c
OFILE = $(subst .c,.o, $(CSOURCES))
OFILE += $(subst .S,.o, $(SSOURCES))
OUTPUTFILE = libawn.so
BASEOUTPUTFILE = libawn.so
HEADERFILE = awn.h
SOINSTALLDIR = /usr/local/lib
HEADINSTALLDIR = /usr/local/include/awn
#CC = /usr/bin/gcc-7

.PHONY: all
all: CCFLAGS += -O3 -UBASE_VERSION
all: LDFLAGS	= -ldl -fcommon
all: SSOURCES = memcmp-avx2-addr.S
#all: $(OUTPUTFILE)
all: compile

.PHONY: debug
debug: CCFLAGS += -ggdb -UBASE_VERSION
debug: LDFLAGS	= -ldl
debug: SSOURCES = memcmp-avx2-addr.S
#debug: $(OUTPUTFILE)
debug: compile

.PHONY: base
base: CCFLAGS += -O3 -DBASE_VERSION 
base: baseCompile

.PHONY: basedebug
basedebug: CCFLAGS += -ggdb -DBASE_VERSION 
basedebug:  baseCompile
# Build libgeorgeringo.so from george.o, ringo.o, 
# and georgeringo.o; subst is the search-and-replace 
# function demonstrated in Recipe 1.16

#$(OUTPUTFILE): CFILE SFILE
#	$(CC) $(CCFLAGS) -shared -fPIC $(LDFLAGS) -o $@ $(OFILE)

compile: CFILE SFILE
	$(CC) $(CCFLAGS) -shared -fPIC $(LDFLAGS) -o $(OUTPUTFILE) $(OFILE)

baseCompile: CFILE
	$(CC) $(CCFLAGS) -shared -fPIC $(LDFLAGS) -o libawn.so $(OFILE)

CFILE: $(CSOURCES)
	$(CC) $(CCFLAGS) -Wall -fPIC -c $(LDFLAGS) $(CSOURCES)

SFILE : $(SSOURCES)
	$(CC) $(CCFLAGS) -Wall -fPIC -c $(LDFLAGS) $(SSOURCES)

#%.o: %.c
#	$(CC) -Wall -fPIC -c $(LDFLAGS) -o $@ $^

.PHONY: install
install:
	mkdir -p $(SOINSTALLDIR)
	mkdir -p $(HEADINSTALLDIR)
	cp -p $(OUTPUTFILE) $(SOINSTALLDIR)
	cp -p $(HEADERFILE) $(HEADINSTALLDIR)

## Generate dependencies of .ccp files on .hpp files
#include $(subst .cpp,.d,$(CSOURCES))
#
#%.d: %.cpp
#	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
#	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
#rm -f $@.$$$$
.PHONY: clean 
clean:
	rm -f *.o *.so
