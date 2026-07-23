[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Executable,

    [Parameter(ValueFromRemainingArguments = $true)]
    [AllowEmptyCollection()]
    [AllowEmptyString()]
    [string[]]$Arguments = @(),

    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    ),

    [string]$BinaryCache = (
        Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg\bincache'
    )
)

$ErrorActionPreference = 'Stop'
$lockPath = Join-Path $PSScriptRoot '..\toolchain-lock.json'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
$ToolchainRoot = [IO.Path]::GetFullPath($ToolchainRoot)
$BinaryCache = [IO.Path]::GetFullPath($BinaryCache)
$emsdkVersion = [string]$lock.emscripten.version
$emsdkCommit = [string]$lock.emscripten.emsdkCommit
$vcpkgCommit = [string]$lock.vcpkg.baseline
$emsdk = (Resolve-Path -LiteralPath (
    Join-Path $ToolchainRoot "emsdk-$emsdkVersion"
)).Path
$vcpkg = (Resolve-Path -LiteralPath (
    Join-Path $ToolchainRoot "vcpkg-$($vcpkgCommit.Substring(0, 8))"
)).Path
$cmake = (Resolve-Path -LiteralPath (
    Join-Path $ToolchainRoot ([string]$lock.buildTools.cmake.directory)
)).Path
$ninja = (Resolve-Path -LiteralPath (
    Join-Path $ToolchainRoot ([string]$lock.buildTools.ninja.directory)
)).Path

function Assert-Commit {
    param(
        [string]$Repository,
        [string]$Expected
    )

    $actualLines = @(& git -C $Repository rev-parse HEAD)
    $exitCode = $LASTEXITCODE
    $actual = ($actualLines -join "`n").Trim()
    if ($exitCode -ne 0 -or $actual -ne $Expected) {
        throw "Expected $Repository at $Expected, got $actual"
    }
}

function Test-Descendant {
    param(
        [string]$Path,
        [string]$Root
    )

    $fullPath = [IO.Path]::GetFullPath($Path)
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar
    )
    $prefix = "$fullRoot$([IO.Path]::DirectorySeparatorChar)"
    return $fullPath.StartsWith(
        $prefix,
        [StringComparison]::OrdinalIgnoreCase
    )
}

function Resolve-PinnedApplication {
    param(
        [string]$Name,
        [string]$ExpectedRoot
    )

    $commands = @(Get-Command `
        -Name $Name `
        -CommandType Application `
        -All `
        -ErrorAction Stop)
    if ($commands.Count -eq 0) {
        throw "Expected pinned $Name beneath $ExpectedRoot"
    }
    $source = (Resolve-Path -LiteralPath $commands[0].Source).Path
    if (-not (Test-Descendant -Path $source -Root $ExpectedRoot)) {
        throw (
            "Expected pinned $Name beneath $ExpectedRoot, " +
            "resolved $source"
        )
    }
    return $source
}

function ConvertTo-CmdArgument {
    param(
        [AllowEmptyString()]
        [string]$Value
    )

    if (-not $Value) {
        return '""'
    }
    return '"' + $Value.Replace('"', '""') + '"'
}

Assert-Commit -Repository $emsdk -Expected $emsdkCommit
Assert-Commit -Repository $vcpkg -Expected $vcpkgCommit
$activationFile = Join-Path $emsdk '.emscripten'
if (-not (Test-Path -LiteralPath $activationFile -PathType Leaf)) {
    throw 'Pinned emsdk is installed but not locally activated'
}
$activation = Get-Content -LiteralPath $activationFile -Raw
if (-not $activation.Contains(
    "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'"
)) {
    throw 'Pinned emsdk activation does not select upstream/emscripten'
}

. (Join-Path $emsdk 'emsdk_env.ps1')
$activeEmsdk = (Resolve-Path -LiteralPath $env:EMSDK).Path
if ($activeEmsdk -ne $emsdk) {
    throw "Expected EMSDK=$emsdk, got $activeEmsdk"
}

$emscriptenRoot = (
    Resolve-Path -LiteralPath (Join-Path $emsdk 'upstream\emscripten')
).Path
$cmakeBin = (
    Resolve-Path -LiteralPath (Join-Path $cmake 'bin')
).Path
$env:EMSDK = $emsdk
$env:EMSCRIPTEN_ROOT = $emscriptenRoot
$env:EMSCRIPTEN_VERSION = $emsdkVersion
$env:VCPKG_ROOT = $vcpkg
$pathSeparator = [IO.Path]::PathSeparator
$env:Path = (
    $vcpkg,
    $cmakeBin,
    $ninja,
    $emscriptenRoot,
    $env:Path
) -join $pathSeparator
$env:VCPKG_DISABLE_METRICS = '1'
$env:VCPKG_DEFAULT_BINARY_CACHE = $BinaryCache
New-Item `
    -ItemType Directory `
    -Path $env:VCPKG_DEFAULT_BINARY_CACHE `
    -Force | Out-Null

$emxx = Resolve-PinnedApplication `
    -Name 'em++' `
    -ExpectedRoot $emscriptenRoot
$vcpkgCommand = Resolve-PinnedApplication `
    -Name 'vcpkg' `
    -ExpectedRoot $vcpkg
$cmakeCommand = Resolve-PinnedApplication `
    -Name 'cmake' `
    -ExpectedRoot $cmakeBin
$ninjaCommand = Resolve-PinnedApplication `
    -Name 'ninja' `
    -ExpectedRoot $ninja

$emxxVersion = @(& $emxx --version)
$emxxVersionText = $emxxVersion -join "`n"
if ($LASTEXITCODE -ne 0 -or
    $emxxVersionText -notmatch (
        "\b$([Regex]::Escape($emsdkVersion))\b"
    )) {
    throw (
        "Expected em++ $emsdkVersion, got: " +
        ($emxxVersion -join ' ')
    )
}
$vcpkgVersion = @(& $vcpkgCommand version)
if ($LASTEXITCODE -ne 0) {
    throw "Pinned vcpkg version probe failed: $($vcpkgVersion -join ' ')"
}
$cmakeVersion = @(& $cmakeCommand --version)
$cmakeVersionText = $cmakeVersion -join "`n"
$expectedCmake = [string]$lock.buildTools.cmake.version
if ($LASTEXITCODE -ne 0 -or
    $cmakeVersionText -notmatch (
        "(?m)^cmake version $([Regex]::Escape($expectedCmake))$"
    )) {
    throw "Expected CMake $expectedCmake, got: $cmakeVersionText"
}
$ninjaVersion = @(& $ninjaCommand --version)
$ninjaVersionText = $ninjaVersion -join "`n"
$expectedNinja = [string]$lock.buildTools.ninja.version
if ($LASTEXITCODE -ne 0 -or
    $ninjaVersionText.Trim() -ne $expectedNinja) {
    throw "Expected Ninja $expectedNinja, got: $ninjaVersionText"
}

if ([IO.Path]::GetExtension($Executable) -in '.cmd', '.bat') {
    $commandTokens = @(
        ConvertTo-CmdArgument -Value $Executable
        foreach ($argument in $Arguments) {
            ConvertTo-CmdArgument -Value $argument
        }
    )
    & $env:ComSpec /d /s /c ($commandTokens -join ' ')
}
else {
    & $Executable @Arguments
}
$childExitCode = $LASTEXITCODE
exit $childExitCode
