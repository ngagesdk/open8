# lexaloffle

This is a miscellaneous collection of code and material related to products by Lexaloffle Games LLP.

This collection is maintained for the benefit of the Lexaloffle community. Source code licenses are indicated below and in the corresponding files, and may differ between files.

## PICO-8 file format routines

The PICO-8 fantasy game console stores game data in one of several documented file formats. The canonical file format is the [P8PNG](https://pico-8.fandom.com/wiki/P8PNGFileFormat) format (`.p8.png`), an encoding of the data stored stegonographically with a PNG image of the game's "cartridge." Game data consists of several regions for graphics, sound, music, and Lua code. In the P8PNG format, the Lua region is compressed using one of two documented proprietary methods: the legacy `:c:` method and the newer `pxa` method.

This repository contains C routines that can compress and decompress data using the two methods:

* `pxa_compress_snippets.c`: the PXA method, supported by PICO-8 versions 0.2.0 and newer
* `p8_compress.c`: the legacy `:c:` method, supported by all versions of PICO-8
  * This includes `FUTURE_CODE` that was injected for forwards compatibility at PICO-8 version 0.1.7. This was added to the default wrapper code in PICO-8 0.1.8 and no longer needs to be injected by the save routine.

This compression code was created and officially released by Lexaloffle Games LLP under open source licenses. See each file for the text of the respective license.

For a Python implementation of the complete P8PNG format including stegonographic decoding, see [picotool](https://github.com/dansanderson/picotool). (As of this writing, picotool only supports `:c:` compression.)

For a prose description of the file formats and compression algorithms, see [P8PNGFileFormat in the PICO-8 wiki](https://pico-8.fandom.com/wiki/P8PNGFileFormat).
