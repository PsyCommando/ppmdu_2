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

## Grabbing the source code:

Check out the repository and the submodules:
```
git clone --recurse-submodules https://github.com/PsyCommando/ppmdu_2.git
```

## Building the Source Code With Visual Studio 2022

You can open the project directory within VS2022 or some other IDE that supports CMake projects and the Ninja build system, and it should be properly setup for building out of the box! Everything is preconfigured in the vcpkg.json and the CMakePreset.json files.


## Building the Source Code Via Console Commands

This is a bit more involved. First we need to make sure you got all you need installed.

### Bootstrap vcpkg
Now move into the root of the repository you just cloned. 
```cd ppmdu_2```

Then, make sure to bootstrap vcpkg, so it can grab the packages we need for building this automatically. On windows:
```
vcpkg/bootstrap-vcpkg.bat
```
or on linux:
```
./vcpkg/bootstrap-vcpkg.sh
```

### Make Sure CMake is Installed
Grab cmake with your package manager. On ubuntu/debian:
```
sudo apt-get install cmake
```

On windows, if you're not using Visual Studio 2022, that's a bit more complicated. You'll have to download the cmake binary, and refer to cmake.exe via the full path in the commands below, or if you added the directory with the cmake.exe to your PATH environment variable, you don't have to do that.
Here's the link to the cmake downloads: https://cmake.org/download/

### Make Sure Ninja is Installed
Then make sure you got the ninja build system installed. 
On Windows, just grab it here: https://github.com/ninja-build/ninja/releases. Then put the full path to the exe in this arg when using cmake (The ... is just to represent the rest of the command line):
```
cmake ... -DCMAKE_MAKE_PROGRAM="[PATH TO NINJA.EXE HERE]"
```

Or on linux you can just use your package manager and grab the package: https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages
For example, on ubuntu/debian:
```
sudo apt-get install ninja-build
```

### Configuring and Building the Project
The next part should be relatively straightforward.

#### Windows

You should format the configuration command this way:
```
cmake -G "Ninja" -DCMAKE_C_COMPILER:STRING="cl.exe" -DCMAKE_CXX_COMPILER:STRING="cl.exe" -DCMAKE_TOOLCHAIN_FILES="vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_MAKE_PROGRAM=[PATH TO THE NINJA.EXE] -DCMAKE_BUILD_TYPE:STRING=[BUILD TYPE EITHER "Debug" or "Release"] -DCMAKE_INSTALL_PREFIX:PATH="out/install/["ARCH-BUILD_TYPE" FOR EXAMPLE: "x64-debug"]"
```
Example:
```
cmake -G "Ninja" -DCMAKE_C_COMPILER:STRING="cl.exe" -DCMAKE_CXX_COMPILER:STRING="cl.exe" -DCMAKE_TOOLCHAIN_FILES="vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_MAKE_PROGRAM="ninja.exe" -DCMAKE_BUILD_TYPE:STRING="Debug" -DCMAKE_INSTALL_PREFIX:PATH="out/install/x64-debug"
```

If all goes well, vcpkg will download all the missing dependencies and set them up automatically. It might take a bit, because it will compile POCO which is a pretty large library, but next times you'll build, it'll be much faster once its done once.

To build the project, just do it like this:
```
cmake --build --preset [CONFIGURATION PRESET]
```
Where ``[CONFIGURATION PRESET]`` should be replaced with one of the presets in the CMakePresets.json file. Since we went with a 64 bits debug build earlier, lets use ``"x64-debug"`` for example here.


#### Linux
You should format the configuration command this way:
```
cmake -G "Ninja" -DCMAKE_C_COMPILER:STRING="gcc" -DCMAKE_CXX_COMPILER:STRING="g++" -DCMAKE_TOOLCHAIN_FILES="vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_MAKE_PROGRAM="ninja" -DCMAKE_BUILD_TYPE:STRING=[BUILD TYPE EITHER "Debug" or "Release"] -DCMAKE_INSTALL_PREFIX:PATH="out/install/["ARCH-BUILD_TYPE" FOR EXAMPLE: "x64-debug"]"
```

Where ``[CONFIGURATION PRESET]`` should be replaced with one of the presets in the ``CMakePresets.json`` file.
For example:
```
cmake -G "Ninja" -DCMAKE_C_COMPILER:STRING="gcc" -DCMAKE_CXX_COMPILER:STRING="g++" -DCMAKE_TOOLCHAIN_FILES="vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_MAKE_PROGRAM="ninja" -DCMAKE_BUILD_TYPE:STRING="Debug" -DCMAKE_INSTALL_PREFIX:PATH="out/install/x64-debug"
```

If all goes well, vspkg will automatically grab the needed dependencies and install them to the vspkg directory in the repository's root.
Next you can build the project using the command:

cmake --build --preset [CONFIGURATION PRESET]
```
Where ``[CONFIGURATION PRESET]`` should be replaced with one of the presets in the ``CMakePresets.json`` file. Since we went with a 64 bits debug build earlier, lets use ``"x64-linux-debug"`` for example here.

## Portability:
  Should be compatible with Windows 7+ 32/64, Linux 64bits. I have no ways of testing Apple support.
	All the libraries, either the submodules, or the ones I had to include in the code, should be multi-platform.

## License:
  Those tools and their source code, excluding the content of the /deps/ directory (present only for convenience), is [Creative Common 0](https://creativecommons.org/publicdomain/zero/1.0/), AKA Public Domain.
  Do what you want with it. Use the code in your coding horror museum, copy-paste it in your pmd2 tools, anything! XD
  You don't have to credit me, but its always appreciated if you do ! And I'd love to see what people will do with this code.
