# MCPatcherPatcher++
[![CodeFactor](https://www.codefactor.io/repository/github/supsm/mcpppp/badge)](https://www.codefactor.io/repository/github/supsm/mcpppp)
[![Codacy](https://app.codacy.com/project/badge/Grade/78de1baf045f4931ab13ccd7664c8d74)](https://www.codacy.com/gh/supsm/MCPPPP/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=supsm/MCPPPP&amp;utm_campaign=Badge_Grade)
[![Coverity](https://img.shields.io/coverity/scan/mcpppp.svg)](https://scan.coverity.com/projects/mcpppp)
[![GitHub Build Status](https://img.shields.io/github/workflow/status/supsm/mcpppp/Compile)](https://github.com/supsm/MCPPPP/actions/workflows/compile.yml)  
[![Latest Release](https://img.shields.io/github/v/release/supsm/mcpppp)](https://github.com/supsm/mcpppp/releases)
[![GitHub Downloads](https://img.shields.io/github/downloads/supsm/mcpppp/total?label=Github%20downloads)](https://github.com/supsm/mcpppp/releases)
[![Modrinth Downloads](https://img.shields.io/modrinth/dt/V7z6aY71?label=Modrinth%20downloads)](https://modrinth.com/mod/mcpppp)
[![Curseforge Downloads](https://cf.way2muchnoise.eu/565465.svg)](https://www.curseforge.com/minecraft/mc-mods/mcpppp)  
[![License](https://img.shields.io/github/license/supsm/mcpppp)](https://github.com/supsm/mcpppp)
[![Discord](https://img.shields.io/discord/824116179534348288?logo=discord)](https://discord.gg/waXJDswsaR)  
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ and FLTK as the language and base framework. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (LambdAurora's now discontinued MCPP used 50 MB idle). Note that RAM usage varies depending on what is being converted, FSB and zipped resource packs usually will use more. As a native application, it should also perform conversions faster.  

### Links  
[Discord](https://discord.gg/waXJDswsaR)  
[Modrinth](https://modrinth.com/mod/mcpppp)  
[Curseforge](https://www.curseforge.com/minecraft/mc-mods/mcpppp)  

#### Formats
Currently, the converter supports [Fabricskyboxes](https://modrinth.com/mod/fabricskyboxes), [Varied Mob Textures](https://www.curseforge.com/minecraft/mc-mods/varied-mob-textures), and [Chime](https://www.curseforge.com/minecraft/mc-mods/chime-fabric). Fabricskyboxes conversion is the most complete, Varied Mob Textures is probably ok, and Chime isn't very complete.  
[Centime](https://github.com/SekoiaTree/Centime) will be supported eventually if I have enough time.  

#### Important Note
Some websites have been stealing this project (you'll know what I mean if you search "MCPPPP"). If you are not on github, please refer to the [official page](https://github.com/supsm/MCPPPP) instead. Most other websites are outdated, and some might contain unwanted programs. If there is another official website where I distribute binaries (e.g. modrinth), it will be linked on the github.  

## TLDR
**Windows**: download `MCPPPP-windows.exe` from releases and double click on it.  
**Mac**: download `MCPPPP-mac.dmg` from releases and extract it (I think you just double click). `MCPPPP-mac.app` should be extracted, click on it to run.  
**Linux**: download `MCPPPP-linux-cli` (no gui) or `MCPPPP-linux` (has gui). Make it executable and run it.  

## Install/Use
Newly compiled binaries for linux, windows, and mac can be found as build artifacts. Note that they may not be stable and may not work properly. I will add releases when I feel it is stable enough.  
To access build artifacts, head to the [compile](https://github.com/supsm/MCPPPP/actions/workflows/compile.yml) page, click on the top result, then scroll down to artifacts. Download the artifact corresponding to your system, then unzip it.  
`MCPPPP-windows.exe`, `MCPPPP-mac.tar.gz`, and `MCPPPP-linux` will contain a gui.
`MCPPPP-windows-cli`, `MCPPPP-mac-cli.tar.gz`, and `MCPPPP-linux-cli` do not contain a gui. There is additional information below (Section **CLI**)  
  
If a folder already contains the output directories (such as `assets/fabricskyboxes`), it will be skipped. If you want to re-convert this pack, delete the directory. MCPPPP will try to be as least invasive as possible, and will only modify these folders (as well as `pack.mcmeta`, which there will be a backup of).  
More detailed instructions below  
<details>
  <summary>Output Directories</summary>

  Fabricskyboxes: `assets/fabricskyboxes/sky`  
  Varied Mob Textures: `assets/minecraft/varied/textures/entity`  
  Chime: `assets/mcpppp`, `assets/minecraft/overrides`
</details>

##### Unofficial Distributions
Unofficial releases for rpm-based linux distributions and aur (arch linux) are provided by sharpenedblade.  
**WARNING**: these are unofficial and I cannot guarantee anything. **Use at your own risk**  
To install rpm, run `dnf copr enable sharpenedblade/mcpppp` and browse for the appropriate application.  
Aur package can be found [here](https://aur.archlinux.org/packages/mcpppp)  


#### CLI
Run `MCPPPP-cli --help`  
Running without any arguments will allow use of the [config file](CONFIG.md).  
Fun Fact: On windows, you can "Open With" a zipped resourcepack with MCPPPP, and it will convert (but unfortunately settings will not be settable). Maybe it works on other platforms too, haven't tried  

#### GUI
In the GUI version of MCPPPP, you can edit `mcpppp.properties` inside the gui. If you want, you can also add your own configuration options in the file. Your options will not be deleted; instead, there will be a GUI section at the bottom of the file. If you add anything here, it may be deleted.  
This version can also accept command line arguments, but doing so will remove the gui entirely and function like the CLI version.  

## Build
Prerequisites: Git, Cmake, compiler with c++20  
1. Clone the repository with `git clone https://github.com/supsm/MCPPPP --depth=1` and navigate to the MCPPPP folder  
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
