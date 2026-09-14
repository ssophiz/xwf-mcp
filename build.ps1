param([string]$BuildTools = 'C:\BuildTools')
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path build | Out-Null
$vcvars = Join-Path $BuildTools 'VC\Auxiliary\Build\vcvars64.bat'
if (!(Test-Path -LiteralPath $vcvars)) { throw 'Visual C++ x64 Build Tools missing' }
# Only compile our own source; no filesystem mutation is delegated across shells.
cmd /c "`"$vcvars`" && cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /MT /LD bridge.cpp /Fo:build\bridge.obj /Fe:build\xwf-mcp.dll /link advapi32.lib ole32.lib user32.lib"
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
