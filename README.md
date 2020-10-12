romusage
===========

A small command line tool for estimating usage of Game Boy ROMs from map file output.

It produces a trimmed, sorted output of ROM/RAMs, their usage and optionally the Areas located in them.

Runs on Linux and Windows, meant for use with [GBDK 2020](https://github.com/Zal0/gbdk-2020/). If map file output is not already enabled, use either `-Wl-m` with `lcc` or `-m` with `sdldgb` directly.

The usage calculation will attempt to merge overlapping areas to avoid counting shared space multiple times (such as HEADER areas).

Binaries are [here](/bin/)


```
romusage input_file.map [options]

Options
-h  : Show this help
-a  : Show Areas in each Bank
-sH : Show HEADER Areas (normally hidden)

Use: Read a map file to display area sizes.
Example: "romusage build/MyProject.map"

Note: Estimates are as close as possible, but may not be complete.
      They *do not* factor regions lacking complete ranges in
      the Map file, for example Shadow OAM and Stack.
```


Example output for a 32k non-banked ROM, called after completion of the link stage:
```
$ romusage.exe /ReleaseColor/Petris.map -u32k

Bank             Range               Size    Used  Used%    Free  Free%
----------       ----------------   -----   -----  -----   -----  -----
ROM              0x0000 -> 0x3FFF   16384   15989    97%     395     2%
ROM_0            0x4000 -> 0x7FFF   16384   12780    78%    3604    21%
WRAM             0xC000 -> 0xCFFF    4096    3936    96%     160     3%
WRAM_1_0         0xD000 -> 0xDFFF    4096    1893    46%    2203    53%
```

And another example, with display of areas in the banks enabled:
```
romusage gbdk/examples/gb/new_banks.map -a

Bank             Range               Size    Used  Used%    Free  Free%
----------       ----------------   -----   -----  -----   -----  -----
ROM              0x0000 -> 0x3FFF   16384    8211    50%    8173    49%
│
└─_CODE          0x0200 -> 0x111A    3867
└─_HOME          0x111B -> 0x1286     364
└─_BASE          0x1287 -> 0x20F2    3692
└─_GSINIT        0x20F3 -> 0x219C     170
└─_GSINITTAIL    0x219D -> 0x219D       1

ROM_1            0x4000 -> 0x7FFF   16384      19     0%   16365    99%
│
└─_CODE_1        0x4000 -> 0x4012      19

ROM_2            0x4000 -> 0x7FFF   16384      19     0%   16365    99%
│
└─_CODE_2        0x4000 -> 0x4012      19

XRAM_1           0xA000 -> 0xBFFF    8192      22     0%    8170    99%
│
└─_DATA_1        0xA000 -> 0xA015      22

XRAM_2           0xA000 -> 0xBFFF    8192      28     0%    8164    99%
│
└─_DATA_2        0xA000 -> 0xA01B      28

WRAM             0xC000 -> 0xCFFF    4096     102     2%    3994    97%
│
└─_DATA          0xC0A0 -> 0xC0B5      22
└─_BSS           0xC0B6 -> 0xC105      80
```
