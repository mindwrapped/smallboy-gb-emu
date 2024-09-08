## SmallBoy GB Emulator

A GB emulator written in C. This currently has most of the CPU instructions including CB prefix instructions implemented. I have been testing and fixing issues using [Blargg's CPU Test Roms](https://github.com/retrio/gb-test-roms) and also comparing my emulators logs to the logs included in [Gameboy Doctor](https://github.com/robert/gameboy-doctor). So far my emulator can pass 9/11 Blargg CPU test roms.


I also wrote a small python file, "parse_opcodes_json.py", to process the opcodes.json file and convert it to a more readable csv format  which I use to display and debug cpu instructions in my emulator. 


This is still very much a work in progress. Current outstanding items todo are:
- Implement handling CPU cycles and timing properly.
- Implement interrupts.
- Finish optional ncurses debugger implementation.
- Finish display output.
- Optimize and do a port to cuda for fun. Try to run solely on GPU.

#### Installation
Make sure you have gcc, SDL2, and ncurses installed.

#### Build Instructions
Clone this repository:

```bash
git clone https://github.com/mindwrapped/smallboy-gb-emu.git
cd smallboy-gb-emu
make
```
#### Usage
To run a GB ROM, simply pass the ROM file as an argument when executing the emulator:

```bash
./gb path_to_rom
```

#### References
I have been referencing these links for information on gameboy hardware and software:
- https://gbdev.io/pandocs/
- https://gekkio.fi/files/gb-docs/gbctr.pdf

#### Contributing
Contributions are welcome! Please follow these steps:
- Fork the repository.
- Create a new branch (git checkout -b feature-branch).
- Commit your changes (git commit -m 'Add some feature').
- Push to the branch (git push origin feature-branch).
- Open a Pull Request.

This project is licensed under the MIT License - see the LICENSE file for details.
