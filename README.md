# SlothBP

Collaborative Breakpoint Manager for x64dbg.

## Example INI

```
[Memory]
VirtualAlloc="kernel32:VirtualAlloc"
VirtualFree="kernel32:VirtualFree"
HeapAlloc="kernel32:HeapAlloc"
HeapFree="kernel32:HeapFree"
GlobalAlloc="kernel32:GlobalAlloc"
GlobalFree="kernel32:GlobalFree"
WriteProcessMemory="kernel32:WriteProcessMemory"
ReadProcessMemory="kernel32:ReadProcessMemory"

[Code Injection]
SetWindowsHookEx="user32:SetWindowsHookEx"
CreateRemoteThread="kernel32:CreateRemoteThread"
VirtualAllocEx="kernel32:VirtualAllocEx"
QueueUserAPC="kernel32.QueueUserAPC"

[Networking]
UrlDownloadToFile="shell32.ShellExecute"
```
