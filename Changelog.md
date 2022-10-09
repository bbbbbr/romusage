Changelog
=========

# Version 1.2.3
* `-sR` : [Rainbow] Color output
  - `-sRe` : Row Ends only
  - `-sRd` : Middle Dimmed
  - `-sRp` : Percentage based coloring
* `-sR` : Custom Color Palette
  -  Colon separated entries are decimal VT100 color codes
  -  Used for section based color mode only, not Percetange based coloring
  - `-sp:DEFAULT:ROM:VRAM:SRAM:WRAM:HRAM`

# Version 1.2.2
* Add support for RGBDS v0.6.0 map file spaces -> tab change
* Fix rejection of short file names (ex: a.gb)
* Changed:
  - XRAM -> SRAM
  - WRAM -> WRAM_LO
  - WRAM_1_* -> WRAM_HI_*
  - ROM (Bank 0) -> ROM_0
  - ROM_0 (Bank 0 overflow) -> ROM_1 (will clash and warn of overflow)
* -E overlap / multiple write flag
  - Additional header area warning suppression
  - Improved warning format
* Added -sC : Show Compact Output (by Evie)
* Added rom .duck extension for megaduck ROMs
* Building: added Github Runner build workflow for Linux/Windows & Mac OSX

# Version 1.2.1
* Adds ascii block style option for small and large graphs (-gA and -GA)
* Make file extension detection case insensitive

# romusage 1.2.0
* Adds GB/GBC ROM File support
* Fix argument handling that might incorrectly match a parameter

# Version 1.1.2
* Minor argument handling fixes

# Version 1.1.1
* Turned on HRAM area for RGBDS

# Version 1.1
* Fixed bank totals that were sometimes incorrect (Windows/MinGW)
* Fixed sorting that was sometimes incomplete (Linux & Windows/MinGW)
* Added .cdb file support (defaults to `-aS`)
* Added `-aA` : Sort areas by Address Ascending 
* Added `-aS` : Sort areas by Size Descending
* Added `-nB` : Hide warning banner (for .cdb output)
* Added `-nA` : Hide areas (shown by default in .cdb output)
* Added `-z`  : Hide areas smaller than SIZE. Example: hide areas smaller than 256 bytes `-z:256`


# Version 1.0
* .map file support
* .noi file support
* .noi file support
* Large and small graphs
