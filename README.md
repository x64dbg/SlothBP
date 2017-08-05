# SlothBP

Collaborative Breakpoint Manager for x64dbg.

![screenshot](https://i.imgur.com/v07n6LT.png)

## Example INI

```
[Memory]
VirtualAlloc="kernel32:VirtualAlloc"

[Code Injection]
SetWindowsHookEx="user32:SetWindowsHookEx"

[Networking]
UrlDownloadToFile="urlmon.UrlDownloadToFile"
```

See [SlothBP.ini](https://github.com/x64dbg/SlothBP/blob/master/SlothBP.ini) for a more complete example.

## How to use

* Download relases from [Release] (https://ci.appveyor.com/project/mrexodia/slothbp/build/artifacts)
* Place Plugin in x32/64 plugin directory
* Debug your favorite target and set the breakpoints from the menu items.

Alternatively you can fork and compile the source code.

### Sharing ini's.

The 'Collaborative' part comes with the ability to share your own INI files with predefined breakpoints.
The format is simple:

Add a category

```
[SomeCategory]
```
The fields are in a APIName = BreakpointName format, meaning that APIName will be the menu item name, and BreakpointName is a module.apiname format. This allows the plugin to automatically set the correct breakpoint. (The module isnt necessarily required but definitely recommended).

```
SomeApi = module.SomeApi
```

Thanks to:
* @mrexodia
* [anuj](https://twitter.com/asoni)
* All others who provided input

Feel free to share your INI configurations!

