[![forthebadge](http://forthebadge.com/images/badges/60-percent-of-the-time-works-every-time.svg)](http://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/compatibility-club-penguin.svg)](http://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/built-with-science.svg)](http://forthebadge.com)
[![forthebadge](http://forthebadge.com/images/badges/built-with-love.svg)](http://forthebadge.com)

# Pokemon Mystery Dungeon Utilities
Updated repository for https://github.com/PsyCommando/ppmdu.
Reworked everything to build using CMake and made the project structure less of a nightmare. It's still a bit clunky, since I'm kinda new to the whole CMake thing, but it's much easier to build.
Combined code for several utilities used to export and import things to and from the "Pokemon Mystery Dungeon : Explorers of" games on the NDS.

Here's my development thread on Project Pokemon:  
http://projectpokemon.org/forums/showthread.php?40199-Pokemon-Mystery-Dungeon-2-Psy_commando-s-Tools-and-research-notes

And here's the wiki for the file formats in PMD2:  
http://projectpokemon.org/wiki/Pokemon_Mystery_Dungeon_Explorers_of_Sky

## Utilities Info:

* **AudioUtil:** *WIP*   
  A tool for exporting and importing all the various audio formats used by Pokemon Mystery Dungeon.
  It will most likely work with any other games that uses the DSE sound driver, by Procyon Studios.

  Most of the conversion process happens in the files with the prefix "dse_", or with the names "smdl", "swdl", and "sedl".

* **DoPX:**  
  A utility to compress files into "PX" compressed containers such as "at4px" and "pkdpx". (The actual name of the compression format is unknown. But it looks like a variant of LZ.)

* **UnPX:**  
  A utility to decompress "at4px", "pkdpx", and their SIR0 wrapped form.

* **GFXCrunch:**
  A utility meant to support import/export of every single graphic formats used in PMD2.

* **PackFileUtil:**  
  A tool meant mostly for research. It extracts a "pack file"'s content, or rebuild one. The functionality is automated in other tools, so its mostly unnecessary now!

* **StatsUtil:**
  This tool exports/imports game statistics files. Things such as game text, pokemon and item names, pokemon stats, stats growth, item and move data, dungeon stats, etc..
  It exports/imports all that data(besides game strings) to XML files as an intermediate format. XML files are easily editable by anyone, and 3rd party tools supporting XML.
  The game strings are extracted to a single simple .txt file, where each line is a C-String, and special characters are represented as escape sequences. The encoding is properly determined by using a XML configuration file that users can use to add the encoding string for a specific game version/language.

## Setting up the source code:

Check out the repository and the submodules:
```
git clone --recurse-submodules https://github.com/PsyCommando/ppmdu_2.git
```

Make sure to bootstrap vcpkg, so it can grab the packages we need for building this automatically:
```
./vcpkg/bootstrap-vcpkg.bat
```
or
```
./vcpkg/bootstrap-vcpkg.sh
```

### Building the Source Code:

You can open the project directory within VS2022 or some other IDE that supports CMake projects, and it should be properly setup for building from.

### Alternative Way to Build the Source Code

Use cmake to build the project:
```
cmake --build --preset [REPLACE_ME_WITH_BUILD_PRESET_NAME] -S .
```
Where ``[REPLACE_ME_WITH_BUILD_PRESET_NAME]`` should be replaced with one of the presets in the ``CMakePresets.json`` file.
For example, for linux:
```
cmake --build --preset linux-debug -S .
```
Or for windows:
```
cmake --build --preset x64-release -S .
```

If all goes well, vspkg will automatically grab the needed dependencies and install them to the vspkg directory in the repository's root.

## Portability:
  Should be compatible with Windows 7+ 32/64, Linux 64bits. I have no ways of testing Apple support.
	All the libraries, either the submodules, or the ones I had to include in the code, should be multi-platform.

## License:
  Those tools and their source code, excluding the content of the /deps/ directory (present only for convenience), is [Creative Common 0](https://creativecommons.org/publicdomain/zero/1.0/), AKA Public Domain.
  Do what you want with it. Use the code in your coding horror museum, copy-paste it in your pmd2 tools, anything! XD
  You don't have to credit me, but its always appreciated if you do ! And I'd love to see what people will do with this code.
