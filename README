romusage
===========

A small command line tool for estimating usage of Game Boy ROMs from map file output.

It produces a trimmed, sorted output of the Areas from the map file.

Runs on Linux and Windows, meant for use with [GBDK 2020](https://github.com/Zal0/gbdk-2020/). If map file is not enabled, use either `-Wl-m` with `lcc` or `-m` with `sdldgb` directly.

Binaries are in [here](/bin/)

TODO: Calculate actual use of banks based on address and size using all areas.

```
romusage input_file.map [options]

Options
-h : Show this help
-u32k : Estimate usage with 32k Areas
-u16k : Estimate usage with 16k Areas

Use: Read a map file to display area sizes.
Example: "romusage build/MyProject.map"

Note: Usage estimates are for a given Area only.
      They **do not** factor in whether multiple areas share
      the same bank (such as HOME, CODE, GS_INIT, etc).
```


Example output for a 32k non-banked ROM, called after completion of the link stage:
```
$ romusage.exe /ReleaseColor/Petris.map -u32k
Area        Addr                Size         Used  Remains
-----       -----------------   -----------  ----  -------
_BASE          6d4f ->   6ff1     675 bytes
_BSS           d725 ->   d765      65 bytes
_CODE           200 ->   6c61   27234 bytes  %83    5533
_DATA          c0a0 ->   d724    5765 bytes
_GSINIT        6ff2 ->   71ea     505 bytes
_GSINITTAIL    71eb ->   71eb       1 bytes
_HOME          6c62 ->   6d4e     237 bytes
_HRAM10           0 ->      0       1 bytes
```

And another example:
```
romusage gbdk/examples/gb/banks.map

Area        Addr                Size
-----       -----------------   -----------
_BASE          10fe ->   1f7a    3709 bytes
_BSS           c0a2 ->   c0f1      80 bytes
_CODE           200 ->    fb7    3512 bytes
_CODE_1       14000 ->  1401b      28 bytes
_CODE_2       24000 ->  2401b      28 bytes
_CODE_3       34000 ->  3401b      28 bytes
_DATA          c0a0 ->   c0a1       2 bytes
_DATA_0        a000 ->   a001       2 bytes
_DATA_1       1a000 ->  1a001       2 bytes
_DATA_2       2a000 ->  2a001       2 bytes
_DATA_3       3a000 ->  3a001       2 bytes
_GSINITTAIL    1f7b ->   1f7b       1 bytes
_HOME           fb8 ->   10fd     326 bytes
_HRAM10           0 ->      0       1 bytes
```
