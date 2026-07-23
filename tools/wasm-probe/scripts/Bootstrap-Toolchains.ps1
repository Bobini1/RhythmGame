[CmdletBinding()]
param(
    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'
$emsdkCommit = 'c69d433d8509c5c64564c2f0d054bf102a5cf67e'
$vcpkgCommit = 'a0400024711b283056538ac19ced80b91a83c24c'
$emsdk = Join-Path $ToolchainRoot 'emsdk-4.0.7'
$vcpkg = Join-Path $ToolchainRoot 'vcpkg-a0400024'

New-Item -ItemType Directory -Path $ToolchainRoot -Force | Out-Null

function Assert-Commit {
    param([string]$Repository, [string]$Expected)
    $actual = git -C $Repository rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "Expected $Repository at $Expected, got $actual"
    }
}

if (-not (Test-Path -LiteralPath $emsdk)) {
    git clone --filter=blob:none `
        https://github.com/emscripten-core/emsdk.git $emsdk
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to clone emsdk'
    }
    git -C $emsdk checkout --detach $emsdkCommit
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to check out pinned emsdk'
    }
}
Assert-Commit -Repository $emsdk -Expected $emsdkCommit

if (-not (Test-Path -LiteralPath $vcpkg)) {
    git clone --filter=blob:none `
        https://github.com/microsoft/vcpkg.git $vcpkg
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to clone vcpkg'
    }
    git -C $vcpkg checkout --detach $vcpkgCommit
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to check out pinned vcpkg'
    }
}
Assert-Commit -Repository $vcpkg -Expected $vcpkgCommit

& (Join-Path $emsdk 'emsdk.bat') install 4.0.7
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to install Emscripten 4.0.7'
}
& (Join-Path $emsdk 'emsdk.bat') activate 4.0.7
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to activate Emscripten 4.0.7 in the isolated emsdk tree'
}

& (Join-Path $vcpkg 'bootstrap-vcpkg.bat') -disableMetrics
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to bootstrap pinned vcpkg'
}

Write-Output "EMSDK=$emsdk"
Write-Output "VCPKG_ROOT=$vcpkg"
