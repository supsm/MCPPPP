# MCPatcherPatcher++
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ as the language. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (MCPP uses 50 MB idle)  
  
[Discord](https://discord.gg/waXJDswsaR)  
## Use
Newly compiled binaries can be found as `MCPPPP-linux` and `MCPPPP-windows.exe`. They may not be stable, and may not contain things from the latest release. I will add releases when I feel it is stable enough.  
Also, I'm not sure what format would be best for linux, as I don't use it very often. If you want you may suggest file types in the [discord](https://discord.gg/waXJDswsaR).  
  
Use should be pretty self-explanitory, run the binary and put the path of folders to convert in `mcpppp.properties` such as `C:\Users\supsm\AppData\Roaming\.minecraft\resourcepacks`  
If a folder already contains the output directories (such as `assets/fabricskyboxes`), it will be skipped. If you want to re-convert this pack, delete the directory.  
<details>
  <summary>Output Directories</summary>

  Fabricskyboxes: `assets/fabricskyboxes/sky`  
  Variated Mob Textures: `assets/minecraft/varied/textures/entity`  
</details>

  
## Build
1. Clone this using `git clone` or Download and Extract the ZIP via Github.  
2. Make sure your current folder is MCPPPP, if you cloned the repository `cd MCPPPP`.  
3. Build `Source.cpp` in whichever fashion you want, using C++17. (`-fpermissive` is required for g++). Turn on optimizations if possible, the conversion process will be a lot faster.  
