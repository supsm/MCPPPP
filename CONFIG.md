The json config file (`mcpppp-config.json`) is relatively easy to edit. There are only 2 parts you need to edit, `paths` and `settings`.

### Important
You may add comments however you want and format your json however you want, however, **these will all be overridden**.  
Additional key/value pairs will be preserved, but due to current limitations with the json library, the order may not be preserved.

### Settings
The `settings` object contains a list of settings to be changed. The names are not case sensitive. Below are a list of settings that can be changed.  
<details>
  <summary>Settings</summary>

  | Name              | Values/Type      | Description                                                                                                            | Default           |
  |:-----------------:|:----------------:|:----------------------------------------------------------------------------------------------------------------------:|:-----------------:|
  | `pauseOnExit`    | `true`, `false` | Wait for enter/key to be pressed once execution has been finished                                                      | `true`           |
  | `log`             | String           | A log file where logs will be stored. `""` disables logging.                                                          | `mcpppp-log.txt` |
  | `timestamp`      | `true`, `false` | Timestamp console (Logs will always be timestamped)                                                                    | `false`          |
  | `autoDeleteTemp` | `true`, `false` | Automatically delete `mcpppp-temp` folder on startup                                                                  | `false`          |
  | `outputLevel`    | Integer, `1-5`   | How much info should be outputted <br>`1` - Spam <br>`2` - Info <br>`3` - Important <br>`4` - Warning <br>`5` - Error | `3`              |
  | `logLevel`       | Integer, `1-5`   | Same as `outputLevel`, but for logs <br>Has no effect if no log file is set                                           | `1`              |
  | `autoReconvert`  | `true`, `false` | Automatically reconvert resourcepacks instead of skipping. **Could lose data** if a pack isn't converted with MCPPPP   | `false`          |
  | `fsbTransparent` | `true`, `false` | Make Fabricskyboxes skyboxes semi-transparent to replicate what optifine does internally.                              | `true`           |
</details>

### Paths
The `paths` array contains a list of paths to resourcepacks folders (e.g. `C:\Users\user\AppData\Roaming\.minecraft\resourcepacks`).

### GUI
The `gui` object contains additional settings and paths changed from the gui. It is used so the GUI will not override your original settings. **You should not edit this object manually**.

## Example config
```json
// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md
{
	"settings": {
		"autodeletetemp": true,
		"loglevel": 1,
		"timestamp": true
	},
	"paths": [
		"D:\\Downloads\\test"
	],
	"gui": {
		"excludepaths": [
			"D:\\Downloads\\test"
		],
		"paths": [
			"F:\\MultiMC\\instances\\Fabric 1.17\\.minecraft\\resourcepacks"
		]
	}
}
```
