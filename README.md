# MCPatcherPatcher++
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ as the language. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (MCPP uses 50 MB idle). Note that RAM usage varies depending on what is being converted, FSB usually will use more.  
  
[Discord](https://discord.gg/waXJDswsaR)  

#### Important Note
Some websites have been stealing this project (you'll know what I mean if you search "MCPPPP"). If you are not on github, please refer to the [official page](https://github.com/supsm/MCPPPP) instead. Most other websites are outdated, and some might contain unwanted programs. If there is another official website where I distribute binaries, it will be linked on the github.  

## TLDR
On Windows, download `MCPPPP-windows.exe` from releases and double click on it.  
On Mac, download `MCPPPP-mac.tar.gz` from releases and double click on it. A file named `MCPPPP-mac` should appear in the same location. Double click on it.  
If you are on linux, you can read the section below :P

## Use
Newly compiled binaries for linux, windows, and mac can be found as `MCPPPP-linux`, `MCPPPP-windows.exe`, and `MCPPPP-mac.tar.gz` respectively. They may not be stable, and may not contain things from the latest commit. However, these are guaranteed to not immediately crash or have some obvious error. I will add releases when I feel it is stable enough.  
Note that on mac, you must first unzip  
`MCPPPP-windows.exe`, `MCPPPP-mac.tar.gz`, and `MCPPPP-linux-gui` will contain a gui.
`MCPPPP-linux`, `MCPPPP-windows-cli`, and `MCPPPP-mac-cli` do not contain a gui. There is additional information below (Section **CLI**)  
  
If a folder already contains the output directories (such as `assets/fabricskyboxes`), it will be skipped. If you want to re-convert this pack, delete the directory.  
More detailed instructions below  
<details>
  <summary>Output Directories</summary>

  Fabricskyboxes: `assets/fabricskyboxes/sky`  
  Variated Mob Textures: `assets/minecraft/varied/textures/entity`  
  Chime: `assets/mcpppp`, `assets/minecraft/overrides`
</details>


#### CLI
In the CLI versrion of MCPPPP, you will need to add paths and settings yourself. To do this, add paths of folders to convert in `mcpppp.properties` such as `C:\Users\supsm\AppData\Roaming\.minecraft\resourcepacks`.  
Settings can be specified in `mcpppp.properties`. To do so, put `//set` followed by an option and a value (space seperated) in one line.  
Alternatively, command-line arguments may be passed for temorary settings (if you don't know what this means, you probably don't need to use this anyway). Newlines are replaced with `;`.  
<details>
  <summary>Settings</summary>

  | Name              | Values/Type      | Description                                                                                                            | Default    |
  |:-----------------:|:----------------:|:----------------------------------------------------------------------------------------------------------------------:|:----------:|
  | `pauseOnExit`    | `true`, `false` | Wait for enter/key to be pressed once execution has been finished                                                      | `true`    |
  | `log`             | String           | A log file where logs will be stored                                                                                   | `log.txt` |
  | `timestamp`      | `true`, `false` | Timestamp console (Logs will always be timestamped)                                                                    | `false`   |
  | `autoDeleteTemp` | `true`, `false` | Automatically delete `mcpppp-temp` folder on startup                                                                  | `false`  |
  | `outputLevel`    | Integer, `1-5`   | How much info should be outputted <br>`1` - Spam <br>`2` - Info <br>`3` - Important <br>`4` - Warning <br>`5` - Error | `3`       |
  | `logLevel`       | Integer, `1-5`   | Same as `outputLevel`, but for logs <br>Has no effect if no log file is set                                           | `2`       |
  | `deleteSource`   | `true`, `false` | Delete source (optifine/mcpatcher) files when done converting. The pack will no longer be able to re-convert           | `false`   |
</details>

#### GUI
In the GUI version of MCPPPP, you can edit `mcpppp.properties` inside the gui. If you want, you can also add your own configuration options in the file. Your options will not be deleted; instead, there will be a GUI section at the bottom of the file. If you add anything here, it may be deleted.  

## Build
1. Clone this using `git clone` or Download and Extract the ZIP via Github.  
2. Make sure your current folder is MCPPPP, if you cloned the repository `cd MCPPPP`.  
3. Build `Source.cpp` in whichever fashion you want, using C++17. (`-fpermissive` is required for g++). Turn on optimizations if possible, the conversion process will be a lot faster.  

My build script: `g++ Source.cpp -fpermissive -std=c++17 -O3 -o MCPPPP-linux`  

#### GUI (Windows)
Uncomment `#define GUI` at top of `Source.cpp`  
Add `./` as include path  
Add `fltk.lib` as a library  

#### GUI (Other)
Uncomeent `#define GUI` at top of `Source.cpp`  
Note: You can use my build scripts instead of using `fltk-config`. `libfltk.a` is for linux, `libfltk-mac.a` is for mac  
Download [fltk 1.3.6](https://github.com/fltk/fltk/releases/tag/release-1.3.6)  
Extract, follow instructions (e.g. `README.OSX.txt` for mac) and build  
Note: On linux, make sure you have `autoconf`, `libx11-dev`, `libglu1-mesa-dev`, `libxft-dev`, and `libxext-dev` (or the equivalent)  
Drag the `fltk` library (e.g. `libfltk.a`) to the libs folder if necessary  
Run `fltk-config` in `build`  

My linux build script: `g++ -I./ -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_THREAD_SAFE -D_REENTRANT Source.cpp ./libfltk.a -lXrender -lXext -lXft -lfontconfig -lpthread -ldl -lm -lX11 -fpermissive -std=c++17 -o MCPPPP-linux-gui`  
My mac build script: `g++ -I./ -D_LARGEFILE_SOURCE -D_THREAD_SAFE -D_REENTRANT Source.cpp ./libfltk.a -lpthread -fpermissive -O3 -std=c++17 -o MCPPPP-mac`  

