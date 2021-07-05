# Intel 8080 emulator

Intel 8080 emulator, modular by design and allow external and easy creation of io/interrupts external handlers.

## I8080 and Libi8080

The library contains the actual emulator. The executable contains two boards: One which emulates just enough of a CP/M to run the tests, and another emulating Space Invaders.

Running space invaders:
```
i8080 -board space-invaders SPACE-INVADERS.ROM
```

Running a simple CP/M COM file (only BIOS output supported):
```
i8080 -board CP/M <COM file>
```

## Building

CMake is used to configure, build and install binaires and documentations, version 3.14 minimum is required:
```
mkdir -p build && cd build
cmake ../
cmake --build .
```

## Tests

The tests are CP/M COM files and can be found [here](https://altairclone.com/downloads/cpu_tests/).

## References

Useful documentations are available in the `docs` directory in the root of the repository:
- 8080 Data Sheet.pdf: Hardware description for the I8080
- 8080 Programmers Manual.pdf: Programmers Manual for the I8080
- CPM 1.4 Manual.pdf: CP/M 1.4 Manual and references
- CPM 2 Manual.pdf: CP/M 2 Manual and references
- PLM Manual.pdf: PL/M Manual and references, also useful for its ABI.

Other useful resources are available:
- (Reverse engineering and HW details of the space invaders)[https://www.computerarcheology.com/Arcade/SpaceInvaders/Code.html]
- (Bunch of Altair, thus I8080 related resources)[https://altairclone.com/downloads/]

