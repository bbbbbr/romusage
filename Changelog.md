Changelog
=========

# Version 1.3.2
- Added Linux Arm 64 build
- Fixed incorrect array growth size
- Fix missing error when filename not present

# Version 1.3.1
- Experimental support for GBDK's flavor of NES (`-p:NES1"`) .noi/.map files
- Allow areas with leading underscores for .noi/.map files
- Fixed Brief/summarized mode: overlapped areas in header areas counted multiple times when copying banks

# Version 1.3.0
- `-b:HEXVAL:[...]` Set hex bytes treated as Empty in ROM files (.gb/etc) (default `FF`(255), ex use 255 AND 0: `-b:FF:00`
- Improve error messaging
- Allow filename at any location in option arguments

# Version 1.2.9
- `-nMEM` Hide memory regions with case sensitive substring (ex hide all RAM: `-nMEM:RAM`)

# Version 1.2.8
- `-sJ` JSON output
- `-Q` Suppress output of warnings and errors

# Version 1.2.7
- Attempt to warn when regions such as `_HOME` or `_CODE` have overflowed ROM0 and rom size is larger than 32k

# Version 1.2.6
- Added support for GBDK's flavor of Game Gear/SMS (`-p:SMS_GG"`) .noi/.map files (with `_LIT` and `_CODE` areas)
- Fix: free % + used% did not always equal 100%

# Version 1.2.5
- Added `-B` : Brief (summarized) mode:
  - Useful with large numbers of banks
  - Collapses banked regions to single lines: [Region]_[Max Used Bank] / [auto-sized Max Bank Num]
    - Example: ROM_1...ROM_12 -> ROM_12/15
  - Range column is unmodified. Size, used, free, graph, are adjusted to represent total size of merged banks

- Added `-F` : Force Displayed Max ROM and SRAM bank num for -B. (0 based) -F:ROM:SRAM (ex: -F:255:15)

- Added combined non-banked region + banked to a single line mode
  - `-smWRAM` : Combined WRAM_0 and WRAM_1 display (i.e DMG/MGB not CGB)
  - `-smROM`  : Combined ROM_0 and ROM_1 display (i.e. bare 32K ROM)
  - Compatible with banked ROM_x or WRAM_x when used with `-B`

- Added Windows "Drag and Drop" build
  - Allows drag-and-drop a ROM/.noi/etc file onto the console `romusage_drag_and_drop.exe` program
  - Romusage will load the file, open a console window to show output and wait for a key press before closing
  - Uses defaults (it can't accept option settings) : `-sRp -g -B`

# Version 1.2.4
- Fix: Support spaces in RGBDS map file section (area) names

# Version 1.2.3
- Added `-sR` : [Rainbow] Color output
  - `-sRe` : Row Ends only
  - `-sRd` : Middle Dimmed
  - `-sRp` : Percentage based coloring
- Added `-sP` : Custom Color Palette
  -  Colon separated entries are decimal VT100 color codes
  -  Used for section based color mode only, not Percetange based coloring
  - `-sP:DEFAULT:ROM:VRAM:SRAM:WRAM:HRAM`

# Version 1.2.2
- Added support for RGBDS v0.6.0 map file spaces -> tab change
- Fixed rejection of short file names (ex: a.gb)
- Changed:
  - XRAM -> SRAM
  - WRAM -> WRAM_LO
  - WRAM_1_* -> WRAM_HI_*
  - ROM (Bank 0) -> ROM_0
  - ROM_0 (Bank 0 overflow) -> ROM_1 (will clash and warn of overflow)
- Added `-E` overlap / multiple write flag
  - Additional header area warning suppression
  - Improved warning format
- Added `-sC` : Show Compact Output (by Evie)
- Added rom .duck extension for megaduck ROMs
- Building: added Github Runner build workflow for Linux/Windows & Mac OSX

# Version 1.2.1
- Adds ascii block style option for small and large graphs (-gA and -GA)
- Make file extension detection case insensitive

# romusage 1.2.0
- Adds GB/GBC ROM File support
- Fix argument handling that might incorrectly match a parameter

# Version 1.1.2
- Minor argument handling fixes

# Version 1.1.1
- Turned on HRAM area for RGBDS

# Version 1.1
- Fixed bank totals that were sometimes incorrect (Windows/MinGW)
- Fixed sorting that was sometimes incomplete (Linux & Windows/MinGW)
- Added .cdb file support (defaults to `-aS`)
- Added `-aA` : Sort areas by Address Ascending 
- Added `-aS` : Sort areas by Size Descending
- Added `-nB` : Hide warning banner (for .cdb output)
- Added `-nA` : Hide areas (shown by default in .cdb output)
- Added `-z`  : Hide areas smaller than SIZE. Example: hide areas smaller than 256 bytes `-z:256`

# Version 1.0
- .map file support
- .noi file support
- .noi file support
- Large and small graphs
