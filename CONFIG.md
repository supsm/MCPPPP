The json config file (`mcpppp-config.json`) is relatively easy to edit. There are only 2 parts you need to edit, `paths` and `settings`.

### Important
You may add comments however you want and format your json however you want, however, **these will all be overridden**.  
Additional key/value pairs will be preserved, but due to current limitations with the json library, the order may not be preserved.

### Settings
The `settings` object contains a list of settings to be changed. The names are not case sensitive. Below are a list of settings that can be changed.  
<details>
  <summary>Settings</summary>

  | Name              | Values/Type      | Description                                                                                                                | Default          | Mod Default   |
  |:-----------------:|:----------------:|:--------------------------------------------------------------------------------------------------------------------------:|:----------------:|:-------------:|
  | `pauseOnExit`     | `true`, `false`  | Wait for enter key to be pressed once execution has been finished                                                          | `true`           | `false`       |
  | `log`             | String           | A log file where logs will be stored. `""` disables logging                                                                | `mcpppp-log.txt` | Default       |
  | `timestamp`       | `true`, `false`  | Add timestamp to console output (Logs will always be timestamped)                                                          | `false`          | `true`        |
  | `autoDeleteTemp`  | `true`, `false`  | Automatically delete `mcpppp-temp` folder on startup                                                                       | `false`          | `true`        |
  | `outputLevel`     | Integer, `0-5`   | How much info should be outputted. See *Output levels* below                                                               | `3`              | `2`           |
  | `logLevel`        | Integer, `0-5`   | Same as `outputLevel`, but for logs <br>Has no effect if no log file is set                                                | `0`              | Default       |
  | `autoReconvert`   | `true`, `false`  | Automatically reconvert changed resourcepacks instead of skipping. Only checks packs that have previously been converted   | `false`          | `true`        |
</details>
<details>
	<symmary>Output Levels</summary>

	Output levels determine the amount of information printed to normal output or log. All info with a greater or equal level value to the level setting will be displayed.  
	Setting a low output level will print more info, high output levels will print less info.  
	| Numeric Value | Name        | Description                                                |
	|:-------------:|:-----------:|:----------------------------------------------------------:|
	| `0`           | `Debug`     | Debug information, including line number, file, etc.       |
	| `1`           | `Detail`    | Detailed information, individual files converted           |
	| `2`           | `Info`      | Somewhat detailed info, including "warnings" for pack devs |
	| `3`           | `Important` | Important info, default for `outputLevel`                  |
	| `4`           | `Warning`   | Self-explanitory                                           |
	| `5`           | `Error`     | Self-explanitory
</details>

### Paths
The `paths` array contains a list of paths to resourcepacks folders (e.g. `C:\Users\user\AppData\Roaming\.minecraft\resourcepacks`).

### GUI
The `gui` object contains additional settings and paths changed from the gui. It is used so the GUI will not override your original settings. **You should not edit this object manually**.  
Ignored in CLI version

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
