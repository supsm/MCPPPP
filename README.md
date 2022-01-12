# MCPatcherPatcher++
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ and FLTK as the language and base framework. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (LambdAurora's now discontinued MCPP used 50 MB idle). Note that RAM usage varies depending on what is being converted, FSB and zipped resource packs usually will use more. As a native application, it should also perform conversions faster.  

### Links  
[Discord](https://discord.gg/waXJDswsaR)  
[Modrinth](https://modrinth.com/mod/mcpppp)  
[Curseforge](https://www.curseforge.com/minecraft/mc-mods/mcpppp)  

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
Newly compiled binaries for linux, windows, and mac can be found in the repo in the `bin` directory. They may not be stable, and may not contain things from the latest commit. However, these are guaranteed to not immediately crash or have some obvious error. I will add releases when I feel it is stable enough.  
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
Prerequisites: Cmake, compiler with c++17  
To build MCPPPP from source, you should use cmake. There are 3 options for mcpppp, which are `MCPPPP_CLI`, `MCPPPP_GUI`, and `MCPPPP_JNI` (the rest are for fltk and should be ignored). `MCPPPP_JNI` defaults to false, the other two default to true.  
In cmake-gui, simply check or uncheck these checkboxes.  
If you wish to use cmake from the command line, 
1. Configure with `cmake -B build`. You may specify options such as `-DMCPPPP_CLI=TRUE`. As an example, if I only want to build jni libraries I would run `cmake -B build -DMCPPPP_CLI=FALSE -DMCPPPP_GUI=FALSE -DMCPPPP_JNI=TRUE`
2. Build with `cmake --build build --config Release`
In both cases, the binaries should be in `build/bin`

##### Mod
Prerequisites: JDK, JNI libraries from previous step
1. Download or clone the **mod branch** of MCPPPP  
2. Copy the libraries compiled in **JNI** to `src/main/resources`. There will already be compiled libraries there, you may delete them if you want.  
3. Run `./gradlew build`. JARs will be in `build/libs`  
