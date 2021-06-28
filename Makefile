DEL = rm -f

# Add all c source files
CFILES = $(wildcard *.c)
# Remove some files that won't/can't be used
COBJ = $(CFILES:.c=.o)

BIN = romusage
BIN_WIN = $(BIN).exe


all: linux

info:
	@echo "--> Please specify target, 'make linux' or 'make wincross' (MinGW Windows build)"


# Linux MinGW build for Windows
# (static linking to avoid DLL dependencies)
wincross: TARGET=i686-w64-mingw32
wincross: CC = $(TARGET)-g++
wincross: LDFLAGS = -s -static
wincross: $(COBJ)
	$(CC) -o $(BIN_WIN)  $^ $(LDFLAGS)

# Linux build
macos: linux
linux: CC = gcc
linux: LDFLAGS = -s
linux: $(COBJ)
	$(CC) -o $(BIN) $^ $(LDFLAGS)

cleanobj:
	$(DEL) $(COBJ)

clean:
	$(DEL) $(COBJ) $(BIN_WIN) $(BIN)

macoszip: macos
	mkdir -p bin
	zip $(BIN)-macos.zip $(BIN)
	mv $(BIN)-macos.zip bin
	cp $(BIN) bin

linuxzip: linux
	mkdir -p bin
	zip $(BIN)-linux.zip $(BIN)
	mv $(BIN)-linux.zip bin
	cp $(BIN) bin

wincrosszip: wincross
	mkdir -p bin
	zip $(BIN)-windows.zip $(BIN_WIN)
	mv $(BIN)-windows.zip bin
	cp $(BIN_WIN) bin

package:
	${MAKE} clean
	${MAKE} wincrosszip
	${MAKE} clean
	${MAKE} linuxzip
