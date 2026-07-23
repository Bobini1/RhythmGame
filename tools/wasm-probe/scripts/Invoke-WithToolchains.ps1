$ErrorActionPreference = 'Stop'
$ToolchainRoot = Join-Path $PSScriptRoot '..\..\..\.toolchains'
$BinaryCache = Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg\bincache'
$toolchainRootSeen = $false
$binaryCacheSeen = $false
$delimiterSeen = $false
$argumentIndex = 0

while ($argumentIndex -lt $args.Count) {
    $token = [string]$args[$argumentIndex]
    if ($token -ceq '--') {
        $delimiterSeen = $true
        $argumentIndex++
        break
    }

    if ($token -ieq '-ToolchainRoot') {
        if ($toolchainRootSeen) {
            throw 'Duplicate wrapper option: -ToolchainRoot'
        }
        if ($argumentIndex + 1 -ge $args.Count -or
            [string]$args[$argumentIndex + 1] -ceq '--' -or
            [string]::IsNullOrEmpty(
                [string]$args[$argumentIndex + 1]
            )) {
            throw 'Missing value for wrapper option: -ToolchainRoot'
        }
        $ToolchainRoot = [string]$args[$argumentIndex + 1]
        $toolchainRootSeen = $true
        $argumentIndex += 2
        continue
    }

    if ($token -ieq '-BinaryCache') {
        if ($binaryCacheSeen) {
            throw 'Duplicate wrapper option: -BinaryCache'
        }
        if ($argumentIndex + 1 -ge $args.Count -or
            [string]$args[$argumentIndex + 1] -ceq '--' -or
            [string]::IsNullOrEmpty(
                [string]$args[$argumentIndex + 1]
            )) {
            throw 'Missing value for wrapper option: -BinaryCache'
        }
        $BinaryCache = [string]$args[$argumentIndex + 1]
        $binaryCacheSeen = $true
        $argumentIndex += 2
        continue
    }

    throw "Unknown wrapper option before --: $token"
}

if (-not $delimiterSeen) {
    throw 'Missing mandatory -- wrapper delimiter'
}
if ($argumentIndex -ge $args.Count -or
    [string]::IsNullOrEmpty([string]$args[$argumentIndex])) {
    throw 'Missing executable after -- wrapper delimiter'
}

$Executable = [string]$args[$argumentIndex]
$argumentIndex++
$childArguments = [Collections.Generic.List[string]]::new()
while ($argumentIndex -lt $args.Count) {
    $childArguments.Add([string]$args[$argumentIndex])
    $argumentIndex++
}
$Arguments = $childArguments.ToArray()

function Assert-SafeLeafName {
    param(
        [AllowEmptyString()]
        [string]$Value,
        [string]$Field
    )

    $invalid = [string]::IsNullOrWhiteSpace($Value) -or
        [IO.Path]::IsPathRooted($Value) -or
        $Value -in '.', '..' -or
        $Value.IndexOf([IO.Path]::DirectorySeparatorChar) -ge 0 -or
        $Value.IndexOf([IO.Path]::AltDirectorySeparatorChar) -ge 0 -or
        $Value.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or
        $Value.EndsWith(' ') -or
        $Value.EndsWith('.')
    $baseName = $Value.Split('.')[0]
    $reserved = $baseName -match (
        '^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$'
    )
    if ($invalid -or $reserved) {
        throw "$Field must be a safe leaf directory name, got '$Value'"
    }
    return $Value
}

function Get-StrictDescendantPath {
    param(
        [string]$Path,
        [string]$Root,
        [string]$Description
    )

    $fullRoot = [IO.Path]::GetFullPath($Root)
    $fullPath = [IO.Path]::GetFullPath($Path)
    $relative = [IO.Path]::GetRelativePath($fullRoot, $fullPath)
    $outside = $relative -eq '.' -or
        $relative -eq '..' -or
        [IO.Path]::IsPathRooted($relative) -or
        $relative.StartsWith(
            "..$([IO.Path]::DirectorySeparatorChar)",
            [StringComparison]::Ordinal
        ) -or
        $relative.StartsWith(
            "..$([IO.Path]::AltDirectorySeparatorChar)",
            [StringComparison]::Ordinal
        )
    if ($outside) {
        throw (
            "$Description must be a strict descendant of $fullRoot, " +
            "got $fullPath"
        )
    }
    return $fullPath
}

function Assert-Commit {
    param(
        [string]$Repository,
        [string]$Expected
    )

    $repositoryPath = Get-StrictDescendantPath `
        -Path $Repository `
        -Root $ToolchainRoot `
        -Description 'Repository path'
    $actualLines = @(& git -C $repositoryPath rev-parse HEAD)
    $exitCode = $LASTEXITCODE
    $actual = ($actualLines -join "`n").Trim()
    if ($exitCode -ne 0 -or $actual -ne $Expected) {
        throw "Expected $repositoryPath at $Expected, got $actual"
    }
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
    $candidate = Get-StrictDescendantPath `
        -Path $commands[0].Source `
        -Root $ExpectedRoot `
        -Description "$Name application"
    $source = (Resolve-Path -LiteralPath $candidate).Path
    $source = Get-StrictDescendantPath `
        -Path $source `
        -Root $ExpectedRoot `
        -Description "$Name resolved application"
    return $source
}

function Assert-ResolvedLeafContained {
    param(
        [string]$Path,
        [string]$Root,
        [string]$Description
    )

    $contained = Get-StrictDescendantPath `
        -Path $Path `
        -Root $Root `
        -Description $Description
    if (-not (Test-Path -LiteralPath $contained -PathType Leaf)) {
        throw "$Description is missing: $contained"
    }
    $item = Get-Item -LiteralPath $contained
    $target = $item.ResolveLinkTarget($true)
    if ($null -ne $target) {
        $contained = Get-StrictDescendantPath `
            -Path $target.FullName `
            -Root $Root `
            -Description "$Description link target"
    }
    return $contained
}

function Resolve-ContainedLeaf {
    param(
        [string]$Path,
        [string]$Root,
        [string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "$Description is missing"
    }
    $candidate = Get-StrictDescendantPath `
        -Path $Path `
        -Root $Root `
        -Description $Description
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "$Description is missing: $candidate"
    }
    $resolved = (Resolve-Path -LiteralPath $candidate).Path
    return Assert-ResolvedLeafContained `
        -Path $resolved `
        -Root $Root `
        -Description "Resolved $Description"
}

function Resolve-ChildApplication {
    param(
        [string]$Requested
    )

    $commands = @(Get-Command `
        -Name $Requested `
        -CommandType Application `
        -All `
        -ErrorAction Stop)
    if ($commands.Count -eq 0) {
        throw "Unable to resolve child application: $Requested"
    }
    $candidate = [string]$commands[0].Source
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        throw "Resolved child application has no source: $Requested"
    }
    return (Resolve-Path -LiteralPath $candidate).Path
}

function Invoke-NativeProcess {
    param(
        [string]$FileName,
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [string[]]$ChildArguments
    )

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FileName
    $startInfo.UseShellExecute = $false
    foreach ($argument in $ChildArguments) {
        $null = $startInfo.ArgumentList.Add($argument)
    }

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw "Failed to start native child: $FileName"
        }
        $process.WaitForExit()
        return $process.ExitCode
    }
    finally {
        $process.Dispose()
    }
}

$lockPath = Join-Path $PSScriptRoot '..\toolchain-lock.json'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
$ToolchainRoot = [IO.Path]::GetFullPath($ToolchainRoot)
$BinaryCache = [IO.Path]::GetFullPath($BinaryCache)
$emsdkVersion = [string]$lock.emscripten.version
$emsdkCommit = [string]$lock.emscripten.emsdkCommit
$vcpkgCommit = [string]$lock.vcpkg.baseline
if ($emsdkCommit -notmatch '^[0-9a-fA-F]{40}$') {
    throw 'emscripten.emsdkCommit must be a 40-character hexadecimal commit'
}
if ($vcpkgCommit -notmatch '^[0-9a-fA-F]{40}$') {
    throw 'vcpkg.baseline must be a 40-character hexadecimal commit'
}

$emsdkDirectory = Assert-SafeLeafName `
    -Value "emsdk-$emsdkVersion" `
    -Field 'emscripten directory'
$vcpkgDirectory = Assert-SafeLeafName `
    -Value "vcpkg-$($vcpkgCommit.Substring(0, 8))" `
    -Field 'vcpkg directory'
$cmakeDirectory = Assert-SafeLeafName `
    -Value ([string]$lock.buildTools.cmake.directory) `
    -Field 'buildTools.cmake.directory'
$ninjaDirectory = Assert-SafeLeafName `
    -Value ([string]$lock.buildTools.ninja.directory) `
    -Field 'buildTools.ninja.directory'

$emsdkCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $emsdkDirectory) `
    -Root $ToolchainRoot `
    -Description 'emsdk canonical path'
$vcpkgCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $vcpkgDirectory) `
    -Root $ToolchainRoot `
    -Description 'vcpkg canonical path'
$cmakeCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $cmakeDirectory) `
    -Root $ToolchainRoot `
    -Description 'CMake canonical path'
$ninjaCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $ninjaDirectory) `
    -Root $ToolchainRoot `
    -Description 'Ninja canonical path'
$emsdk = (Resolve-Path -LiteralPath $emsdkCandidate).Path
$vcpkg = (Resolve-Path -LiteralPath $vcpkgCandidate).Path
$cmake = (Resolve-Path -LiteralPath $cmakeCandidate).Path
$ninja = (Resolve-Path -LiteralPath $ninjaCandidate).Path
foreach ($resolvedRoot in @($emsdk, $vcpkg, $cmake, $ninja)) {
    $null = Get-StrictDescendantPath `
        -Path $resolvedRoot `
        -Root $ToolchainRoot `
        -Description 'Resolved canonical toolchain path'
}

Assert-Commit -Repository $emsdk -Expected $emsdkCommit
Assert-Commit -Repository $vcpkg -Expected $vcpkgCommit
$activationCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk '.emscripten') `
    -Root $emsdk `
    -Description 'emsdk activation file'
$activationFile = (
    Resolve-Path -LiteralPath $activationCandidate
).Path
$activationFile = Get-StrictDescendantPath `
    -Path $activationFile `
    -Root $emsdk `
    -Description 'Resolved emsdk activation file'
if (-not (Test-Path -LiteralPath $activationFile -PathType Leaf)) {
    throw 'Pinned emsdk is installed but not locally activated'
}
$activation = Get-Content -LiteralPath $activationFile -Raw
if (-not $activation.Contains(
    "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'"
)) {
    throw 'Pinned emsdk activation does not select upstream/emscripten'
}

$emsdkEnvironmentCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk 'emsdk_env.ps1') `
    -Root $emsdk `
    -Description 'emsdk environment script'
$emsdkEnvironment = (
    Resolve-Path -LiteralPath $emsdkEnvironmentCandidate
).Path
$emsdkEnvironment = Get-StrictDescendantPath `
    -Path $emsdkEnvironment `
    -Root $emsdk `
    -Description 'Resolved emsdk environment script'
. $emsdkEnvironment
$activeEmsdkCandidate = Get-StrictDescendantPath `
    -Path $env:EMSDK `
    -Root $ToolchainRoot `
    -Description 'Active EMSDK path'
$activeEmsdk = (Resolve-Path -LiteralPath $activeEmsdkCandidate).Path
$activeEmsdk = Get-StrictDescendantPath `
    -Path $activeEmsdk `
    -Root $ToolchainRoot `
    -Description 'Resolved active EMSDK path'
if ($activeEmsdk -ne $emsdk) {
    throw "Expected EMSDK=$emsdk, got $activeEmsdk"
}

$emscriptenCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk 'upstream\emscripten') `
    -Root $emsdk `
    -Description 'Emscripten command root'
$cmakeBinCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $cmake 'bin') `
    -Root $cmake `
    -Description 'CMake command root'
$emscriptenRoot = (Resolve-Path -LiteralPath $emscriptenCandidate).Path
$cmakeBin = (Resolve-Path -LiteralPath $cmakeBinCandidate).Path
$emscriptenRoot = Get-StrictDescendantPath `
    -Path $emscriptenRoot `
    -Root $emsdk `
    -Description 'Resolved Emscripten command root'
$cmakeBin = Get-StrictDescendantPath `
    -Path $cmakeBin `
    -Root $cmake `
    -Description 'Resolved CMake command root'

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
$emcc = Resolve-PinnedApplication `
    -Name 'emcc' `
    -ExpectedRoot $emscriptenRoot
$emscriptenLaunchers = [ordered]@{
    'em++' = $emxx
    'emcc' = $emcc
}
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

$child = Resolve-ChildApplication -Requested $Executable
$requestedKind = if ($Executable -ieq 'em++') {
    'em++'
}
elseif ($Executable -ieq 'emcc') {
    'emcc'
}
else {
    $null
}
$launcherKind = $null
foreach ($kind in $emscriptenLaunchers.Keys) {
    if ([string]::Equals(
        $child,
        [string]$emscriptenLaunchers[$kind],
        [StringComparison]::OrdinalIgnoreCase
    )) {
        $launcherKind = [string]$kind
        break
    }
}
if ($requestedKind -and $launcherKind -ne $requestedKind) {
    throw (
        "Pinned $requestedKind alias resolved to an unexpected launcher: " +
        $child
    )
}
$emscriptenKind = if ($requestedKind) {
    $requestedKind
}
else {
    $launcherKind
}

if ($emscriptenKind) {
    $null = Assert-ResolvedLeafContained `
        -Path $child `
        -Root $emscriptenRoot `
        -Description "Pinned $emscriptenKind launcher"
    $python = Resolve-ContainedLeaf `
        -Path $env:EMSDK_PYTHON `
        -Root $emsdk `
        -Description 'Pinned EMSDK_PYTHON'
    if ([IO.Path]::GetExtension($python) -ine '.exe') {
        throw "Pinned EMSDK_PYTHON must be a native .exe: $python"
    }
    $driver = Resolve-ContainedLeaf `
        -Path (Join-Path $emscriptenRoot "$emscriptenKind.py") `
        -Root $emscriptenRoot `
        -Description "Emscripten driver $emscriptenKind.py"
    $driverArguments = [Collections.Generic.List[string]]::new()
    $driverArguments.Add('-E')
    $driverArguments.Add($driver)
    foreach ($argument in $Arguments) {
        $driverArguments.Add($argument)
    }
    exit (Invoke-NativeProcess `
        -FileName $python `
        -ChildArguments $driverArguments.ToArray())
}

$childExtension = [IO.Path]::GetExtension($child)
if ($childExtension -iin '.cmd', '.bat') {
    throw "Batch child executables are not supported: $child"
}
exit (Invoke-NativeProcess `
    -FileName $child `
    -ChildArguments $Arguments)
