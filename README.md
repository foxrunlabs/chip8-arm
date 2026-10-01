# CHIP-8 for STM32

A hardware CHIP-8 emulator written in C++ for the **STM32F411RE**, running on a Nucleo-F411RE development board with a Nokia 5110 LCD, 4×4 matrix keypad, and piezoelectric audio output.

Rather than emulating CHIP-8 on a desktop computer, this project implements the virtual machine on a microcontroller and maps its display, keypad, timers, and sound directly to physical hardware.

🎥 [Video of CHIP-8 for STM32 in action.](doc/videos/chip8-arm.mov)

## Overview

CHIP-8 is a small interpreted virtual machine originally developed in the 1970s for simple games and educational programs. Its compact architecture makes it particularly well suited to emulator development.

This implementation uses an STM32F411RE Cortex-M4 microcontroller to execute CHIP-8 programs while STM32 peripherals provide:

- instruction timing
- 60 Hz delay and sound timers
- SPI communication with the display
- GPIO matrix-keypad scanning
- PWM audio generation
- interrupt-driven return-to-menu input

The emulator runs without an operating system or RTOS and uses the STM32 Low-Layer (LL) peripheral APIs.

## Hardware

The project was developed for the **ST Nucleo-F411RE** development board.

### Components

- Nucleo-F411RE development board
- STM32F411RET6 Cortex-M4 microcontroller
- Nokia 5110 / PCD8544 84×48 LCD
- 4×4 matrix keypad
- Piezoelectric transducer
- 2N3904 transistor
- Supporting resistors

The STM32F411RE is configured to run at **84 MHz**.

### Peripheral Mapping

| Function | STM32 Peripheral / Pin |
| --- | --- |
| LCD clock | SPI5 SCK / PB0 |
| LCD data | SPI5 MOSI / PA10 |
| LCD chip enable | PC0 |
| LCD data/command | PC1 |
| LCD reset | PB5 |
| Keypad row 1 | PB4 |
| Keypad row 2 | PB10 |
| Keypad row 3 | PC7 |
| Keypad row 4 | PB6 |
| Keypad column 1 | PA6 |
| Keypad column 2 | PA7 |
| Keypad column 3 | PA8 |
| Keypad column 4 | PA9 |
| Piezo audio | TIM2 CH1 / PA0 |
| Menu/reset button | B1 / PC13 |
| USART2 TX/RX | PA2 / PA3 |

## Emulator Architecture

The emulator is implemented primarily by the `CHIP8` C++ class.

It models the standard CHIP-8 architecture:

- 4 KiB address space
- sixteen 8-bit general-purpose registers (`V0`–`VF`)
- 16-bit index register
- program counter
- 16-level call stack
- delay timer
- sound timer
- 64×32 monochrome framebuffer
- 16-key hexadecimal keypad

Programs are loaded beginning at the traditional CHIP-8 address:

```text
0x200
```

An 80-byte hexadecimal font set is stored at the beginning of emulator memory.

### Application States

The firmware uses three basic states:

```text
Splash / Menu
      ↓
Program Selection
      ↓
CHIP-8 Emulator
```

Pressing the Nucleo board's **B1** button returns the application to the program-selection menu.

## Timing

Hardware timers provide the timing required by the virtual machine.

| Timer | Function | Approximate Frequency |
| --- | --- | ---: |
| TIM3 | CHIP-8 instruction clock | 500 Hz |
| TIM4 | Delay and sound timers | 60 Hz |
| TIM2 | Piezo PWM | 4 kHz |

The CHIP-8 CPU and timer update flags are polled by the application rather than using an RTOS scheduler.

## Display

CHIP-8's native **64×32 monochrome framebuffer** is mapped onto the Nokia 5110's 84×48 PCD8544 display.

Graphics use the traditional CHIP-8 XOR drawing behavior:

- sprite pixels are XORed with existing pixels
- `VF` reports sprite collisions
- pixels wrap at the framebuffer boundaries

The framebuffer is centered within the larger physical LCD.

Display communication uses **SPI5**.

## Keypad

The physical 4×4 matrix keypad maps directly to the CHIP-8 hexadecimal keypad:

```text
1  2  3  A
4  5  6  B
7  8  9  C
E  0  F  D
```

The STM32 scans the keypad by driving each row and reading the four column inputs.

CHIP-8 key instructions including `Ex9E`, `ExA1`, and `Fx0A` operate using this hardware input.

## Sound

CHIP-8's sound timer is connected to a physical piezoelectric transducer.

When the CHIP-8 sound timer is nonzero, **TIM2 channel 1** generates a PWM signal at approximately 4 kHz. When the timer expires, the PWM output is disabled.

This provides the simple single-tone audio expected by CHIP-8 software.

## Included Programs

Four CHIP-8 programs are compiled directly into the firmware:

| Key | Program |
| ---: | --- |
| `1` | Font Test |
| `2` | Key Test |
| `3` | Space Invaders |
| `4` | Pong |

The ROM images are stored as byte arrays in:

```text
Core/Inc/roms.hpp
```

The project currently does not load ROMs from external storage.

## CHIP-8 Compatibility

The emulator implements the core CHIP-8 instruction set, including:

- jumps and subroutines
- conditional skips
- register arithmetic and logic
- sprite rendering
- keypad input
- delay and sound timers
- random-number generation
- BCD conversion
- register/memory transfers

Some interpreter behaviors vary among historical CHIP-8 implementations. This emulator uses the following conventions:

- `8xy6` shifts `Vx`
- `8xyE` shifts `Vx`
- `Bnnn` jumps to `nnn + V0`
- `Fx55` and `Fx65` increment `I`

Super-CHIP and XO-CHIP extensions are not implemented.

## Important Files

`Core/Inc/chip8.hpp`  
Defines the CHIP-8 virtual machine, framebuffer, registers, timers, input mapping, and opcode handlers.

`Core/Src/chip8.cpp`  
Implements the emulator, instruction decoder, graphics, keypad scanning, audio, and program-selection menu.

`Core/Src/main.cpp`  
Initializes the STM32 clock and peripherals and launches the emulator.

`Core/Inc/roms.hpp`  
Contains the CHIP-8 ROM images embedded in the firmware.

`chip8.ioc`  
STM32CubeMX hardware configuration.

## Building

The project was created with **STM32CubeIDE** and targets the **NUCLEO-F411RE**.

The checked-in configuration uses:

- STM32F411RE
- ARM Cortex-M4
- GNU Arm Embedded toolchain
- C++20
- STM32 Low-Layer drivers
- hard-float ABI
- `fpv4-sp-d16`

### Dependency

The project uses a separate PCD8544 display library:

[github.com/foxrunlabs/pcd8544](https://github.com/foxrunlabs/pcd8544)

The STM32CubeIDE configuration expects the library in a sibling workspace project named `pcd8544`:

```text
${workspace_loc}/pcd8544/Inc
${workspace_loc}/pcd8544/Release
```

If the library is stored elsewhere, update the include and linker search paths in STM32CubeIDE.

### Build Steps

1. Clone this repository.

   ```bash
   git clone https://github.com/foxrunlabs/chip8-arm.git
   ```

2. Obtain the required PCD8544 library.

3. Import both projects into an STM32CubeIDE workspace.

4. Verify the PCD8544 include and library paths.

5. Select the **Debug** or **Release** build configuration.

6. Build the project.

7. Connect the Nucleo-F411RE using its onboard ST-LINK interface.

8. Flash and run the firmware from STM32CubeIDE.

## Design Notes

This project was primarily an exercise in combining emulator development with embedded hardware.

The CHIP-8 virtual machine itself is relatively small; the more interesting part of the project is mapping the abstractions expected by CHIP-8 software onto real microcontroller peripherals:

```text
CHIP-8 Display ───────► STM32 SPI ───────► PCD8544 LCD
CHIP-8 Keypad ────────► STM32 GPIO ──────► 4×4 Keypad
CHIP-8 CPU Timing ────► STM32 Timer
CHIP-8 60 Hz Timers ──► STM32 Timer
CHIP-8 Sound ─────────► STM32 PWM ───────► Piezo
```

It therefore sits at the intersection of **emulation, C++ software design, digital interfaces, and embedded-system development**.

## Limitations

The project reflects the state of the original hardware implementation and has several limitations:

- ROMs are compiled into the firmware rather than loaded dynamically.
- Super-CHIP and XO-CHIP instructions are not supported.
- There is no automated emulator compatibility test suite.
- The PCD8544 driver is maintained as a separate project.
- Returning to the menu performs only a partial reset of emulator state.
- Memory and stack bounds are not explicitly checked.
- Multi-key keypad input is not supported by the current scan logic.

These are useful areas for future refinement rather than requirements for the original implementation.

## Potential Improvements

Possible future work includes:

- SD-card or serial ROM loading
- full emulator-state reset between programs
- automated CHIP-8 compatibility tests
- configurable interpreter quirks
- Super-CHIP support
- improved keypad handling and debouncing
- framebuffer updates only when graphics change
- CMake or another portable build configuration

## License

The original project code is licensed under the [Apache License 2.0](LICENSE)

STM32-generated code, CMSIS components, third-party libraries, and embedded ROMs may be subject to their own licensing terms.

Copyright © 2022 Ryan Clarke