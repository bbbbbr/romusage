
DEL = rm -f

info:
	@echo "--> Please specify target, 'make romusage' (Linux build) or 'make romusage.exe' (MinGW Windows build)"

# Linux MinGW build for Windows
# (static linking to avoid DLL dependencies)
romusage.exe : TARGET=i686-w64-mingw32
romusage.exe : CC = $(TARGET)-g++
romusage.exe : LDFLAGS = -s -static
romusage.exe:
	$(CC) -o romusage.exe romusage.c $(LDFLAGS)

# Linux build
romusage : CC = gcc
romusage : LDFLAGS = -s
romusage:
	$(CC) -o romusage romusage.c $(LDFLAGS)

cleanobj:
	$(DEL) $(COBJ)

clean:
	$(DEL) romusage.o romusage.exe romusage

