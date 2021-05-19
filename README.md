# MCPatcherPatcher++
A resource pack converter from Optifine format to newer and alternative formats. This project uses C++ as the language. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use excessive amounts of resources for no reason (MCPP uses 50 MB idle)  
  
[Discord](https://discord.gg/waXJDswsaR)  

#### Important Note
Some websites have been stealing this project (you'll know what I mean if you search "MCPPPP"). If you are not on github, please refer to the [official page](https://github.com/supsm/MCPPPP) instead. Most other websites are outdated, and some might contain unwanted programs. If there is another official website where I distribute binaries, it will be linked on the github.  

## Use
Newly compiled binaries for linux, windows, and mac can be found as `MCPPPP-linux`, `MCPPPP-windows.exe`, and `MCPPPP-mac.tar.gz` respectively. They may not be stable, and may not contain things from the latest commit. However, these are guaranteed to not immediately crash or have some obvious error. I will add releases when I feel it is stable enough.  
Note that on mac, you must first unzip  
  
Use should be pretty self-explanitory, run the binary and put the path of folders to convert in `mcpppp.properties` such as `C:\Users\supsm\AppData\Roaming\.minecraft\resourcepacks`  
If a folder already contains the output directories (such as `assets/fabricskyboxes`), it will be skipped. If you want to re-convert this pack, delete the directory.  
More detailed instructions below  
<details>
  <summary>Output Directories</summary>

  Fabricskyboxes: `assets/fabricskyboxes/sky`  
  Variated Mob Textures: `assets/minecraft/varied/textures/entity`  
</details>

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

#### Windows
Download `MCPPPP-windows.exe` from releases or preview version  
Navigate to where you downloaded the file and run it (double click)  
Open `MCPPPP-windows.properties` (may not have file extension)  
Go to your resourcepacks folder in file explorer (by default, the path is `%appdata%\.minecraft\resourcepacks`, type that in the large bar on the left and press enter)  
Right click the large bar and click `Copy Address`  
Paste into `MCPPPP-windows.properties` (you should've opened this earlier)  
Save (`Ctrl+S`) and close  
Run `MCPPPP-windows` again and wait for it to finish  
Packs should be modified to work with the mod(s)  

#### Mac
Download `MCPPPP-mac.tar.gz` from releases or preview version  
Navigate to where you downloaded the file and unzip it (double click)  
Run (double click) the extracted file (`MCPPPP-mac`)  
Open another window in finder, click on `Home` and open `MCPPPP.properties` in a text editor  
Add the path of where your resourcepacks folder is  
Save and close the text file (also close the finder window)  
Run `MCPPPP-mac` again (from the first finder window)  
Packs should be modified to work with the mod(s)  

I don't have a mac to do any testing with, so some steps may be a bit vague. If you need further assistance there should be people on the discord server who can help you.  

#### Linux/Other
See the **Build** section below (or download `MCPPPP-linux` on linux)  
Run the binary (most likely this will be through a terminal, if so `cd` first)  
Open `MCPPPP.properties` in the same directory  
Add the path of where your resourcepacks folder is  
Save and close
Run the binary again and wait for it to finish  
Packs should be modified to work with the mod(s)  


## Build
1. Clone this using `git clone` or Download and Extract the ZIP via Github.  
2. Make sure your current folder is MCPPPP, if you cloned the repository `cd MCPPPP`.  
3. Build `Source.cpp` in whichever fashion you want, using C++17. (`-fpermissive` is required for g++). Turn on optimizations if possible, the conversion process will be a lot faster.  
