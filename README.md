# [Retro Rewind Launcher](https://wiki.tockdom.com/wiki/Retro_Rewind)
This is the code to launch the hit Mario Kart Wii Custom Track Pack, Retro Rewind! Retro Rewind was created by ZPL.

This is the first time i have really used Github for something but i will try to do everything correctly.

## Features

- Updater
- Credits
- Launch
- Exit
- Settings/Options

## Credits

- Cats4Life - Channel
- ZPL - Art
- TWD98 - Art
- r00t1024 - Making Riivolution Buildable
- Aaron "AerialX", tueidj, Tempus, megazig - Riivolution

## Dependencies

- [devkitPPC and devkitARM](https://devkitpro.org/wiki/Getting_Started)
	- `dkp-pacman -S wii-dev ppc-portlibs wii-portlibs devkitARM`
- ppc-libogg, ppc-libvorbisidec and ppc-freetype
	- `dkp-pacman -S ppc-libogg ppc-libvorbisidec ppc-freetype`
- a build environment for your host with GNU Make, some coreutils, and a C++ compiler
	- on Windows, devkitPro supplies and requires this via [MSYS2](https://www.msys2.org/)
- Python 3.x and python-yaml or pyyaml (only for rawksd)
- `curl`, `unzip`, and i386 multilibs (for dollz3)
- `zip` (only for packaging the build result)


## Building

With the required dependencies installed, just run `make launcher` in the project's directory:
