# Virtual Toy Pad — Release 1.0

**By HeitorSpectre**

An unofficial ELF payload that lets you play the PS4 version of LEGO Dimensions on a jailbroken PS5 with a virtual Toy Pad, without the physical accessory. Select characters and vehicles using your DualSense.

## Compatibility

- **LEGO Dimensions:** CUSA00935, **update 1.23**. Game offsets are version-specific.
- **Console:** Jailbroken PS5 with a working ELF loader (such as ELFLDR or a compatible etaHEN loader).
- Other game versions, regions, and PS5 environments have not been validated.

## Installation and controls

1. **Close LEGO Dimensions** before loading the payload.
2. Extract `Virtual-Toy-Pad.elf` from the release ZIP and send it to your PS5 ELF loader.
3. Launch LEGO Dimensions and wait for the virtual Toy Pad to be detected.
4. Press **L3 + R3** to open/close the Virtual Toy Pad menu. Navigate with the D-pad or left analog stick.
5. **X:** select; **Circle:** back; **L1/R1:** character/vehicle category; **L2/R2:** pages.
6. Within the grid, hold **Square + D-pad** to adjust native borders, **Square + L2/R2** to adjust the selected border size, or **Square + Triangle** to cycle border modes.

The payload stores data in `/data/ldmod` on the PS5. PSN sign-in is not required.

## Build from source

On Linux or WSL, install `clang`, `ld.lld`, `make`, `gcc`, `xxd`, Python 3, `pyelftools`, and the [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk).

Set `PS5_PAYLOAD_SDK` to your SDK location (default: `$HOME/ps5-payload-sdk`). Then run:

```bash
bash build.sh
```

The compiled payload is `build/ldinject.elf`. The build generates `ldhook.bin` and `blob.h` automatically; these intermediate files are not version-controlled.

## Repository contents

This private repository includes essential source code, build scripts, figure tables, and documentation. It contains no game files, EBOOT dumps, firmware, PSN content, credentials, or diagnostic captures.

## Credits, license, and disclaimer

Created by **HeitorSpectre**. This is an independent, unofficial project not affiliated with LEGO, Warner Bros., or the game's developers. All trademarks belong to their respective owners. Users are responsible for using their own game copies lawfully.

`src/injector/pt.c` and `src/injector/pt.h` include work by **John Törnblom (2024)** under **GPL-3.0-or-later**. The original notices are preserved. See `COPYING` for the license terms. The release ZIP includes the corresponding source code in `Codigo-Fonte.zip`.