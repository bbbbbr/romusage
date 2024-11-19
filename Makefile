DEL = rm -f


SRCDIR = src
OBJDIR = obj
BINDIR = bin
PACKDIR = package
MKDIRS = $(OBJDIR) $(BINDIR) $(PACKDIR)
# Add all c source files from $(SRCDIR)
# Create the object files in $(OBJDIR)
CFILES = $(wildcard $(SRCDIR)/*.c)
COBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CFILES))

ifdef DRAG_AND_DROP_MODE
	EXTRA_FNAME = _drag_and_drop
	CFLAGS+= -DDRAG_AND_DROP_MODE
endif
BIN = $(BINDIR)/romusage$(EXTRA_FNAME)
BIN_WIN = $(BIN).exe


# ignore package dicrectory that conflicts with rule target
.PHONY: package

all: linux

# Compile .c to .o in a separate directory
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Linux MinGW build for Windows
# (static linking to avoid DLL dependencies)
wincross: TARGET=i686-w64-mingw32
wincross: CC = $(TARGET)-g++
wincross: LDFLAGS = -s -static
wincross: $(COBJ)
	$(CC) -o $(BIN_WIN)  $^ $(LDFLAGS)

# Maxos uses linux target
macos: linux

# Linux build
linux: CC = gcc
linux: LDFLAGS = -s
linux: $(COBJ)
	$(CC) -o $(BIN) $^ $(LDFLAGS)

cleanobj:
	$(DEL) $(COBJ)

clean:
	$(DEL) $(COBJ) $(BIN_WIN) $(BIN)

macos-x64-zip: macos
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(BIN)-macos-x64.zip $(BIN)
	mv $(BIN)-macos-x64.zip $(PACKDIR)
#	cp $(BIN) $(PACKDIR)

macos-arm64-zip: macos
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(BIN)-macos-arm64.zip $(BIN)
	mv $(BIN)-macos-arm64.zip $(PACKDIR)
#	cp $(BIN) $(PACKDIR)

linuxzip: linux
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(BIN)-linux.zip $(BIN)
	mv $(BIN)-linux.zip $(PACKDIR)
#	cp $(BIN) $(PACKDIR)

wincrosszip: wincross
	mkdir -p $(PACKDIR)
	strip $(BIN_WIN)
	# -j discards (junks) path to file
	zip -j $(BIN)-windows.zip $(BIN_WIN)
	mv $(BIN)-windows.zip $(PACKDIR)
#	cp $(BIN_WIN) $(PACKDIR)

package:
	${MAKE} clean
	${MAKE} wincrosszip
	${MAKE} clean
	${MAKE} wincrosszip DRAG_AND_DROP_MODE=TRUE
	${MAKE} clean
	${MAKE} linuxzip


.PHONY: test

test:
	echo "see test-norepo"

updatetest:
	echo "see updatetest-norepo"


test-norepo:
	mkdir -p test_norepo/output
	rm -f  test_norepo/output/test_run.txt
	find test_norepo/gb/* -iname "*.map" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	find test_norepo/gb/* -iname "*.noi" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	find test_norepo/gb/* -iname "*.ihx" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	find test_norepo/gb/* -iname "*.cdb" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	find test_norepo/gb/* -iname "*.gb"  -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	# GG
	find test_norepo/smsgg/* -iname "*.map" -type f | xargs -I {} $(BIN) -p:SMS_GG -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	find test_norepo/smsgg/* -iname "*.noi" -type f | xargs -I {} $(BIN) -p:SMS_GG -g -a "{}" >> test_norepo/output/test_run.txt 2>&1
	find test_norepo/smsgg/* -iname "*.gg"  -type f | xargs -I {} $(BIN) -p:SMS_GG -g -a "{}" >> test_norepo/output/test_run.txt 2>&1

	diff --brief test_norepo/output/test_ref.txt test_norepo/output/test_run.txt

updatetest-norepo:
	rm -f  test_norepo/output/test_ref.txt
	find test_norepo/gb/* -iname "*.map" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	find test_norepo/gb/* -iname "*.noi" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	find test_norepo/gb/* -iname "*.ihx" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	find test_norepo/gb/* -iname "*.cdb" -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	find test_norepo/gb/* -iname "*.gb"  -type f | xargs -I {} $(BIN) -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	# GG
	find test_norepo/smsgg/* -iname "*.map" -type f | xargs -I {} $(BIN) -p:SMS_GG -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	find test_norepo/smsgg/* -iname "*.noi" -type f | xargs -I {} $(BIN) -p:SMS_GG -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1
	find test_norepo/smsgg/* -iname "*.gg"  -type f | xargs -I {} $(BIN) -p:SMS_GG -g -a "{}" >> test_norepo/output/test_ref.txt 2>&1


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
$(info $(shell mkdir -p $(MKDIRS)))
