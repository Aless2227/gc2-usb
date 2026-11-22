param([string]$DevkitPro = $env:DEVKITPRO)
$ErrorActionPreference='Stop'
if (-not $DevkitPro) { throw 'Set DEVKITPRO to your existing devkitPro installation.' }
$root=Split-Path $PSScriptRoot -Parent
$bash=Join-Path $DevkitPro 'msys2\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) { throw "MSYS2 bash not found: $bash" }
# Invoke an installed, user-managed toolchain. No global environment/registry changes.
$env:GC2_PROJECT_ROOT=$root.Replace('\','/')
& $bash --login -c 'cd "$GC2_PROJECT_ROOT" && make package'
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
