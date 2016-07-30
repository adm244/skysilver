##SkySilver asi plugin for ScriptDragon (Skyrim)##

**Current version: 0.3**

### New: ###

- Now key presses registering properly
- Init, info, error and status messages can be disabled
- Incresed maximum bat files count

### Description: ###

This plugin lets you bind your skyrim bat files to a specific keys on a keyboard.
It's useful for people who streams the game and wants to have an ability to make it a little bit interactive.

### Features: ###

- Up to 50 skyrim bat files (commands) can be binded
- Ability to disable\enable commands (not individually)
- Custom key bindings
- Determines position in load order among other ESPs and offsets any IDs it uses based on that

### Installation: ###

**Installing ScriptDragon**

Download latest ScriptDragon from the [official web-page](http://www.dev-c.com/skyrim/scriptdragon/). Inside a zip archive you will find 2 folders:

- **bin** - holds binaries of ScriptDragon and example plugins
- **pluginsrc** - contains ScriptDragon SDK and sources for example plugins

From `bin` folder copy into the **root** folder of Skyrim (where TESV.exe is) next 2 files:

- **dinput8.dll** - a hook library which is loaded by Skyrim Engine and loads *.asi plugins
- **ScriptDragon.dll** - the ScriptDragon library itself

**Installing SkySilver**

Download binary files from "[Download](https://bitbucket.org/adm244/skysilver/downloads)" section. Inside a zip archive you will find 3 files:

- **SkySilver.esp** - an ESP plugin for Skyrim
- **skysilver.asi** - a library plugin for ScriptDragon
- **skysilver.ini** - a configuration file

Copy `skysilver.asi` and `skysilver.ini` into the **root** folder of Skyrim (where TESV.exe is).
Copy `SkySilver.esp` into /Data folder of Skyrim (where Skyrim.esm is). That's it!

Make sure you've activated `SkySilver.esp` in the launcher and placed it **AFTER ALL** *.esm files in your load order.

You can also open and modify `skysilver.ini` to bind keys for certain commands (bat files).

### Todo List: ###

- Cooldown for each command
- A set chance for each command
- Simple scripting VM to expose ScriptDragon API to bat files

### Knows Bugs: ###

- Executing command won't do anything if player didn't open a console at least once since game was launched

### Other Information: ###

ScriptDragon can be obtained at [http://www.dev-c.com/skyrim/scriptdragon/](http://www.dev-c.com/skyrim/scriptdragon/)

Thanks to Alexander Blade ([http://www.dev-c.com](http://www.dev-c.com)) for making it!