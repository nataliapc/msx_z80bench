# Changelog

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
