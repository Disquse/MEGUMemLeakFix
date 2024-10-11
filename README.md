# MEGUMemLeakFix

This small mod addresses the major issue in [Marc Ecko's Getting Up: Contents Under Pressure](https://en.wikipedia.org/wiki/Marc_Ecko%27s_Getting_Up:_Contents_Under_Pressure) with memory leaks during level restarting/unloading. The game loses track of many allocated memory resources during level transitions/restarts, which results in a memory leak and occasional out-of-memory crashes once the game exceeds 2GB of memory usage (as large address awareness is disabled by default).

This memory leak issue was a huge problem for the speedrunning community, as players have to restart the game at least twice during a full playthrough to avoid crashes. The mod aims to resolve most of these issues by hooking into various game functions to monitor allocated memory and free it once the game finishes its unloading process. With this mod you should be able to complete the entire game in a single run without needing to restart or experiencing crashes.

Let's take level 1-4 ("Storefront Row") as an example:
- Initial memory usage after first level loading: ~613MB
- 10x level restarts without the mod: ~1935MB
- 10x level restarts with the mod: ~733MB

## Installation
Download the latest version from [releases](https://github.com/Disquse/MEGUMemLeakFix/releases).
Put `GettingUp.MemLeakFix.asi` in the `_Bin` folder within your game installation directory, located next to `GettingUp.exe`.
Only the latest Steam version (1.0.0.1) is supported and has been tested.

## Compiling
Generate a solution using [premake5](https://premake.github.io):
```sh
premake5 vs2022 # Visual Studio 2022
```

Open the generated solution file `.\build\megu-memleakfix.sln` and build it.

## License
MIT.
