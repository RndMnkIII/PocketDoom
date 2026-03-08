# PocketDoom WAD Setup for pocket-sync / pupdate

PocketDoom uses instance-based game selection. WADs go under `Assets/pocketdoom/common/` in subdirectories, and instance JSONs under `Assets/pocketdoom/ThinkElastic.PocketDoom/` tell the core which WADs to load.

The instance JSONs are included in the release. You only need to provide the WADs.

## Source Archive

All WADs can be extracted from this Internet Archive tar:

```
https://archive.org/download/wadsitemirror/DOOM-WADS.tar
```

## WAD Extraction Map

| Archive ZIP Path | WAD Inside ZIP | SD Card Destination |
|---|---|---|
| `storage/.../DOOM-WADs/Doom (v1.9).zip` | `DOOM.WAD` | `Assets/pocketdoom/common/doom/DOOM.WAD` |
| `storage/.../DOOM-WADs/Ultimate Doom, The.zip` | `DOOM.WAD` | `Assets/pocketdoom/common/doomu/doomu.wad` |
| `storage/.../DOOM-WADs/Doom II - Hell on Earth (v1.9).zip` | `DOOM2.WAD` | `Assets/pocketdoom/common/doom2/DOOM2.WAD` |
| `storage/.../DOOM-WADs/Final Doom - The Plutonia Experiment.zip` | `PLUTONIA.WAD` | `Assets/pocketdoom/common/plutonia/PLUTONIA.WAD` |
| `storage/.../DOOM-WADs/Final Doom - Evilution.zip` | `TNT.WAD` | `Assets/pocketdoom/common/tnt/TNT.WAD` |

Full prefix: `storage/wad_backup/wadsitemirror/DOOM-WADs/`

## Direct ZIP URLs (via tar member extraction)

```
https://archive.org/download/wadsitemirror/DOOM-WADS.tar/storage/wad_backup/wadsitemirror/DOOM-WADs/Doom%20%28v1.9%29.zip
https://archive.org/download/wadsitemirror/DOOM-WADS.tar/storage/wad_backup/wadsitemirror/DOOM-WADs/Ultimate%20Doom%2C%20The.zip
https://archive.org/download/wadsitemirror/DOOM-WADS.tar/storage/wad_backup/wadsitemirror/DOOM-WADs/Doom%20II%20-%20Hell%20on%20Earth%20%28v1.9%29.zip
https://archive.org/download/wadsitemirror/DOOM-WADS.tar/storage/wad_backup/wadsitemirror/DOOM-WADs/Final%20Doom%20-%20The%20Plutonia%20Experiment.zip
https://archive.org/download/wadsitemirror/DOOM-WADS.tar/storage/wad_backup/wadsitemirror/DOOM-WADs/Final%20Doom%20-%20Evilution.zip
```

## SD Card Layout

```
Assets/pocketdoom/
├── common/
│   ├── doom.bin                          # Included in release
│   ├── doom/DOOM.WAD                     # Doom v1.9 (registered)
│   ├── doomu/doomu.wad                   # The Ultimate Doom
│   ├── doom2/DOOM2.WAD                   # Doom II v1.9
│   ├── plutonia/PLUTONIA.WAD             # Final Doom: Plutonia
│   └── tnt/TNT.WAD                       # Final Doom: Evilution
└── ThinkElastic.PocketDoom/
    ├── Doom.json                          # Included in release
    ├── UltimateDoom.json                  # Included in release
    ├── Doom2.json                         # Included in release
    ├── Plutonia.json                      # Included in release
    └── TNT.json                           # Included in release
```

## Notes

- Each ZIP contains one WAD file — extract and place as shown above
- The shareware WAD (`Doom (v1.9) (Demo).zip` → `DOOM1.WAD`) also works if you want a free option — place it as `Assets/pocketdoom/common/doom/DOOM1.WAD` and update the Doom.json filename accordingly
- Instance JSONs and `doom.bin` are included in the core release — only the WADs need to be sourced separately
- Game mode is auto-detected from WAD contents (shareware, registered, commercial, etc.)
