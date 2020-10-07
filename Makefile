DEL = rm -f

# Add all c source files
CFILES = $(wildcard *.c)
# Remove some files that won't/can't be used
COBJ = $(CFILES:.c=.o)


info:
	@echo "--> Please specify target, 'make romusage' (Linux build) or 'make romusage.exe' (MinGW Windows build)"


# Linux MinGW build for Windows
# (static linking to avoid DLL dependencies)
romusage.exe : TARGET=i686-w64-mingw32
romusage.exe : CC = $(TARGET)-g++
romusage.exe : LDFLAGS = -s -static
romusage.exe: $(COBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Linux build
romusage : CC = gcc
romusage : LDFLAGS = -s
romusage: $(COBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

cleanobj:
	$(DEL) $(COBJ)

clean:
	$(DEL) $(COBJ) romusage.exe romusage

