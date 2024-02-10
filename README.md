# Secure File Remove (SFR)
SFR is a secure file remove tool in c lang.
SFR overwrites file with null or random data, renames file, and finally deletes it.

## Usage

```bash
sfr <files paths ...> [option]
```

## Options
 * -r: write random data instead of null
 * -O: only obfuscate file (do not delete)
 * -R: only obfuscate and rename file (do not delete)
 * -q: quiet output (no output) fastests
 * -h: display help and exit

## Example
remove: file1 file2 file3, quietly, with random data
```bash
sfr file1 file2 file3 -qr
```
or
```bash
sfr file1 file2 file3 -q -r
```

## Build
this project does not use makefile.
compile directly `./src/sfr.c` file.
> the program can be compiled with tcc, recommended for faster compile & smaller binary (perfect for portable use)

# Simple Secure Remove (SSR)
SSR is same as SFR but is quiet and null data only with no options.
only error messages are printed.
```bash
ssr <files paths ...>
```

note: SSR can be compiled to force it to be completely silent (no output at all) by defining `SILENT` macro.

# License
this project is licensed under the MIT License see [LICENSE](LICENSE) file for details.

# Author
lucasIov https://github.com/lucasIov/

