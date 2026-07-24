$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Toolchain-Provenance.ps1')
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

function Clear-BuildEnvironment {
    $exactNames = @(
        'AR',
        'BINARYEN_ROOT',
        'CC',
        'CL',
        'CPP',
        'CPPFLAGS',
        'CXX',
        'CFLAGS',
        'CXXFLAGS',
        'EMCC_CFLAGS',
        'EM_CACHE',
        'EM_CONFIG',
        'EM_PORTS',
        'EM_COMPILER_WRAPPER',
        'EM_COMPILER_WRAPPER2',
        'EMSDK',
        'EMSDK_NODE',
        'EMSDK_PYTHON',
        'LD',
        'LDFLAGS',
        'LLVM_ROOT',
        'NM',
        'NODE_JS',
        'PYTHONHOME',
        'PYTHONPATH',
        'RANLIB',
        'STRIP',
        '_CL_',
        '_EMCC_CCACHE'
    )
    $prefixes = @(
        'CCACHE_',
        'CMAKE_',
        'EMCC_',
        'EMMAKEN_',
        'EMSCRIPTEN_',
        'EMSCONS_PKG_CONFIG_',
        'EMSDK_',
        'EM_',
        'GIT_',
        'PKG_CONFIG_',
        'VCPKG_',
        'X_VCPKG_'
    )
    foreach ($entry in @(Get-ChildItem Env:)) {
        $name = $entry.Name.ToUpperInvariant()
        $remove = $exactNames -ccontains $name
        if (-not $remove) {
            foreach ($prefix in $prefixes) {
                if ($name.StartsWith(
                    $prefix,
                    [StringComparison]::Ordinal
                )) {
                    $remove = $true
                    break
                }
            }
        }
        if ($remove) {
            Remove-Item -LiteralPath "Env:$($entry.Name)" -ErrorAction SilentlyContinue
        }
    }
}

Clear-BuildEnvironment

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

function Invoke-NativeProcessCapture {
    param(
        [string]$FileName,
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [string[]]$ChildArguments
    )

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FileName
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $ChildArguments) {
        $null = $startInfo.ArgumentList.Add($argument)
    }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw "Failed to start native child: $FileName"
        }
        $stdout = $process.StandardOutput.ReadToEnd()
        $stderr = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        return [PSCustomObject]@{
            ExitCode = $process.ExitCode
            Stdout = $stdout
            Stderr = $stderr
        }
    }
    finally {
        $process.Dispose()
    }
}

function Assert-NoForbiddenEmscriptenArguments {
    param(
        [string[]]$ChildArguments
    )

    for ($index = 0; $index -lt $ChildArguments.Count; $index++) {
        $argument = [string]$ChildArguments[$index]
        if ($argument -ceq '-fexceptions') {
            throw 'Forbidden Emscripten argument: -fexceptions'
        }
        if ($argument -ceq '-fno-wasm-exceptions') {
            throw 'Forbidden Emscripten argument: -fno-wasm-exceptions'
        }
        $setting = $null
        if ($argument -ceq '-s') {
            if ($index + 1 -ge $ChildArguments.Count) {
                throw 'Emscripten -s option has no setting'
            }
            $setting = [string]$ChildArguments[$index + 1]
            $index++
        }
        elseif ($argument.StartsWith(
            '-s',
            [StringComparison]::Ordinal
        ) -and $argument.Length -gt 2) {
            $setting = $argument.Substring(2)
        }
        if ($null -ne $setting -and
            $setting -match (
                '^(?:ASYNCIFY(?:=|$)|' +
                'NO_WASM_EXCEPTIONS(?:=|$)|' +
                'WASM_EXCEPTIONS=0$)'
            )) {
            throw "Forbidden Emscripten setting: $setting"
        }

        if ($argument.StartsWith(
            '@',
            [StringComparison]::Ordinal
        )) {
            $responseValue = $argument.Substring(1)
            if ([string]::IsNullOrWhiteSpace($responseValue)) {
                throw 'Empty Emscripten response-file argument'
            }
            $response = (
                Resolve-Path -LiteralPath $responseValue -ErrorAction Stop
            ).Path
            $responseItem = Get-Item -LiteralPath $response -Force
            if (($responseItem.Attributes -band
                [IO.FileAttributes]::ReparsePoint) -ne 0 -or
                -not (Test-Path -LiteralPath $response -PathType Leaf)) {
                throw (
                    'Emscripten response file is not a regular file: ' +
                    $response
                )
            }
            $responseText = Get-Content -LiteralPath $response -Raw
            foreach ($pattern in @(
                '(?<![A-Za-z0-9_])-fexceptions(?![A-Za-z0-9_-])',
                '(?<![A-Za-z0-9_])-fno-wasm-exceptions(?![A-Za-z0-9_-])',
                '(?<![A-Za-z0-9_])ASYNCIFY(?:=|\b)',
                '(?<![A-Za-z0-9_])NO_WASM_EXCEPTIONS(?:=|\b)',
                '(?<![A-Za-z0-9_])WASM_EXCEPTIONS=0(?![A-Za-z0-9_])'
            )) {
                if ($responseText -match $pattern) {
                    throw (
                        'Forbidden Emscripten response-file argument in ' +
                        $response
                    )
                }
            }
        }
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
$cmakeArchiveName = Assert-SafeLeafName `
    -Value ([string]$lock.buildTools.cmake.archiveFile) `
    -Field 'buildTools.cmake.archiveFile'
$ninjaArchiveName = Assert-SafeLeafName `
    -Value ([string]$lock.buildTools.ninja.archiveFile) `
    -Field 'buildTools.ninja.archiveFile'

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
$downloadsCandidate = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot 'downloads') `
    -Root $ToolchainRoot `
    -Description 'Build-tool archive directory'
$null = Assert-DirectoryNotReparse `
    -Path $downloadsCandidate `
    -Description 'Build-tool archive directory'
$emsdk = (Resolve-Path -LiteralPath $emsdkCandidate).Path
$vcpkg = (Resolve-Path -LiteralPath $vcpkgCandidate).Path
$cmake = (Resolve-Path -LiteralPath $cmakeCandidate).Path
$ninja = (Resolve-Path -LiteralPath $ninjaCandidate).Path
$downloads = (Resolve-Path -LiteralPath $downloadsCandidate).Path
foreach ($resolvedRoot in @(
    $emsdk,
    $vcpkg,
    $cmake,
    $ninja,
    $downloads
)) {
    $null = Get-StrictDescendantPath `
        -Path $resolvedRoot `
        -Root $ToolchainRoot `
        -Description 'Resolved canonical toolchain path'
}

Assert-RepositoryClean `
    -Repository $emsdk `
    -ExpectedCommit $emsdkCommit `
    -Description 'emsdk' | Out-Null
Assert-RepositoryClean `
    -Repository $vcpkg `
    -ExpectedCommit $vcpkgCommit `
    -Description 'vcpkg' | Out-Null
Assert-EmscriptenInstallation `
    -Emsdk $emsdk `
    -Version $emsdkVersion `
    -Contract $lock.emscripten `
    -BeforeBytecodeNormalization | Out-Null
Remove-EmscriptenBytecodeCaches `
    -Emsdk $emsdk `
    -Contract $lock.emscripten
Assert-EmscriptenInstallation `
    -Emsdk $emsdk `
    -Version $emsdkVersion `
    -Contract $lock.emscripten | Out-Null
$cmakeArchive = Get-StrictDescendantPath `
    -Path (Join-Path $downloads $cmakeArchiveName) `
    -Root $downloads `
    -Description 'CMake retained archive'
$ninjaArchive = Get-StrictDescendantPath `
    -Path (Join-Path $downloads $ninjaArchiveName) `
    -Root $downloads `
    -Description 'Ninja retained archive'
Assert-BuildToolInstallation `
    -Name 'CMake' `
    -Archive $cmakeArchive `
    -Installation $cmake `
    -Artifact $lock.buildTools.cmake | Out-Null
Assert-BuildToolInstallation `
    -Name 'Ninja' `
    -Archive $ninjaArchive `
    -Installation $ninja `
    -Artifact $lock.buildTools.ninja | Out-Null
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
$env:EM_CONFIG = $activationFile
$env:EM_CACHE = Get-StrictDescendantPath `
    -Path (Join-Path $emscriptenRoot 'cache') `
    -Root $emscriptenRoot `
    -Description 'Emscripten mutable cache'
$env:EMSDK_PYTHON = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk ([string]$lock.emscripten.pythonExecutable)) `
    -Root $emsdk `
    -Description 'Pinned EMSDK_PYTHON'
$env:EMSDK_NODE = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk ([string]$lock.emscripten.nodeExecutable)) `
    -Root $emsdk `
    -Description 'Pinned EMSDK_NODE'
$env:PYTHONDONTWRITEBYTECODE = '1'
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
$env:CMAKE_NINJA_FORCE_RESPONSE_FILE = '1'
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

$python = Resolve-ContainedLeaf `
    -Path $env:EMSDK_PYTHON `
    -Root $emsdk `
    -Description 'Pinned EMSDK_PYTHON'
$node = Resolve-ContainedLeaf `
    -Path $env:EMSDK_NODE `
    -Root $emsdk `
    -Description 'Pinned EMSDK_NODE'
if ([IO.Path]::GetExtension($python) -ine '.exe' -or
    [IO.Path]::GetExtension($node) -ine '.exe') {
    throw 'Pinned Emscripten Python and Node runtimes must be native .exe files'
}
$emxxDriver = Resolve-ContainedLeaf `
    -Path (Join-Path $emscriptenRoot 'em++.py') `
    -Root $emscriptenRoot `
    -Description 'Emscripten driver em++.py'
$emccDriver = Resolve-ContainedLeaf `
    -Path (Join-Path $emscriptenRoot 'emcc.py') `
    -Root $emscriptenRoot `
    -Description 'Emscripten driver emcc.py'
$null = Assert-FileSha256 `
    -Path $cmakeCommand `
    -Expected ([string]$lock.buildTools.cmake.executableSha256) `
    -Description 'CMake executable'
$null = Assert-FileSha256 `
    -Path $ninjaCommand `
    -Expected ([string]$lock.buildTools.ninja.executableSha256) `
    -Description 'Ninja executable'

$emxxVersionResult = Invoke-NativeProcessCapture `
    -FileName $python `
    -ChildArguments @('-B', '-E', $emxxDriver, '--version')
$emxxVersionText = $emxxVersionResult.Stdout.Trim()
if ($emxxVersionResult.ExitCode -ne 0 -or
    $emxxVersionText -notmatch (
        "\b$([Regex]::Escape($emsdkVersion))\b"
    )) {
    throw (
        "Expected em++ $emsdkVersion, got: " +
        "$emxxVersionText $($emxxVersionResult.Stderr.Trim())"
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
    Assert-NoForbiddenEmscriptenArguments -ChildArguments $Arguments
    $driver = if ($emscriptenKind -ceq 'em++') {
        $emxxDriver
    }
    else {
        $emccDriver
    }
    $driverArguments = [Collections.Generic.List[string]]::new()
    $driverArguments.Add('-B')
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
