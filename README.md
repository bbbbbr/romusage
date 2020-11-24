romusage
===========

A small command line tool for estimating usage of Game Boy ROMs from a .map, .noi or ihx file.

It produces a trimmed, sorted output of ROM/RAMs, their usage and optionally the Areas located in them.

Runs on Linux and Windows, it can be used with [GBDK 2020](https://github.com/Zal0/gbdk-2020/), [ZGB v2020+](https://github.com/Zal0/ZGB/) and [RGBDS](https://github.com/gbdev/rgbds). If map file output is not already enabled, use either `-Wl-m` with `lcc` or `-m` with `sdldgb` directly.

The usage calculation will attempt to merge overlapping areas to avoid counting shared space multiple times (such as HEADER areas). Optionally it can warn of overlap in exclusive areas, such as the Stack.

IHX Files:
- For .ihx files bank overflow can only be guessed at (aside from duplicate writes). It's often not possible to tell the difference two banks with data that perfectly aligns on a shared boundary and a single bank that spills over into the unused area of a following bank. It's better to use .map and .noi files to check for overflow.
- Due to their nature, RAM estimates are unavailable with .ihx files

Binaries are [here](/bin/)


```
romusage input_file.[map|noi|ihx] [options]

Options
-h  : Show this help
-a  : Show Areas in each Bank
-sH : Show HEADER Areas (normally hidden)
-g  : Show a small usage graph per bank
-G  : Show a large usage graph per bank
-m  : Manually specify an Area -m:NAME:HEXADDR:HEXLENGTH
-e  : Manually specify an Area that should not overlap -e:NAME:HEXADDR:HEXLENGTH
-E  : All areas are exclusive (except HEADERs), warn for any overlaps
-q  : Quiet, no output except warnings and errors
-R  : Return error code for Area warnings and errors

Use: Read a .map, .noi or .ihx file to display area sizes.
Example 1: "romusage build/MyProject.map"
Example 2: "romusage build/MyProject.noi -a -e:STACK:DEFF:100 -e:SHADOW_OAM:C000:A0"
Example 3: "romusage build/MyProject.ihx -g"
Example 4: "romusage build/MyProject.map -q -R"

Notes:
  * GBDK / RGBDS map file format detection is automatic.
  * Estimates are as close as possible, but may not be complete.
    Unless specified with -m/-e they *do not* factor regions lacking
    complete ranges in the Map/Noi/Ihx file, for example Shadow OAM and Stack.
  * IHX files can only detect overlaps, not detect memory region overflows.

```


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

Example of quiet mode (-q) that just reports errors and warnings, and can returns an error on exit (-R) if any are present.
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