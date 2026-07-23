[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Executable,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments,

    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'
$emsdkCommit = 'c69d433d8509c5c64564c2f0d054bf102a5cf67e'
$vcpkgCommit = 'a0400024711b283056538ac19ced80b91a83c24c'
$emsdk = (Resolve-Path (
    Join-Path $ToolchainRoot 'emsdk-4.0.7'
)).Path
$vcpkg = (Resolve-Path (
    Join-Path $ToolchainRoot 'vcpkg-a0400024'
)).Path

function Assert-Commit {
    param([string]$Repository, [string]$Expected)
    $actual = git -C $Repository rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "Expected $Repository at $Expected, got $actual"
    }
}

Assert-Commit -Repository $emsdk -Expected $emsdkCommit
Assert-Commit -Repository $vcpkg -Expected $vcpkgCommit
$activationFile = Join-Path $emsdk '.emscripten'
if (-not (Test-Path -LiteralPath $activationFile)) {
    throw 'Pinned emsdk is installed but not locally activated'
}
$activation = Get-Content -LiteralPath $activationFile -Raw
if (-not $activation.Contains(
    "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'"
)) {
    throw 'Pinned emsdk activation does not select upstream/emscripten'
}

. (Join-Path $emsdk 'emsdk_env.ps1')
$activeEmsdk = (Resolve-Path $env:EMSDK).Path
if ($activeEmsdk -ne $emsdk) {
    throw "Expected EMSDK=$emsdk, got $activeEmsdk"
}
$env:EMSDK = $emsdk
$env:EMSCRIPTEN_ROOT = Join-Path $emsdk 'upstream\emscripten'
$env:EMSCRIPTEN_VERSION = '4.0.7'
$env:VCPKG_ROOT = $vcpkg
$env:Path = "$vcpkg$([IO.Path]::PathSeparator)$env:Path"
$env:VCPKG_DISABLE_METRICS = '1'
$env:VCPKG_DEFAULT_BINARY_CACHE = (
    Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg\bincache'
)
New-Item -ItemType Directory `
    -Path $env:VCPKG_DEFAULT_BINARY_CACHE -Force | Out-Null

$emxx = Join-Path $env:EMSCRIPTEN_ROOT 'em++.bat'
$emxxVersion = & $emxx --version
$emxxVersionText = $emxxVersion -join "`n"
if ($LASTEXITCODE -ne 0 -or $emxxVersionText -notmatch '\b4\.0\.7\b') {
    throw "Expected em++ 4.0.7, got: $($emxxVersion -join ' ')"
}
$cmakeVersion = (& cmake --version) -join "`n"
if ($LASTEXITCODE -ne 0 -or
    $cmakeVersion -notmatch '(?m)^cmake version 4\.2\.3$') {
    throw "Expected CMake 4.2.3, got: $cmakeVersion"
}
$ninjaVersion = (& ninja --version) -join "`n"
if ($LASTEXITCODE -ne 0 -or $ninjaVersion.Trim() -ne '1.13.2') {
    throw "Expected Ninja 1.13.2, got: $ninjaVersion"
}

& $Executable @Arguments
exit $LASTEXITCODE
