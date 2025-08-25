# MSX Z80 Frequency Benchmark

[[**Downloads page**](https://github.com/nataliapc/msx_z80bench/releases)]

**Z80BENCH** program allows you to measure and manage your MSX CPU speed.

It has been created with not only _Panasonic MSX_ with _TurboPana_ and _MSX TurboR_ machines in mind, but also _FPGA_ machines [_OCM/MSX++_](https://github.com/gnogni/ocm-pld-dev), and MSX based on boards and hardware developments like [_Tides-Rider_](https://genami.shop/blogs/news/requirements-to-assemble-a-tides-rider) and [_Taka-Nami_](https://genami.shop/blogs/news/knowing-the-taka-nami) that allow different CPU speeds.

The measurement is approximate and is based on the number of _VDP_ interrupts that occur during a test loop in `IM1` mode.

This means that for low clock frequencies the precision is higher and as the speed increases the margin of error also increases. But in the range of 3 to 20 MHz this error is practically negligible.

With longer test loops and longer duration, the margin of error would be reduced, but a compromise between precision and usability has been chosen.

The available options vary depending on which _MSX_ machine you run the program on.

The visual presentation and operation is based on similar programs that were developed for old _PCs_ under _MS-DOS_.

**🌟 You can help to improve this project by [starring](#more-stars) it! 🌟**

![ocminfo panels](.images/screen.jpg)

## Displayed Information

- **Machine:** MSX generation obtained from _BIOS ROM_ and manufacturer if available obtained from [_expanded I/O ports_](https://map.grauw.nl/resources/msx_io_ports.php#expanded_io).
- **CPU Type:** detection is performed by checking how specific opcodes behave for each CPU. `Z80`, `R800`, and `Z280` (experimental) are detected. For `Z80` it also detects if it's `NMOS` or `CMOS`.
- **CPU Speed:** speed calculated by the test loop.
- **VDP Type:** video chip detection (_TMS9918, V9938, V9958_). Output frequency is also shown (_NTSC/PAL_).

## Options

The options are only shown if they are detected as possible.

The keys must be held down until you hear a click sound, which is when each of the tests ends and keyboard can be read again.

- **F1:** Toggle _TurboPana_ CPU speed (_3.57MHz, and 5.36MHz_).
- **F2:** Cycle _TurboR_ CPU (_Z80, R800(ROM), and R800(DRAM)_).
- **F3:** Cycle _OCM_ Speed (_3.57MHz, 5.36MHz, 4.10MHz, 4.48MHz, 4.90MHz, 5.39MHz, 6.10MHz, 9.96MHz, and 8.06MHz_).
- **F4:** Cycle _Tides-Rider_ Speed (3.57MHz, 6.66MHz, 10MHz, and 20MHz).
- **F6:** Toggle _NTSC/PAL_. CPU Speed may vary slightly when changing this value due to the different interrupts frequency (_60/50Hz_ respectively).

## Final Considerations

Clock measurement is approximate, and may vary when using external RAM mappers.

The number that appears in the border indicates the number of interrupts (x 1e6) that occurred during each iteration of the test loop.

## Acknowledgments

Thank you very much to everyone who has been testing the program so that it could become a reality.

### I hope you all find this useful! :D
NataliaPC

## More stars!

Please give us a star on [GitHub](https://github.com/nataliapc/msx_z80bench) if you like this project.

[![Star History Chart](https://api.star-history.com/svg?repos=nataliapc/msx_z80bench&type=Date&theme=dark)](https://www.star-history.com/#nataliapc/msx_z80bench&Date)
## Star History

---
