### Web Version
A web build that runs in the browser is avaialble at:
- https://bbbbbr.github.io/romusage/

### Note about GBDK-2020
Note: This utility is now included in [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020) (version 4.3.0+), so you may already have a copy if you're using that dev kit.

romusage
===========
### Downloads
 - Releases: See the [Releases](https://github.com/bbbbbr/romusage/releases) section to the right

### Description
A small command line tool for estimating usage (free space) of Game Boy ROMs  (+ Mega Duck, Analogue .pocket, Game Gear, SMS) from the following file types:
- .map (sdcc, rgbds)
- .noi (sdcc)
- .ihx (sdcc)
- .cdb (sdcc)
- .gb / .gbc / .pocket / .duck (ROM image file for: Game Boy / Color, Analogue Pocket, Mega Duck / Cougar Boy)
- .gg / .sms (ROM image file for: Game Gear / Sega Master System. Note: SMS/GG is GBDK-2020 specific)

It produces a trimmed, sorted output of ROM/RAMs, their usage and optionally the Areas located in them.

Runs on Linux, Windows and MacOS, it can be used with [GBDK 2020](https://github.com/Zal0/gbdk-2020/), [ZGB v2020+](https://github.com/Zal0/ZGB/) and [RGBDS](https://github.com/gbdev/rgbds). If map or noi file output is not already enabled, use either `-debug` with `lcc` or `-m` with `sdldgb` directly.

Examples of color output. `-sRe` with `-sRp` for percentage based color on row ends with `-g` small graphs. `-sRd` for section based color with center columns dimmed.
![Romusage Color Examples](/info/romusage_color_examples_linux.png)

### Usage
```
romusage input_file.[map|noi|ihx|cdb|.gb[c]|.pocket|.duck|.gg|.sms] [options]
version 1.2.9, by bbbbbr

Options
-h  : Show this help
-p:SMS_GG : Set platform to GBDK SMS/Game Gear (changes memory map templates)

-a  : Show Areas in each Bank. Optional sort by, address:"-aA" or size:"-aS" 
-g  : Show a small usage graph per bank (-gA for ascii style)
-G  : Show a large usage graph per bank (-GA for ascii style)
-B  : Brief (summarized) output for banked regions. Auto scales max bank
      shows [Region]_[Max Used Bank] / [auto-sized Max Bank Num]
-F  : Force Max ROM and SRAM bank num for -B. (0 based) -F:ROM:SRAM (ex: -F:255:15)

-m  : Manually specify an Area -m:NAME:HEXADDR:HEXLENGTH
-e  : Manually specify an Area that should not overlap -e:NAME:HEXADDR:HEXLENGTH
-E  : All areas are exclusive (except HEADERs), warn for any overlaps
-q  : Quiet, no output except warnings and errors
-Q  : Suppress output of warnings and errors
-R  : Return error code for Area warnings and errors

-sR : [Rainbow] Color output (-sRe for Row Ends, -sRd for Center Dimmed, -sRp % based)
-sP : Custom Color Palette. Colon separated entries are decimal VT100 color codes
      -sP:DEFAULT:ROM:VRAM:SRAM:WRAM:HRAM (section based color only)
-sC : Show Compact Output, hide non-essential columns
-sH : Show HEADER Areas (normally hidden)
-smROM  : Show Merged ROM_0  and ROM_1  output (i.e. bare 32K ROM)
-smWRAM : Show Merged WRAM_0 and WRAM_1 output (i.e DMG/MGB not CGB)
          -sm* compatible with banked ROM_x or WRAM_x when used with -B
-sJ   : Show JSON output. Some options not applicable. When used, -Q recommended
-nB   : Hide warning banner (for .cdb output)
-nA   : Hide areas (shown by default in .cdb output)
-z    : Hide areas smaller than SIZE -z:DECSIZE
-nMEM : Hide banks matching case sensitive substring (ex hide all RAM: -nMEM:RAM)

Use: Read a .map, .noi, .cdb or .ihx file to display area sizes
Example 1: "romusage build/MyProject.map"
Example 2: "romusage build/MyProject.noi -a -e:STACK:DEFF:100 -e:SHADOW_OAM:C000:A0"
Example 3: "romusage build/MyProject.ihx -g"
Example 4: "romusage build/MyProject.map -q -R"
Example 5: "romusage build/MyProject.noi -sR -sP:90:32:90:35:33:36"
Example 6: "romusage build/MyProject.map -sRp -g -B -F:255:15 -smROM -smWRAM"

Notes:
  * GBDK / RGBDS map file format detection is automatic.
  * Estimates are as close as possible, but may not be complete.
    Unless specified with -m/-e they *do not* factor regions lacking
    complete ranges in the Map/Noi/Ihx file, for example Shadow OAM and Stack.
  * IHX files can only detect overlaps, not detect memory region overflows.
  * CDB file output ONLY counts (most) data from C sources.
    It cannot count functions and data from ASM and LIBs,
    so bank totals may be incorrect/missing.
  * GB/GBC/ROM files are just guessing, no promises.
```
### Format notes
The usage calculation will attempt to merge overlapping areas to avoid counting shared space multiple times (such as HEADER areas). Optionally it can warn of overlap in exclusive areas, such as the Stack.

IHX Files:
- For .ihx files bank overflow can only be guessed at (aside from duplicate writes). It's often not possible to tell the difference two banks with data that perfectly aligns on a shared boundary and a single bank that spills over into the unused area of a following bank. It's better to use .map and .noi files to check for overflow.
- Due to their nature, RAM estimates are unavailable with .ihx files

CDB Files:
- To enable .cdb output use the additional debug flags `-Wl-y` with `lcc` or `-y` with `sdldgb` directly.
- For .cdb files the calculated output ONLY reports (most) data from C source files. It cannot count functions and data from ASM sources and LIBs, so bank totals may be incorrect/missing. It's main use is finding the size of individual functions and variables (what's using up space), not estimating the free/used space of banks.

ROM Files (.gb / .gbc / .pocket / .duck / gg / sms) :
- No overflow detection
- Usage estimates can only attempt to distinguish between "empty space" (0xFF's) and data that looks like empty space (0xFF's). It may be inaccurate.


### Examples

Example output with a small graph (-g) for a 32k non-banked ROM, called after completion of the link stage. Manually specify Shadow OAM and Stack as exclusive ranges (-e). Reading from the .map file.
```
$ romusage.exe /ReleaseColor/Petris.map -g -e:STACK:DEFF:100 -e:SHADOW_OAM:C000:A0

Bank           Range             Size   Used   Used%   Free  Free%
----------     ----------------  -----  -----  -----  -----  -----
ROM            0x0000 -> 0x3FFF  16384  15989   97%     395     2% |-###########################|
ROM_0          0x4000 -> 0x7FFF  16384  12843   78%    3541    21% |#####################-......|
WRAM           0xC000 -> 0xCFFF   4096   4096  100%       0     0% |############################|
WRAM_1_0       0xD000 -> 0xDFFF   4096   2150   52%    1946    47% |#############.............-#|

```

Example of quiet mode (-q) that just reports errors and warnings, and can return an error on exit (-R) if any are present.
```
romusage banks.map -R -q

* WARNING: Area _DATA    at  c0a0 -> 115a4 extends past end of address space at  ffff (Underflow error by 5541 bytes)
* WARNING: Area _DATA    at  c0a0 -> 115a4 extends past end of memory region at  dfff (Overflow by 13733 bytes)
* WARNING: Area _CODE_1  at 14000 -> 1c023 extends past end of memory region at 17fff (Overflow by 16420 bytes)
* WARNING: Area _CODE_12 at c4000 -> c801f extends past end of memory region at c7fff (Overflow by 32 bytes)

```


And another example, with display of areas in the banks enabled. Reading from the .noi file.
```
romusage gbdk/examples/gb/new_banks.noi -a

Bank             Range               Size    Used  Used%    Free  Free%
----------       ----------------   -----   -----  -----   -----  -----
ROM              0x0000 -> 0x3FFF   16384    8211    50%    8173    49%
|
+_CODE           0x0200 -> 0x111A    3867
+_HOME           0x111B -> 0x1286     364
+_BASE           0x1287 -> 0x20F2    3692
+_GSINIT         0x20F3 -> 0x219C     170
+_GSINITTAIL     0x219D -> 0x219D       1

ROM_1            0x4000 -> 0x7FFF   16384      19     0%   16365    99%
|
+_CODE_1         0x4000 -> 0x4012      19

ROM_2            0x4000 -> 0x7FFF   16384      19     0%   16365    99%
|
+_CODE_2         0x4000 -> 0x4012      19

XRAM_1           0xA000 -> 0xBFFF    8192      22     0%    8170    99%
|
+_DATA_1         0xA000 -> 0xA015      22

XRAM_2           0xA000 -> 0xBFFF    8192      28     0%    8164    99%
|
+_DATA_2         0xA000 -> 0xA01B      28

WRAM             0xC000 -> 0xCFFF    4096     102     2%    3994    97%
|
+_DATA           0xC0A0 -> 0xC0B5      22
+_BSS            0xC0B6 -> 0xC105      80

```


What's taking up space? Example of .cdb file output showing which functions and variables area larger than 500 bytes (-z:500), with cdb banner noticed suppressed (-nB).
```
 $ romusage Petris_Debug.cdb -z:500 -nB

Bank           Range             Size   Used   Used%   Free  Free% 
----------     ----------------  -----  -----  -----  -----  -----
ROM            0x0000 -> 0x3FFF  16384  16383   99%       1     0%
|
| Name                            Start  -> End      Size 
| ---------------------           ----------------   -----
+ -?-                             0x1E32 -> 0x2937    2822
+ intro_screen_tiles              0x0FB0 -> 0x145F    1200
+ twilight_drive_mod              0x15A2 -> 0x1909     872
+ font_tiles                      0x06D0 -> 0x099F     720
+ pet_tiles                       0x0A50 -> 0x0CDF     656
+ freeost_charselect_mod          0x1BE7 -> 0x1E31     587
+ villainsofhiphop_mod            0x1924 -> 0x1B5C     569
+ -?-                             0x0001 -> 0x01FF     511
+ (102 items < 500 hidden = 8446 total bytes)

ROM_0          0x4000 -> 0x7FFF  16384  11001   67%    5383    32%
|
| Name                            Start  -> End      Size 
| ---------------------           ----------------   -----
+ board_check_completed_pet_xy    0x65DB -> 0x6843     617
+ hinting_petlength_add           0x5807 -> 0x59FB     501
+ (106 items < 500 hidden = 9883 total bytes)

WRAM           0xC000 -> 0xCFFF   4096   4095   99%       1     0%
|
| Name                            Start  -> End      Size 
| ---------------------           ----------------   -----
+ music_decompressed              0xC0ED -> 0xCD08    3100
+ (16 items < 500 hidden = 995 total bytes)

WRAM_1_0       0xD000 -> 0xDFFF   4096   1827   44%    2269    55%
|
| Name                            Start  -> End      Size 
| ---------------------           ----------------   -----
+ (24 items < 500 hidden = 1827 total bytes)
```


Example output with a large graph (-G) for a 32k non-banked ROM, called after completion of the link stage. Manually specify Shadow OAM and Stack as exclusive ranges (-e). Reading from the .map file.
```
$ romusage.exe /ReleaseColor/Petris.map -G -e:STACK:DEFF:100 -e:SHADOW_OAM:C000:A0

Bank           Range             Size   Used   Used%   Free  Free%
----------     ----------------  -----  -----  -----  -----  -----
ROM            0x0000 -> 0x3FFF  16384  15989   97%     395     2%
ROM_0          0x4000 -> 0x7FFF  16384  12843   78%    3541    21%
WRAM           0xC000 -> 0xCFFF   4096   4096  100%       0     0%
WRAM_1_0       0xD000 -> 0xDFFF   4096   2150   52%    1946    47%


Start: ROM    0x0000 -> 0x3FFF
#######-........................################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
End: ROM


Start: ROM_0  0x4000 -> 0x7FFF
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
################################################################
##################################-.............................
................................................................
................................................................
................................................................
End: ROM_0


Start: WRAM    0xC000 -> 0xCFFF
################################################################
################################################################
################################################################
################################################################
End: WRAM


Start: WRAM_1_0  0xD000 -> 0xDFFF
################################################################
######################################################-.........
................................................................
................................................################
End: WRAM_1_0


```
