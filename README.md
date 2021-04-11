# MCPatcherPatcher++
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ as the language. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (MCPP uses 50 MB idle)  
  
[Discord](https://discord.gg/waXJDswsaR)  
## Use
Newly compiled binaries for linux and windows can be found as `MCPPPP-linux` and `MCPPPP-windows.exe`. They may not be stable, and may not contain things from the latest commit. However, these are guaranteed to not immediately crash or have some obvious error. I will add releases when I feel it is stable enough.  
Mac binaries can be found by clicking the green arrow near the commit and clicking Details next to Compile Mac / build. They are guaranteed to contain things from the latest commit, but may immediately crash or have an obvious error.  
Note that on mac, double clicking on the file is not sufficient to run the program; it must be run through terminal.  
  
Use should be pretty self-explanitory, run the binary and put the path of folders to convert in `mcpppp.properties` such as `C:\Users\supsm\AppData\Roaming\.minecraft\resourcepacks`  
If a folder already contains the output directories (such as `assets/fabricskyboxes`), it will be skipped. If you want to re-convert this pack, delete the directory.  
<details>
  <summary>Output Directories</summary>

  Fabricskyboxes: `assets/fabricskyboxes/sky`  
  Variated Mob Textures: `assets/minecraft/varied/textures/entity`  
</details>

Settings can also be specified in `mcpppp.properties`. To do so, put `//set` followed by an option and a value (space seperated) in one line.  
<details>
  <summary>Settings</summary>

  | Name           | Values/Type      | Description                                                       | Default |
  |:--------------:|:----------------:|:-----------------------------------------------------------------:|:-------:|
  | `pauseOnExit` | `true`, `false` | Wait for enter/key to be pressed once execution has been finished | `true` |
  | `log`          | String           | A log file where logs will be stored                              |         |
  | `timestamp`   | `true`, `false` | Timestamp console (Logs will always be timestamped)               | `false`|
</details>


## Build
1. Clone this using `git clone` or Download and Extract the ZIP via Github.  
2. Make sure your current folder is MCPPPP, if you cloned the repository `cd MCPPPP`.  
3. Build `Source.cpp` in whichever fashion you want, using C++17. (`-fpermissive` is required for g++). Turn on optimizations if possible, the conversion process will be a lot faster.  
