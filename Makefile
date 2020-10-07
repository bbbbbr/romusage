
DEL = rm -f
SRCDIR = src
OUTDIR = bin
INCS = -I"$(SRCDIR)"
CFLAGS = $(INCS)

# Add all c source files
CFILES = $(wildcard src/*.c)
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
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) -o $(OUTDIR)/$@ $^ $(LDFLAGS)

# Linux build
romusage : CC = gcc
romusage : LDFLAGS = -s
romusage: $(COBJ)
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) -o $(OUTDIR)/$@ $^ $(LDFLAGS)

cleanobj:
	$(DEL) $(COBJ)

clean:
	$(DEL) $(COBJ) $(OUTDIR)/romusage.exe $(OUTDIR)/romusage

