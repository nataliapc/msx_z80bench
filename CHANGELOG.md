# Changelog

## [1.5.2] - 2026-10-19

### Added

- Detection of the _MSXnano_ and _MSXimus_ FPGA boards, shown as the machine
  brand: `MSXnano`, `MSXimus 60K`, `MSXimus 138K` and `MSXimus Z`. They have
  the config device of the goa'uld core (ID `48h` in the expanded I/O port
  `40h`), and the board comes from the core version in I/O port `2Fh`:
  `1X`-`2X` MSXnano, `3X`-`9X` MSXimus on Tang Console, `A0` and up MSXimus Z
  (Zynq). The 60K and 138K cores give the same version, so the 60K is told
  apart by its I/O ports `20h`-`27h` (last gamepad report, since 3.7.2), which
  read `FFh` on the 138K. An older Tang Console core is shown as `MSXimus`.
  Nothing is written to these boards except the device selection in port
  `40h`, which is restored.

### Fixed

- The MSXnano and MSXimus boards were shown as `OCM/MSX++`, with the F3 option
  to cycle the OCM speeds. They answer to the OCM device ID (`D4h`) when their
  megaram is on, but the speed commands do nothing there. F3 is no longer
  shown on these boards. Their turbo is the Panasonic one, so F1 is shown.
- The F4 Tides-Rider option was shown on the MSXnano and MSXimus boards. Their
  RTC (the OCM `rtc.v`) reads register 14 of block 2 as a RAM nibble with the
  high nibble at `F`, not `FFh` like the RP5C01, so the Tides-Rider detection
  saw a board. It is skipped on these boards, so the program no longer writes
  to their RTC.

## [1.5.1] - 2026-10-19

### Changed

- The program is built with SDCC 4.5.0 (it was 4.2.0), using the Docker image
  `nataliapc/sdcc:4.5.0`.
- `hex2bin` now runs inside the same Docker image, so the build no longer needs
  a host-installed `hex2bin`. The `conio` and `dos` libraries are built with
  `make -j` (thanks to @Pax-nl).
- The `sdcc_msxconio` and `sdcc_msxdos` submodules are updated, and the
  `conio.h`, `conio_aux.h` and `dos.h` headers are synced with them: empty
  parameter lists are now `(void)`, as SDCC 4.5.0 requires, and `dos.h` adds
  the DOS 2 functions for rename, move, attributes, change directory, IOCTL,
  EOF, file size and file exists.

## [1.5.0] - 2026-10-18

### Added

- Two memory speed tests, selected with F5 (CPU speed, Mem 8bit, Mem 16bit).
  They show the memory speed in kB/s (1 kB = 1000 bytes): the bytes moved by
  the test loop divided by its run time. The README explains the calculation.
  - Mem 8bit: reads and writes RAM one byte at a time (`LD A,(DE)` /
    `LD (DE),A`).
  - Mem 16bit: reads and writes RAM one word at a time with `POP DE` /
    `PUSH DE` on a stack at an even address. A CPU with a 16-bit data bus,
    like the Z380 in the XRX, can do each access in one bus cycle.
- The graph has a turboR R800 (DRAM mode) reference bar, next to MSX standard
  and MSX2+ TurboPana, with the value of the selected test (CPU speed
  20.59 MHz, Mem 8bit 1254 kB/s, Mem 16bit 1933 kB/s, measured with openMSX
  on a Panasonic FS-A1ST).
- The debug mode (`z80bench d`) runs the three tests. It shows every result as
  the equivalent Z80 clock (MHz), used to check the calibration.

## [1.4.3] - 2026-10-18

### Added

- Z380 CPU detection, run before the other CPU tests. On the Z380 the opcode
  `CB 37` is `EX A,A'`, so a marker stored in `A'` shows up in `A`. Other CPUs
  change `A` in place (`SLL A` on the Z80, `TSET A` on the Z280). "Z380" is
  shown as the CPU type.

### Fixed

- The program did nothing on Z380 machines (such as the XRX) and went back to
  the DOS prompt. The R800 test runs `MULUB A,C` (`ED C9`), which is not a Z380
  opcode. The Z380 traps undefined opcodes, so it jumped to address `0000h`
  (DOS warm boot). The R800 and Z280 tests are now skipped on a Z380.
- The NMOS/CMOS test is skipped on a Z380. Its `OUT (C),0` (`ED 71`) is the
  3-byte `OUT (C),n` on that CPU. It took the next opcode as its data byte and
  the routine then returned to a wrong address.

### Changed

- On a Z380 the Tides-Rider detection is skipped, so the program no longer
  writes to the RTC (it selected block 2 in mode register 13 and left the
  timer disabled). The F4 Tides-Rider option is not shown.
