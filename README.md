# MCPatcherPatcher++
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ as the language. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (LambdAurora's now discontinued MCPP used 50 MB idle). Note that RAM usage varies depending on what is being converted, FSB and zipped resource packs usually will use more. As a native application, it should also perform conversions faster.  

### Links  
[Discord](https://discord.gg/waXJDswsaR)  
[Modrinth](https://modrinth.com/mod/mcpppp)

#### Formats
Currently, the converter supports [Fabricskyboxes](https://modrinth.com/mod/fabricskyboxes).  
[Varied Mob Textures](https://www.curseforge.com/minecraft/mc-mods/varied-mob-textures) isn't updated to 1.17 and doesn't have a set format yet, and [Chime](https://www.curseforge.com/minecraft/mc-mods/chime-fabric) also isn't updated to 1.17. Chime conversion will work if you build from source, but the conversion is not very complete.  
[Centime](https://github.com/SekoiaTree/Centime) will be supported eventually if I have enough time.  

#### Important Note
Some websites have been stealing this project (you'll know what I mean if you search "MCPPPP"). If you are not on github, please refer to the [official page](https://github.com/supsm/MCPPPP) instead. Most other websites are outdated, and some might contain unwanted programs. If there is another official website where I distribute binaries (e.g. modrinth), it will be linked on the github.  

## TLDR
**Windows**: download `MCPPPP-windows.exe` from releases and double click on it.  
**Mac**: download `MCPPPP-mac.tar.gz` from releases and double click on it. A file named `MCPPPP-mac` should appear in the same location. Double click on it.  
**Linux**: download `MCPPPP-linux` (no gui) or `MCPPPP-linux-gui`. Make it executable and run it.  

## Use
Newly compiled binaries for linux, windows, and mac can be found in the repo as `MCPPPP-linux`, `MCPPPP-windows.exe`, and `MCPPPP-mac.tar.gz` respectively. They may not be stable, and may not contain things from the latest commit. However, these are guaranteed to not immediately crash or have some obvious error. I will add releases when I feel it is stable enough.  
`MCPPPP-windows.exe`, `MCPPPP-mac.tar.gz`, and `MCPPPP-linux-gui` will contain a gui.
`MCPPPP-linux`, `MCPPPP-windows-cli`, and `MCPPPP-mac-cli` do not contain a gui. There is additional information below (Section **CLI**)  
  
If a folder already contains the output directories (such as `assets/fabricskyboxes`), it will be skipped. If you want to re-convert this pack, delete the directory. MCPPPP will try to be as least invasive as possible, and will only modify these folders.  
More detailed instructions below  
<details>
  <summary>Output Directories</summary>

  Fabricskyboxes: `assets/fabricskyboxes/sky`  
  Varied Mob Textures: `assets/minecraft/varied/textures/entity`  
  Chime: `assets/mcpppp`, `assets/minecraft/overrides`
</details>


#### CLI
In the CLI version of MCPPPP, you will need to add paths and settings yourself. Please read [CONFIG.md](CONFIG.md)  
Alternatively, command-line arguments may be passed for temorary settings. Simply add a json as the command line argument(s). 

#### GUI
In the GUI version of MCPPPP, you can edit `mcpppp.properties` inside the gui. If you want, you can also add your own configuration options in the file. Your options will not be deleted; instead, there will be a GUI section at the bottom of the file. If you add anything here, it may be deleted.  

## Build
#### CLI
Prerequisites: basic compiler with C++17 support  
1. Clone this using `git clone` or Download and Extract the ZIP via Github.  
2. Make sure your current folder is MCPPPP, if you cloned the repository `cd MCPPPP`.  
3. Build `Source.cpp` in whichever fashion you want, using C++17. Turn on optimizations if possible, the conversion process will be a lot faster.  

My build script: `clang++ Source.cpp -Ofast -std=c++17 -o MCPPPP-windows-cli.exe`  

#### GUI (Windows)
Prerequisites: compiler with C++17 support, capable of specifying additional include paths and linking libraries  
1. Define `GUI` as a preprocessor definition or uncomment `#define GUI` at top of `Source.cpp`  
2. Add `./` as include path  
3. Add `fltk.lib` as a library  
4. Note: `User32.lib`, `Gdi32.lib`, `Comdlg32.lib`, `Advapi32.lib`, `Shell32.lib`, and `Ole32.lib` are also required. If you are using an ide, these may already be linked.  

My clang (windows) build script: `clang++ -I./ Source.cpp ./fltk.lib -lUser32.lib -lGdi32.lib -lComdlg32.lib -lAdvapi32.lib -lShell32.lib -lOle32.lib -Ofast -std=c++17 -o MCPPPP-windows.exe -Wl,/SUBSYSTEM:WINDOWS`  

#### GUI (Other)
Prerequisites: compiler, fltk (optional, see step 2)  
1. Define `GUI` as a preprocessor definition or uncomment `#define GUI` at top of `Source.cpp`  
2. Note: You can use my build scripts instead of using `fltk-config`. `libfltk.a` is for linux, `libfltk-mac.a` is for mac  
Download [fltk 1.3.6](https://github.com/fltk/fltk/releases/tag/release-1.3.6)  
Extract, follow instructions (e.g. `README.OSX.txt` for mac) and build  
Note: On linux, make sure you have the proper libraries installed (e.g `autoconf`, `libx11-dev`, `libglu1-mesa-dev`, `libxft-dev`, and `libxext-dev`)  
Drag the `fltk` library (e.g. `libfltk.a`) to the libs folder if necessary  
Run `fltk-config --cxxflags` in `build` to get compiler flags  
Add `std=c++17` to the flags  
Navigate back to the MCPPPP folder and compile  

My linux build script: `clang++ -I./ -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_THREAD_SAFE -D_REENTRANT Source.cpp ./libfltk.a -lXrender -lXext -lXft -lfontconfig -lpthread -ldl -lm -lX11 -Ofast -std=c++17 -o MCPPPP-linux-gui`  
My mac build script: `clang++ -I./ -D_LARGEFILE_SOURCE -D_THREAD_SAFE -D_REENTRANT Source.cpp ./libfltk-mac.a -lpthread -framework Cocoa -Ofast -std=c++17 -o MCPPPP-mac`  

#### Mod
To build the mod there are 2 basic steps. First you will need to build the libraries for JNI, then you will need to build the actual mod itself.  
##### JNI
Prerequisites: compiler, JDK  
1. Download or clone the **master branch** of MCPPPP  
2. Add the java includes as include paths (This will be `%JAVA_HOME%\include` and `%JAVA_HOME%\include\win32` on windows. On linux it will be `$JAVA_HOME/include` and `$JAVA_HOME/include/linux`, and on mac replace `linux` with `darwin`)  
3. Compile a shared library. Add `-shared` as a flag for clang++/g++. -fPIC will also be needed for linux/mac. The output should be `mcpppp.dll` on windows or `libmcpppp.so` on linux and `libmcpppp.dylib` on mac.  

My windows build script: `clang++ -shared -std=c++17 -Ofast jni.cpp -o mcpppp.dll -I"%JAVA_HOME%/include" -I"%JAVA_HOME%/include/win32"`

##### Mod
Prerequisites: JDK, JNI libraries from previous step
1. Download or clone the **mod branch** of MCPPPP  
2. Copy the libraries compiled in **JNI** to `src/main/resources`. There will already be compiled libraries there, you may delete them if you want.  
3. Run `./gradlew build`. JARs will be in `build/libs`  
