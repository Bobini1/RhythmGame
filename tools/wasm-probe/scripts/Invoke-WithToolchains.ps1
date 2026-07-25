$ErrorActionPreference = 'Stop'

function Assert-EntrypointPathChain {
    param(
        [string]$Path,
        [string]$Description
    )

    $full = [IO.Path]::GetFullPath($Path)
    $root = [IO.Path]::GetPathRoot($full)
    $current = $root
    foreach ($part in $full.Substring($root.Length).Split(
        @(
            [IO.Path]::DirectorySeparatorChar,
            [IO.Path]::AltDirectorySeparatorChar
        ),
        [StringSplitOptions]::RemoveEmptyEntries
    )) {
        $current = Join-Path $current $part
        $item = Get-Item -LiteralPath $current -Force -ErrorAction Stop
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Description reparse-point component is forbidden: $current"
        }
    }
    return $full
}

$provenanceScript = Assert-EntrypointPathChain `
    -Path (Join-Path $PSScriptRoot 'Toolchain-Provenance.ps1') `
    -Description 'toolchain provenance helper'
. $provenanceScript
$null = Assert-PathChainNotReparse `
    -Path $PSScriptRoot `
    -Description 'wrapper script directory'
$null = Assert-PathChainNotReparse `
    -Path $provenanceScript `
    -Description 'toolchain provenance helper'
$ToolchainRoot = Join-Path $PSScriptRoot '..\..\..\.toolchains'
$BinaryCache = Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg\bincache'
$VcpkgStateRoot = Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg'
$toolchainRootSeen = $false
$binaryCacheSeen = $false
$vcpkgStateRootSeen = $false
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

    if ($token -ieq '-VcpkgStateRoot') {
        if ($vcpkgStateRootSeen) {
            throw 'Duplicate wrapper option: -VcpkgStateRoot'
        }
        if ($argumentIndex + 1 -ge $args.Count -or
            [string]$args[$argumentIndex + 1] -ceq '--' -or
            [string]::IsNullOrEmpty(
                [string]$args[$argumentIndex + 1]
            )) {
            throw 'Missing value for wrapper option: -VcpkgStateRoot'
        }
        $VcpkgStateRoot = [string]$args[$argumentIndex + 1]
        $vcpkgStateRootSeen = $true
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
$requestedExecutableName = [IO.Path]::GetFileNameWithoutExtension(
    $Executable
)
$qualificationRequested = (
    $requestedExecutableName -ieq 'cmake' -and
    $Arguments.Count -gt 0 -and
    $Arguments[0] -ceq '--build'
) -or (
    $requestedExecutableName -ieq 'python' -and
    $Arguments.Count -gt 0 -and
    [IO.Path]::GetFileName($Arguments[0]) -ceq 'verify_build.py'
)
Clear-WasmBuildEnvironment
$env:PYTHONNOUSERSITE = '1'

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

    $fullRoot = Assert-PathChainNotReparse `
        -Path $Root `
        -Description "$Description root"
    $fullPath = Assert-PathChainNotReparse `
        -Path $Path `
        -Description $Description
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

$lockPath = Assert-PathChainNotReparse `
    -Path (Join-Path $PSScriptRoot '..\toolchain-lock.json') `
    -Description 'toolchain lock'
$lockReadLock = Open-AuthenticatedFileReadLock `
    -Path $lockPath `
    -Description 'toolchain lock'
$authenticatedInputLocks = [Collections.Generic.List[IDisposable]]::new()
$authenticatedInputLocks.Add($lockReadLock.Stream)
$lockReader = [IO.StreamReader]::new(
    $lockReadLock.Stream,
    [Text.UTF8Encoding]::new($false, $true),
    $true,
    4096,
    $true
)
try {
    $lockText = $lockReader.ReadToEnd()
}
finally {
    $lockReader.Dispose()
    $lockReadLock.Stream.Position = 0
}
$lock = $lockText | ConvertFrom-Json
$ToolchainRoot = [IO.Path]::GetFullPath($ToolchainRoot)
$BinaryCache = [IO.Path]::GetFullPath($BinaryCache)
$VcpkgStateRoot = [IO.Path]::GetFullPath($VcpkgStateRoot)
$null = Assert-PathChainNotReparse `
    -Path $ToolchainRoot `
    -Description 'toolchain root'
$null = Assert-PathChainNotReparse `
    -Path $BinaryCache `
    -Description 'vcpkg binary cache'
$null = Assert-PathChainNotReparse `
    -Path $VcpkgStateRoot `
    -Description 'vcpkg state root'
$emsdkVersion = [string]$lock.emscripten.version
$emsdkCommit = [string]$lock.emscripten.emsdkCommit
$vcpkgCommit = [string]$lock.vcpkg.baseline
$sourceDateEpoch = [string]$lock.reproducibleBuild.sourceDateEpoch
[Int64]$parsedSourceDateEpoch = 0
$vcpkgMaxConcurrency = [string](
    $lock.reproducibleBuild.vcpkgMaxConcurrency
)
[Int32]$parsedVcpkgMaxConcurrency = 0
if ($emsdkCommit -notmatch '^[0-9a-fA-F]{40}$') {
    throw 'emscripten.emsdkCommit must be a 40-character hexadecimal commit'
}
if ($vcpkgCommit -notmatch '^[0-9a-fA-F]{40}$') {
    throw 'vcpkg.baseline must be a 40-character hexadecimal commit'
}
if ($sourceDateEpoch -notmatch '^[1-9][0-9]*$' -or
    -not [Int64]::TryParse(
        $sourceDateEpoch,
        [Globalization.NumberStyles]::None,
        [Globalization.CultureInfo]::InvariantCulture,
        [ref]$parsedSourceDateEpoch
    )) {
    throw 'reproducibleBuild.sourceDateEpoch must be a positive integer'
}
if ([string]$lock.reproducibleBuild.derivation -cne
    'vcpkg-baseline-source-archive-root-entry-utc') {
    throw 'reproducibleBuild.derivation does not match the pinned contract'
}
if ($vcpkgMaxConcurrency -notmatch '^[1-9][0-9]*$' -or
    -not [Int32]::TryParse(
        $vcpkgMaxConcurrency,
        [Globalization.NumberStyles]::None,
        [Globalization.CultureInfo]::InvariantCulture,
        [ref]$parsedVcpkgMaxConcurrency
    ) -or
    $parsedVcpkgMaxConcurrency -gt 64) {
    throw (
        'reproducibleBuild.vcpkgMaxConcurrency must be an integer ' +
        'from 1 through 64'
    )
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
$emsdkSourceArchiveName = Assert-SafeLeafName `
    -Value ([string]$lock.emscripten.sourceArchive.archiveFile) `
    -Field 'emscripten.sourceArchive.archiveFile'
$vcpkgSourceArchiveName = Assert-SafeLeafName `
    -Value ([string]$lock.vcpkg.sourceArchive.archiveFile) `
    -Field 'vcpkg.sourceArchive.archiveFile'

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
$null = Assert-DirectoryNotReparse `
    -Path $VcpkgStateRoot `
    -Description 'vcpkg state root'
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

$emsdkSourceArchive = Get-StrictDescendantPath `
    -Path (Join-Path $downloads $emsdkSourceArchiveName) `
    -Root $downloads `
    -Description 'emsdk retained source archive'
$vcpkgSourceArchive = Get-StrictDescendantPath `
    -Path (Join-Path $downloads $vcpkgSourceArchiveName) `
    -Root $downloads `
    -Description 'vcpkg retained source archive'
$null = Assert-SourceArchiveInstallation `
    -Archive $emsdkSourceArchive `
    -Installation $emsdk `
    -Artifact $lock.emscripten.sourceArchive `
    -AllowedExtraPrefixes @(
        $lock.emscripten.sourceArchive.allowedRuntimePrefixes
    ) `
    -AllowedExtraFiles @(
        $lock.emscripten.sourceArchive.allowedRuntimeFiles
    ) `
    -Description 'emsdk'
$null = Assert-SourceArchiveInstallation `
    -Archive $vcpkgSourceArchive `
    -Installation $vcpkg `
    -Artifact $lock.vcpkg.sourceArchive `
    -AllowedExtraPrefixes @(
        $lock.vcpkg.sourceArchive.allowedRuntimePrefixes
    ) `
    -AllowedExtraFiles @(
        $lock.vcpkg.sourceArchive.allowedRuntimeFiles
    ) `
    -Description 'vcpkg'
$emsdkBootstrap = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.bootstrapScript) `
    -Description 'emsdk bootstrap script'
$null = Assert-FileSha256 `
    -Path $emsdkBootstrap `
    -Expected ([string]$lock.emscripten.bootstrapScriptSha256) `
    -Description 'emsdk bootstrap script'
$vcpkgBootstrap = Get-ProvenanceContainedPath `
    -Root $vcpkg `
    -Relative ([string]$lock.vcpkg.bootstrapLauncher) `
    -Description 'vcpkg bootstrap launcher'
$null = Assert-FileSha256 `
    -Path $vcpkgBootstrap `
    -Expected ([string]$lock.vcpkg.bootstrapLauncherSha256) `
    -Description 'vcpkg bootstrap launcher'
$vcpkgBootstrapScript = Get-ProvenanceContainedPath `
    -Root $vcpkg `
    -Relative ([string]$lock.vcpkg.bootstrapScript) `
    -Description 'vcpkg bootstrap implementation'
$null = Assert-FileSha256 `
    -Path $vcpkgBootstrapScript `
    -Expected ([string]$lock.vcpkg.bootstrapScriptSha256) `
    -Description 'vcpkg bootstrap implementation'
$vcpkgToolMetadata = Get-ProvenanceContainedPath `
    -Root $vcpkg `
    -Relative ([string]$lock.vcpkg.toolMetadata) `
    -Description 'vcpkg tool metadata'
$null = Assert-FileSha256 `
    -Path $vcpkgToolMetadata `
    -Expected ([string]$lock.vcpkg.toolMetadataSha256) `
    -Description 'vcpkg tool metadata'
$emxx = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.cxxLauncher) `
    -Description 'pinned em++ launcher'
$null = Assert-FileSha256 `
    -Path $emxx `
    -Expected ([string]$lock.emscripten.cxxLauncherSha256) `
    -Description 'pinned em++ launcher'
$emcc = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.cLauncher) `
    -Description 'pinned emcc launcher'
$null = Assert-FileSha256 `
    -Path $emcc `
    -Expected ([string]$lock.emscripten.cLauncherSha256) `
    -Description 'pinned emcc launcher'
$portCmakeContract = $lock.vcpkg.portBuildCMake
$null = Assert-VcpkgPortCMakeManifest `
    -Vcpkg $vcpkg `
    -Contract $portCmakeContract
$portDownloads = Get-StrictDescendantPath `
    -Path (Join-Path $VcpkgStateRoot 'downloads') `
    -Root $VcpkgStateRoot `
    -Description 'vcpkg downloads root'
$portDownloads = Assert-DirectoryNotReparse `
    -Path $portDownloads `
    -Description 'vcpkg downloads root'
$portArchive = Get-StrictDescendantPath `
    -Path (
        Join-Path $portDownloads ([string]$portCmakeContract.archiveFile)
    ) `
    -Root $portDownloads `
    -Description 'vcpkg port-build CMake archive'
$portTools = Get-StrictDescendantPath `
    -Path (Join-Path $portDownloads 'tools') `
    -Root $portDownloads `
    -Description 'vcpkg downloaded tools root'
$portTools = Assert-DirectoryNotReparse `
    -Path $portTools `
    -Description 'vcpkg downloaded tools root'
$portInstallation = Get-StrictDescendantPath `
    -Path (
        Join-Path $portTools (
            [string]$portCmakeContract.installationDirectory
        )
    ) `
    -Root $portTools `
    -Description 'vcpkg port-build CMake installation'
$portArchiveItem = Get-Item -LiteralPath $portArchive -Force
if (($portArchiveItem.Attributes -band
    [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    [long]$portArchiveItem.Length -ne
    [long]$portCmakeContract.archiveBytes) {
    throw 'vcpkg port-build CMake retained archive contract drifted'
}
$portCmakeIdentity = Assert-BuildToolInstallation `
    -Name 'vcpkg port-build CMake' `
    -Archive $portArchive `
    -Installation $portInstallation `
    -Artifact $portCmakeContract
$portCmakeCommand = Get-ProvenanceContainedPath `
    -Root $portInstallation `
    -Relative ([string]$portCmakeContract.executable) `
    -Description 'vcpkg port-build CMake executable'
$null = Assert-FileSha256 `
    -Path $portCmakeCommand `
    -Expected ([string]$portCmakeContract.executableSha256) `
    -Description 'vcpkg port-build CMake executable'
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
$env:SOURCE_DATE_EPOCH = $parsedSourceDateEpoch.ToString(
    [Globalization.CultureInfo]::InvariantCulture
)
$env:VCPKG_MAX_CONCURRENCY = $parsedVcpkgMaxConcurrency.ToString(
    [Globalization.CultureInfo]::InvariantCulture
)
$env:EM_CONFIG = $activationFile
$cacheDirectory = Assert-SafeLeafName `
    -Value ([string]$lock.emscripten.cache.directory) `
    -Field 'emscripten.cache.directory'
$env:EM_CACHE = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $cacheDirectory) `
    -Root $ToolchainRoot `
    -Description 'Emscripten frozen cache'
$env:EM_CACHE = (
    Resolve-Path -LiteralPath $env:EM_CACHE -ErrorAction Stop
).Path
$env:EM_CACHE = Get-StrictDescendantPath `
    -Path $env:EM_CACHE `
    -Root $ToolchainRoot `
    -Description 'Resolved Emscripten frozen cache'
$env:EMSDK_PYTHON = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk ([string]$lock.emscripten.pythonExecutable)) `
    -Root $emsdk `
    -Description 'Pinned EMSDK_PYTHON'
$env:EMSDK_NODE = Get-StrictDescendantPath `
    -Path (Join-Path $emsdk ([string]$lock.emscripten.nodeExecutable)) `
    -Root $emsdk `
    -Description 'Pinned EMSDK_NODE'
$env:PYTHONDONTWRITEBYTECODE = '1'
$cacheIdentityHelper = (
    Resolve-Path -LiteralPath (
        Join-Path $PSScriptRoot 'emscripten_cache_identity.py'
    ) -ErrorAction Stop
).Path
$cacheIdentityHelper = Assert-PathChainNotReparse `
    -Path $cacheIdentityHelper `
    -Description 'Emscripten cache identity helper'
$cacheIdentityHelperItem = Get-Item `
    -LiteralPath $cacheIdentityHelper `
    -Force
if (($cacheIdentityHelperItem.Attributes -band
    [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    -not (Test-Path -LiteralPath $cacheIdentityHelper -PathType Leaf)) {
    throw 'Emscripten cache identity helper must be a regular file'
}
$null = Assert-EmscriptenCacheIdentity `
    -CacheRoot $env:EM_CACHE `
    -Contract $lock.emscripten.cache `
    -Python $env:EMSDK_PYTHON `
    -Helper $cacheIdentityHelper
$env:EM_FROZEN_CACHE = '1'
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

$emxx = Assert-ResolvedLeafContained `
    -Path $emxx `
    -Root $emscriptenRoot `
    -Description 'pinned em++ launcher'
$emcc = Assert-ResolvedLeafContained `
    -Path $emcc `
    -Root $emscriptenRoot `
    -Description 'pinned emcc launcher'
$emscriptenLaunchers = [ordered]@{
    'em++' = $emxx
    'emcc' = $emcc
}
$vcpkgCommand = Get-ProvenanceContainedPath `
    -Root $vcpkg `
    -Relative ([string]$lock.vcpkg.executable) `
    -Description 'pinned vcpkg executable'
$vcpkgCommand = Assert-ResolvedLeafContained `
    -Path $vcpkgCommand `
    -Root $vcpkg `
    -Description 'pinned vcpkg executable'
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
$null = Assert-FileSha256 `
    -Path $vcpkgCommand `
    -Expected ([string]$lock.vcpkg.executableSha256) `
    -Description 'vcpkg executable'
$null = Assert-FileSha256 `
    -Path $cmakeCommand `
    -Expected ([string]$lock.buildTools.cmake.executableSha256) `
    -Description 'CMake executable'
$null = Assert-FileSha256 `
    -Path $ninjaCommand `
    -Expected ([string]$lock.buildTools.ninja.executableSha256) `
    -Description 'Ninja executable'
$authenticatedInputLocks.Add((
    Open-AuthenticatedFileReadLock `
        -Path $python `
        -ExpectedSha256 (
            [string]$lock.emscripten.bootstrapPython.executableSha256
        ) `
        -Description 'Pinned Emscripten Python'
).Stream)
$authenticatedInputLocks.Add((
    Open-AuthenticatedFileReadLock `
        -Path $node `
        -ExpectedSha256 (
            [string]$lock.emscripten.nodeExecutableSha256
        ) `
        -Description 'Pinned Emscripten Node'
).Stream)
$authenticatedInputLocks.Add((
    Open-AuthenticatedFileReadLock `
        -Path $vcpkgCommand `
        -ExpectedSha256 ([string]$lock.vcpkg.executableSha256) `
        -Description 'Pinned vcpkg executable'
).Stream)
$authenticatedInputLocks.Add((
    Open-AuthenticatedFileReadLock `
        -Path $cmakeCommand `
        -ExpectedSha256 (
            [string]$lock.buildTools.cmake.executableSha256
        ) `
        -Description 'Pinned CMake executable'
).Stream)
$authenticatedInputLocks.Add((
    Open-AuthenticatedFileReadLock `
        -Path $ninjaCommand `
        -ExpectedSha256 (
            [string]$lock.buildTools.ninja.executableSha256
        ) `
        -Description 'Pinned Ninja executable'
).Stream)
$authenticatedInputLocks.Add((
    Open-AuthenticatedFileReadLock `
        -Path $portCmakeCommand `
        -ExpectedSha256 (
            [string]$portCmakeContract.executableSha256
        ) `
        -Description 'Pinned vcpkg port-build CMake executable'
).Stream)
$pythonImportClosure = Open-EmscriptenPythonImportClosure `
    -Root $emscriptenRoot `
    -Contract $lock.emscripten.driverApi.pythonImportClosure
foreach ($pythonModuleLock in @($pythonImportClosure.Streams)) {
    $authenticatedInputLocks.Add($pythonModuleLock)
}
$auditor = (
    Resolve-Path -LiteralPath (
        Join-Path $PSScriptRoot 'audit_emscripten_response_files.py'
    ) -ErrorAction Stop
).Path
$auditor = Assert-PathChainNotReparse `
    -Path $auditor `
    -Description 'Emscripten response-file auditor'
$auditorItem = Get-Item -LiteralPath $auditor -Force
if (($auditorItem.Attributes -band
    [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    -not (Test-Path -LiteralPath $auditor -PathType Leaf)) {
    throw 'Emscripten response-file auditor must be a regular file'
}
$null = Assert-FileSha256 `
    -Path $auditor `
    -Expected ([string]$lock.gateTools.responseAuditorSha256) `
    -Description 'Emscripten response-file auditor'
$auditorLock = Open-AuthenticatedFileReadLock `
    -Path $auditor `
    -ExpectedSha256 ([string]$lock.gateTools.responseAuditorSha256) `
    -Description 'Emscripten response-file auditor'
$authenticatedInputLocks.Add($auditorLock.Stream)
$driverAdapter = (
    Resolve-Path -LiteralPath (
        Join-Path $PSScriptRoot 'invoke_emscripten_driver.py'
    ) -ErrorAction Stop
).Path
$driverAdapter = Assert-PathChainNotReparse `
    -Path $driverAdapter `
    -Description 'Emscripten compiler adapter'
$driverAdapterItem = Get-Item -LiteralPath $driverAdapter -Force
if (($driverAdapterItem.Attributes -band
    [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    -not (Test-Path -LiteralPath $driverAdapter -PathType Leaf)) {
    throw 'Emscripten compiler adapter must be a regular file'
}
$null = Assert-FileSha256 `
    -Path $driverAdapter `
    -Expected ([string]$lock.gateTools.adapterSha256) `
    -Description 'Emscripten compiler adapter'
$driverAdapterLock = Open-AuthenticatedFileReadLock `
    -Path $driverAdapter `
    -ExpectedSha256 ([string]$lock.gateTools.adapterSha256) `
    -Description 'Emscripten compiler adapter'
$authenticatedInputLocks.Add($driverAdapterLock.Stream)
$lockPath = Assert-PathChainNotReparse `
    -Path (Resolve-Path -LiteralPath $lockPath -ErrorAction Stop).Path `
    -Description 'toolchain lock'
$lockPathItem = Get-Item -LiteralPath $lockPath -Force
if (($lockPathItem.Attributes -band
    [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    -not (Test-Path -LiteralPath $lockPath -PathType Leaf)) {
    throw 'Toolchain lock must be a regular file'
}
$activationLock = Open-AuthenticatedFileReadLock `
    -Path $activationFile `
    -Description 'emsdk activation file'
$activationSha256 = $activationLock.Sha256
$authenticatedInputLocks.Add($activationLock.Stream)
$cacheIdentityLock = Open-AuthenticatedFileReadLock `
    -Path $cacheIdentityHelper `
    -Description 'Emscripten cache identity helper'
$authenticatedInputLocks.Add($cacheIdentityLock.Stream)

if ($qualificationRequested) {
    $repoRoot = Assert-DirectoryNotReparse `
        -Path (Join-Path $PSScriptRoot '..\..\..') `
        -Description 'Qualification repository root'
    $installedRoot = Get-StrictDescendantPath `
        -Path (Join-Path $VcpkgStateRoot 'installed') `
        -Root $VcpkgStateRoot `
        -Description 'vcpkg installed root'
    $installedRoot = Assert-DirectoryNotReparse `
        -Path $installedRoot `
        -Description 'vcpkg installed root'
    $targetInstalled = Get-StrictDescendantPath `
        -Path (Join-Path $installedRoot 'wasm32-emscripten-rg') `
        -Root $installedRoot `
        -Description 'qualified target installation'
    $hostInstalled = Get-StrictDescendantPath `
        -Path (
            Join-Path $installedRoot 'x64-windows-rg-host-release'
        ) `
        -Root $installedRoot `
        -Description 'qualified host installation'
    $targetInstalled = Assert-DirectoryNotReparse `
        -Path $targetInstalled `
        -Description 'qualified target installation'
    $hostInstalled = Assert-DirectoryNotReparse `
        -Path $hostInstalled `
        -Description 'qualified host installation'

    $qualificationRoots = @(
        [PSCustomObject]@{ Label = 'emsdk'; Path = $emsdk },
        [PSCustomObject]@{
            Label = 'emscripten-cache'
            Path = $env:EM_CACHE
        },
        [PSCustomObject]@{ Label = 'outer-cmake'; Path = $cmake },
        [PSCustomObject]@{ Label = 'ninja'; Path = $ninja },
        [PSCustomObject]@{ Label = 'vcpkg'; Path = $vcpkg },
        [PSCustomObject]@{
            Label = 'vcpkg-port-cmake'
            Path = $portInstallation
        },
        [PSCustomObject]@{
            Label = 'vcpkg-target'
            Path = $targetInstalled
        },
        [PSCustomObject]@{
            # PDBs and import libraries are link/debug products, not files
            # read when the already-built dynamic host tools execute.
            Label = 'vcpkg-host-runtime'
            Path = $hostInstalled
            ExcludedSuffixes = @('.pdb', '.lib')
        }
    )
    $qualificationFiles = [Collections.Generic.List[object]]::new()
    foreach ($retained in @(
        [PSCustomObject]@{
            Logical = 'retained/emsdk-source.zip'
            Path = $emsdkSourceArchive
        },
        [PSCustomObject]@{
            Logical = 'retained/vcpkg-source.zip'
            Path = $vcpkgSourceArchive
        },
        [PSCustomObject]@{
            Logical = 'retained/outer-cmake.zip'
            Path = $cmakeArchive
        },
        [PSCustomObject]@{
            Logical = 'retained/ninja.zip'
            Path = $ninjaArchive
        },
        [PSCustomObject]@{
            Logical = 'retained/vcpkg-port-cmake.zip'
            Path = $portArchive
        }
    )) {
        $qualificationFiles.Add($retained)
    }

    $inputManifest = Get-ProvenanceContainedPath `
        -Root $repoRoot `
        -Relative 'tools/wasm-probe/input-manifest.txt' `
        -Description 'qualification input manifest'
    $qualificationFiles.Add([PSCustomObject]@{
        Logical = 'repo/tools/wasm-probe/input-manifest.txt'
        Path = $inputManifest
    })
    foreach ($line in @(Get-Content -LiteralPath $inputManifest)) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        $relative = Assert-ProvenanceRelativePath `
            -Value $line `
            -Description 'qualification input-manifest path'
        $qualificationFiles.Add([PSCustomObject]@{
            Logical = "repo/$relative"
            Path = Get-ProvenanceContainedPath `
                -Root $repoRoot `
                -Relative $relative `
                -Description "qualification repository input '$relative'"
        })
    }

    $buildRoot = Get-ProvenanceContainedPath `
        -Root $repoRoot `
        -Relative 'tools/wasm-probe/build/wasm-release' `
        -Description 'qualification probe build root'
    $buildRoot = Assert-DirectoryNotReparse `
        -Path $buildRoot `
        -Description 'qualification probe build root'
    $buildControlManifest = Get-ProvenanceContainedPath `
        -Root $repoRoot `
        -Relative 'tools/wasm-probe/build-control-manifest.txt' `
        -Description 'qualification build-control manifest'
    $buildControlCount = 0
    foreach ($line in @(
        Get-Content -LiteralPath $buildControlManifest
    )) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        $relative = Assert-ProvenanceRelativePath `
            -Value $line `
            -Description 'qualification build-control path'
        $qualificationFiles.Add([PSCustomObject]@{
            Logical = "build-control/$relative"
            Path = Get-ProvenanceContainedPath `
                -Root $buildRoot `
                -Relative $relative `
                -Description "qualification build control '$relative'"
        })
        $buildControlCount++
    }
    if ($buildControlCount -eq 0) {
        throw 'Qualification build-control manifest is empty'
    }
    $qualificationStopwatch = [Diagnostics.Stopwatch]::StartNew()
    $qualificationClosure = Open-QualificationClosure `
        -Roots $qualificationRoots `
        -Files $qualificationFiles.ToArray()
    $qualificationStopwatch.Stop()
    Write-Host (
        'Qualification closure: files={0} bytes={1} preflightMs={2}' -f
        $qualificationClosure.FileCount,
        $qualificationClosure.TotalBytes,
        $qualificationStopwatch.ElapsedMilliseconds
    )
    foreach ($qualificationLock in @($qualificationClosure.Streams)) {
        $authenticatedInputLocks.Add($qualificationLock)
    }
    $env:RHYTHMGAME_WASM_QUALIFICATION = '1'
    $env:RHYTHMGAME_WASM_QUALIFICATION_ALGORITHM = (
        $qualificationClosure.Algorithm
    )
    $env:RHYTHMGAME_WASM_QUALIFICATION_FILE_COUNT = [string](
        $qualificationClosure.FileCount
    )
    $env:RHYTHMGAME_WASM_QUALIFICATION_TOTAL_BYTES = [string](
        $qualificationClosure.TotalBytes
    )
    $env:RHYTHMGAME_WASM_QUALIFICATION_INVENTORY_SHA256 = (
        $qualificationClosure.InventorySha256
    )
    $env:RHYTHMGAME_WASM_QUALIFICATION_AGGREGATE_SHA256 = (
        $qualificationClosure.AggregateSha256
    )
}
$env:RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER = $driverAdapter
$env:RHYTHMGAME_WASM_TOOLCHAIN_LOCK = $lockPath
$env:RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR = $auditor
$env:RHYTHMGAME_EMSCRIPTEN_ROOT = $emscriptenRoot
$env:RHYTHMGAME_EM_CONFIG = $activationFile
$env:RHYTHMGAME_EM_CONFIG_SHA256 = $activationSha256
$env:RHYTHMGAME_EM_CACHE = $env:EM_CACHE

$childExitCode = $null
try {
    $emxxVersionArguments = @(
        '-I',
        '-B',
        $driverAdapter,
        '--lock',
        $lockPath,
        '--auditor',
        $auditor,
        '--emscripten-root',
        $emscriptenRoot,
        '--driver-kind',
        'em++',
        '--em-config',
        $activationFile,
        '--em-config-sha256',
        $activationSha256,
        '--cache-root',
        $env:EM_CACHE,
        '--',
        $emxx,
        '--version'
    )
    $emxxVersionResult = Invoke-NativeProcessCapture `
        -FileName $python `
        -ChildArguments $emxxVersionArguments
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
    $portCmakeVersion = @(& $portCmakeCommand --version)
    $portCmakeVersionText = $portCmakeVersion -join "`n"
    if ($LASTEXITCODE -ne 0 -or
        $portCmakeVersionText -notmatch (
            "(?m)^cmake version " +
            "$([Regex]::Escape([string]$portCmakeContract.version))$"
        )) {
        throw (
            'Expected vcpkg port-build CMake ' +
            "$($portCmakeContract.version), got: $portCmakeVersionText"
        )
    }

    $pythonRequested = $Executable -ieq 'python' -or
        $Executable -ieq 'python.exe'
    $child = if ($pythonRequested) {
        $python
    }
    else {
        Resolve-ChildApplication -Requested $Executable
    }
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
        $driverArguments = [Collections.Generic.List[string]]::new()
        $driverArguments.Add('-I')
        $driverArguments.Add('-B')
        $driverArguments.Add($driverAdapter)
        foreach ($fixed in @(
            '--lock',
            $lockPath,
            '--auditor',
            $auditor,
            '--emscripten-root',
            $emscriptenRoot,
            '--driver-kind',
            $emscriptenKind,
            '--em-config',
            $activationFile,
            '--em-config-sha256',
            $activationSha256,
            '--cache-root',
            $env:EM_CACHE,
            '--',
            $child
        )) {
            $driverArguments.Add($fixed)
        }
        foreach ($argument in $Arguments) {
            $driverArguments.Add($argument)
        }
        $childExitCode = Invoke-NativeProcess `
            -FileName $python `
            -ChildArguments $driverArguments.ToArray()
    }
    elseif ($pythonRequested) {
        $pythonArguments = [Collections.Generic.List[string]]::new()
        $pythonArguments.Add('-I')
        $pythonArguments.Add('-B')
        foreach ($argument in $Arguments) {
            $pythonArguments.Add($argument)
        }
        $childExitCode = Invoke-NativeProcess `
            -FileName $python `
            -ChildArguments $pythonArguments.ToArray()
    }
    else {
        $childExtension = [IO.Path]::GetExtension($child)
        if ($childExtension -iin '.cmd', '.bat') {
            throw "Batch child executables are not supported: $child"
        }
        $childExitCode = Invoke-NativeProcess `
            -FileName $child `
            -ChildArguments $Arguments
    }
}
finally {
    $null = Assert-EmscriptenCacheIdentity `
        -CacheRoot $env:EM_CACHE `
        -Contract $lock.emscripten.cache `
        -Python $python `
        -Helper $cacheIdentityHelper
    for (
        $lockIndex = $authenticatedInputLocks.Count - 1;
        $lockIndex -ge 0;
        $lockIndex--
    ) {
        $authenticatedInputLocks[$lockIndex].Dispose()
    }
}
exit $childExitCode
