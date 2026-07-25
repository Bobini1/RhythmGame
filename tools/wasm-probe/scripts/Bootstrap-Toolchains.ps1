[CmdletBinding()]
param(
    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    ),
    [string]$VcpkgStateRoot = (
        Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg'
    ),
    [switch]$InitializeEmscriptenCache
)

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
    -Description 'bootstrap script directory'
$null = Assert-PathChainNotReparse `
    -Path $provenanceScript `
    -Description 'toolchain provenance helper'
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

$lockPath = Assert-PathChainNotReparse `
    -Path (Join-Path $PSScriptRoot '..\toolchain-lock.json') `
    -Description 'toolchain lock'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
$ToolchainRoot = [IO.Path]::GetFullPath($ToolchainRoot)
$VcpkgStateRoot = [IO.Path]::GetFullPath($VcpkgStateRoot)
$null = Assert-PathChainNotReparse `
    -Path $ToolchainRoot `
    -Description 'toolchain root'
$null = Assert-PathChainNotReparse `
    -Path $VcpkgStateRoot `
    -Description 'vcpkg state root'
New-Item -ItemType Directory -Path $ToolchainRoot -Force | Out-Null
$ToolchainRoot = Assert-DirectoryNotReparse `
    -Path $ToolchainRoot `
    -Description 'toolchain root'
New-Item -ItemType Directory -Path $VcpkgStateRoot -Force | Out-Null
$VcpkgStateRoot = Assert-DirectoryNotReparse `
    -Path $VcpkgStateRoot `
    -Description 'vcpkg state root'
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

$emsdk = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $emsdkDirectory) `
    -Root $ToolchainRoot `
    -Description 'emsdk canonical path'
$vcpkg = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $vcpkgDirectory) `
    -Root $ToolchainRoot `
    -Description 'vcpkg canonical path'
$downloads = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot 'downloads') `
    -Root $ToolchainRoot `
    -Description 'Download directory'

New-Item -ItemType Directory -Path $downloads -Force | Out-Null
$downloads = Assert-DirectoryNotReparse `
    -Path $downloads `
    -Description 'Download directory'
New-Item -ItemType Directory -Path $VcpkgStateRoot -Force | Out-Null
$VcpkgStateRoot = Assert-DirectoryNotReparse `
    -Path $VcpkgStateRoot `
    -Description 'vcpkg state root'

function Install-SourceTree {
    param(
        [string]$Installation,
        [PSCustomObject]$Artifact,
        [string]$DisplayName
    )

    $installationPath = Get-StrictDescendantPath `
        -Path $Installation `
        -Root $ToolchainRoot `
        -Description "$DisplayName canonical path"
    $archiveName = Assert-SafeLeafName `
        -Value ([string]$Artifact.archiveFile) `
        -Field "$DisplayName source archive filename"
    $archive = Get-StrictDescendantPath `
        -Path (Join-Path $downloads $archiveName) `
        -Root $downloads `
        -Description "$DisplayName source archive"
    $archive = Install-AuthenticatedDownload `
        -Uri ([string]$Artifact.url) `
        -Destination $archive `
        -ExpectedSha256 ([string]$Artifact.sha256) `
        -Description "$DisplayName source archive"

    if (Test-Path -LiteralPath $installationPath) {
        $null = Assert-SourceArchiveInstallation `
            -Archive $archive `
            -Installation $installationPath `
            -Artifact $Artifact `
            -AllowedExtraPrefixes @($Artifact.allowedRuntimePrefixes) `
            -AllowedExtraFiles @($Artifact.allowedRuntimeFiles) `
            -Description $DisplayName
        return $archive
    }

    $temporary = Get-StrictDescendantPath `
        -Path "$installationPath.bootstrap-tmp" `
        -Root $ToolchainRoot `
        -Description "$DisplayName temporary path"
    if (Test-Path -LiteralPath $temporary) {
        Remove-ProvenanceOwnedTree `
            -Path $temporary `
            -OwnedRoot $ToolchainRoot `
            -Description "$DisplayName temporary tree"
    }

    try {
        New-Item -ItemType Directory -Path $temporary | Out-Null
        $temporary = Assert-DirectoryNotReparse `
            -Path $temporary `
            -Description "$DisplayName temporary extraction root"
        $null = Get-AuthenticatedZipPayload `
            -Archive $archive `
            -ExpectedArchiveSha256 ([string]$Artifact.sha256) `
            -StripPrefix ([string]$Artifact.payload.stripPrefix) `
            -ExtractTo $temporary
        $null = Assert-SourceArchiveInstallation `
            -Archive $archive `
            -Installation $temporary `
            -Artifact $Artifact `
            -Description $DisplayName
        $null = Assert-PathChainNotReparse `
            -Path $installationPath `
            -Description "$DisplayName canonical path"
        if (Test-Path -LiteralPath $installationPath) {
            throw "$DisplayName canonical path appeared during extraction"
        }
        Move-Item `
            -LiteralPath $temporary `
            -Destination $installationPath
        $null = Assert-SourceArchiveInstallation `
            -Archive $archive `
            -Installation $installationPath `
            -Artifact $Artifact `
            -Description $DisplayName
        return $archive
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-ProvenanceOwnedTree `
                -Path $temporary `
                -OwnedRoot $ToolchainRoot `
                -Description "$DisplayName temporary tree"
        }
    }
}

function Resolve-ApplicationInDirectory {
    param(
        [string]$Name,
        [string]$Directory
    )

    $commandRoot = Get-StrictDescendantPath `
        -Path $Directory `
        -Root $ToolchainRoot `
        -Description "$Name command directory"
    foreach ($extension in '.exe', '.cmd', '.bat') {
        $candidate = Get-StrictDescendantPath `
            -Path (Join-Path $commandRoot "$Name$extension") `
            -Root $commandRoot `
            -Description "$Name executable path"
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            continue
        }
        $command = Get-Command `
            -Name $candidate `
            -CommandType Application `
            -ErrorAction Stop
        $sourceCandidate = Get-StrictDescendantPath `
            -Path $command.Source `
            -Root $commandRoot `
            -Description "$Name application"
        $source = (Resolve-Path -LiteralPath $sourceCandidate).Path
        $source = Get-StrictDescendantPath `
            -Path $source `
            -Root $commandRoot `
            -Description "$Name resolved application"
        return $source
    }
    throw "Expected $Name application beneath $commandRoot"
}

function Assert-BuildTool {
    param(
        [string]$Name,
        [string]$Directory,
        [string]$Archive,
        [PSCustomObject]$Artifact
    )

    $toolDirectory = Get-StrictDescendantPath `
        -Path $Directory `
        -Root $ToolchainRoot `
        -Description "$Name tool directory"
    Assert-BuildToolInstallation `
        -Name $Name `
        -Archive $Archive `
        -Installation $toolDirectory `
        -Artifact $Artifact | Out-Null
    $commandDirectory = if ($Name -eq 'cmake') {
        Get-StrictDescendantPath `
            -Path (Join-Path $toolDirectory 'bin') `
            -Root $toolDirectory `
            -Description 'CMake command directory'
    }
    else {
        $toolDirectory
    }
    $command = Resolve-ApplicationInDirectory `
        -Name $Name `
        -Directory $commandDirectory
    $null = Assert-FileSha256 `
        -Path $command `
        -Expected ([string]$Artifact.executableSha256) `
        -Description "$Name executable"
    $versionLines = @(& $command --version)
    $exitCode = $LASTEXITCODE
    $versionText = ($versionLines -join "`n").Trim()
    $Version = [string]$Artifact.version
    if ($Name -eq 'cmake') {
        $valid = $versionText -match (
            "(?m)^cmake version $([Regex]::Escape($Version))$"
        )
        $expectation = "CMake $Version"
    }
    else {
        $valid = $versionText -eq $Version
        $expectation = "Ninja $Version"
    }
    if ($exitCode -ne 0 -or -not $valid) {
        throw "Expected $expectation, got: $versionText"
    }
}

function Install-BuildTool {
    param(
        [string]$Name,
        [PSCustomObject]$Artifact,
        [string]$DirectoryName
    )

    $directoryName = Assert-SafeLeafName `
        -Value $DirectoryName `
        -Field "buildTools.$Name.directory"
    $canonical = Get-StrictDescendantPath `
        -Path (Join-Path $ToolchainRoot $directoryName) `
        -Root $ToolchainRoot `
        -Description "$Name canonical path"
    $version = [string]$Artifact.version
    $archiveName = Assert-SafeLeafName `
        -Value ([string]$Artifact.archiveFile) `
        -Field "buildTools.$Name.archiveFile"
    $archive = Get-StrictDescendantPath `
        -Path (Join-Path $downloads $archiveName) `
        -Root $downloads `
        -Description "$Name archive path"
    $archive = Install-AuthenticatedDownload `
        -Uri ([string]$Artifact.url) `
        -Destination $archive `
        -ExpectedSha256 ([string]$Artifact.sha256) `
        -Description "$Name retained archive"

    if (Test-Path -LiteralPath $canonical) {
        Assert-BuildToolInstallation `
            -Name $Name `
            -Archive $archive `
            -Installation $canonical `
            -Artifact $Artifact | Out-Null
        return $canonical
    }

    $temporary = Get-StrictDescendantPath `
        -Path "$canonical.bootstrap-tmp" `
        -Root $ToolchainRoot `
        -Description "$Name temporary path"
    foreach ($ownedPath in @($temporary)) {
        if (Test-Path -LiteralPath $ownedPath) {
            Remove-ProvenanceOwnedTree `
                -Path $ownedPath `
                -OwnedRoot $ToolchainRoot `
                -Description "$Name temporary tree"
        }
    }

    try {
        New-Item `
            -ItemType Directory `
            -Path $temporary `
            -Force | Out-Null
        $archivePayload = Get-AuthenticatedZipPayload `
            -Archive $archive `
            -ExpectedArchiveSha256 ([string]$Artifact.sha256) `
            -StripPrefix ([string]$Artifact.payload.stripPrefix) `
            -ExtractTo $temporary
        if ($archivePayload.FileCount -ne
            [int]$Artifact.payload.fileCount -or
            $archivePayload.InventorySha256 -cne
            [string]$Artifact.payload.inventorySha256 -or
            $archivePayload.AggregateSha256 -cne
            [string]$Artifact.payload.aggregateSha256) {
            throw "$Name authenticated archive payload drifted"
        }
        Assert-BuildToolInstallation `
            -Name $Name `
            -Archive $archive `
            -Installation $temporary `
            -Artifact $Artifact | Out-Null
        Move-Item -LiteralPath $temporary -Destination $canonical
        return $canonical
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-ProvenanceOwnedTree `
                -Path $temporary `
                -OwnedRoot $ToolchainRoot `
                -Description "$Name temporary tree"
        }
    }
}

function Invoke-AuthenticatedPythonScript {
    param(
        [string]$Python,
        [string]$PythonSha256,
        [string]$Script,
        [string]$ScriptSha256,
        [string[]]$Arguments,
        [string]$Description
    )

    $pythonLock = Open-AuthenticatedFileReadLock `
        -Path $Python `
        -ExpectedSha256 $PythonSha256 `
        -Description "$Description Python"
    $scriptLock = $null
    try {
        $scriptLock = Open-AuthenticatedFileReadLock `
            -Path $Script `
            -ExpectedSha256 $ScriptSha256 `
            -Description "$Description script"
        $startInfo = [Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = $pythonLock.Path
        $startInfo.UseShellExecute = $false
        foreach ($argument in @('-I', '-B', $scriptLock.Path) + $Arguments) {
            $null = $startInfo.ArgumentList.Add($argument)
        }
        $process = [Diagnostics.Process]::new()
        $process.StartInfo = $startInfo
        try {
            if (-not $process.Start()) {
                throw "Failed to start $Description"
            }
            $process.WaitForExit()
            return $process.ExitCode
        }
        finally {
            $process.Dispose()
        }
    }
    finally {
        if ($null -ne $scriptLock) {
            $scriptLock.Stream.Dispose()
        }
        $pythonLock.Stream.Dispose()
    }
}

function Remove-EmsdkGeneratedEnvironmentScript {
    param([string]$Emsdk)

    foreach ($relative in 'emsdk_set_env.bat', 'emsdk_set_env.ps1') {
        $candidate = Get-ProvenanceContainedPath `
            -Root $Emsdk `
            -Relative $relative `
            -Description 'generated emsdk environment script'
        if (-not (Test-Path -LiteralPath $candidate)) {
            continue
        }
        $candidate = Assert-PathChainNotReparse `
            -Path $candidate `
            -Description 'generated emsdk environment script'
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            throw (
                'Generated emsdk environment script is not a file: ' +
                $candidate
            )
        }
        Remove-Item -LiteralPath $candidate -Force
    }
}

$emsdkWasPresent = Test-Path -LiteralPath $emsdk
$emsdkSourceArchive = Install-SourceTree `
    -Installation $emsdk `
    -Artifact $lock.emscripten.sourceArchive `
    -DisplayName 'emsdk'
$vcpkgSourceArchive = Install-SourceTree `
    -Installation $vcpkg `
    -Artifact $lock.vcpkg.sourceArchive `
    -DisplayName 'vcpkg'
if ($emsdkWasPresent) {
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
}
$bootstrapPythonInstallation = Get-StrictDescendantPath `
    -Path (
        Join-Path `
            $emsdk `
            ([string]$lock.emscripten.bootstrapPython.installationDirectory)
    ) `
    -Root $emsdk `
    -Description 'Emscripten bootstrap Python installation'
$bootstrapPythonArchive = Install-SourceTree `
    -Installation $bootstrapPythonInstallation `
    -Artifact $lock.emscripten.bootstrapPython `
    -DisplayName 'Emscripten bootstrap Python'

$cmake = Install-BuildTool `
    -Name 'cmake' `
    -Artifact $lock.buildTools.cmake `
    -DirectoryName $cmakeDirectory
$ninja = Install-BuildTool `
    -Name 'ninja' `
    -Artifact $lock.buildTools.ninja `
    -DirectoryName $ninjaDirectory

$cmakeArchive = Get-StrictDescendantPath `
    -Path (
        Join-Path $downloads ([string]$lock.buildTools.cmake.archiveFile)
    ) `
    -Root $downloads `
    -Description 'CMake retained archive'
$ninjaArchive = Get-StrictDescendantPath `
    -Path (
        Join-Path $downloads ([string]$lock.buildTools.ninja.archiveFile)
    ) `
    -Root $downloads `
    -Description 'Ninja retained archive'
# Both complete installations are authenticated before either executable is
# resolved or run.
Assert-BuildTool `
    -Name 'cmake' `
    -Directory $cmake `
    -Archive $cmakeArchive `
    -Artifact $lock.buildTools.cmake
Assert-BuildTool `
    -Name 'ninja' `
    -Directory $ninja `
    -Archive $ninjaArchive `
    -Artifact $lock.buildTools.ninja

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

$emsdkScript = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.bootstrapScript) `
    -Description 'emsdk bootstrap script'
$null = Assert-FileSha256 `
    -Path $emsdkScript `
    -Expected ([string]$lock.emscripten.bootstrapScriptSha256) `
    -Description 'emsdk bootstrap script'
$bootstrapPython = Get-ProvenanceContainedPath `
    -Root $bootstrapPythonInstallation `
    -Relative ([string]$lock.emscripten.bootstrapPython.executable) `
    -Description 'Emscripten bootstrap Python executable'
$null = Assert-FileSha256 `
    -Path $bootstrapPython `
    -Expected ([string]$lock.emscripten.bootstrapPython.executableSha256) `
    -Description 'Emscripten bootstrap Python executable'
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
$toolMetadata = ConvertFrom-StringData (
    Get-Content -LiteralPath $vcpkgToolMetadata -Raw
)
if ([string]$toolMetadata.VCPKG_TOOL_RELEASE_TAG -cne
    [string]$lock.vcpkg.toolReleaseTag -or
    [string]$lock.vcpkg.toolUrl -cne (
        'https://github.com/microsoft/vcpkg-tool/releases/download/' +
        [string]$lock.vcpkg.toolReleaseTag +
        '/vcpkg.exe'
    )) {
    throw 'vcpkg tool release metadata drifted'
}

$emsdkInstallExit = Invoke-AuthenticatedPythonScript `
    -Python $bootstrapPython `
    -PythonSha256 (
        [string]$lock.emscripten.bootstrapPython.executableSha256
    ) `
    -Script $emsdkScript `
    -ScriptSha256 ([string]$lock.emscripten.bootstrapScriptSha256) `
    -Arguments @('install', $emsdkVersion) `
    -Description 'emsdk install'
if ($emsdkInstallExit -ne 0) {
    throw "Failed to install Emscripten $emsdkVersion"
}
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
    -Archive $bootstrapPythonArchive `
    -Installation $bootstrapPythonInstallation `
    -Artifact $lock.emscripten.bootstrapPython `
    -AllowedExtraPrefixes @(
        $lock.emscripten.bootstrapPython.allowedRuntimePrefixes
    ) `
    -AllowedExtraFiles @(
        $lock.emscripten.bootstrapPython.allowedRuntimeFiles
    ) `
    -Description 'Emscripten bootstrap Python'
$emsdkActivateExit = Invoke-AuthenticatedPythonScript `
    -Python $bootstrapPython `
    -PythonSha256 (
        [string]$lock.emscripten.bootstrapPython.executableSha256
    ) `
    -Script $emsdkScript `
    -ScriptSha256 ([string]$lock.emscripten.bootstrapScriptSha256) `
    -Arguments @('activate', $emsdkVersion) `
    -Description 'emsdk activate'
if ($emsdkActivateExit -ne 0) {
    throw (
        "Failed to activate Emscripten $emsdkVersion " +
        'in the isolated emsdk tree'
    )
}
Remove-EmsdkGeneratedEnvironmentScript -Emsdk $emsdk
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
$emccLauncher = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.cLauncher) `
    -Description 'pinned emcc launcher'
$null = Assert-FileSha256 `
    -Path $emccLauncher `
    -Expected ([string]$lock.emscripten.cLauncherSha256) `
    -Description 'pinned emcc launcher'
$emxxLauncher = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.cxxLauncher) `
    -Description 'pinned em++ launcher'
$null = Assert-FileSha256 `
    -Path $emxxLauncher `
    -Expected ([string]$lock.emscripten.cxxLauncherSha256) `
    -Description 'pinned em++ launcher'

$vcpkgCommand = Get-ProvenanceContainedPath `
    -Root $vcpkg `
    -Relative ([string]$lock.vcpkg.executable) `
    -Description 'pinned vcpkg executable'
$vcpkgCommand = Install-AuthenticatedDownload `
    -Uri ([string]$lock.vcpkg.toolUrl) `
    -Destination $vcpkgCommand `
    -ExpectedSha256 ([string]$lock.vcpkg.executableSha256) `
    -Description 'pinned vcpkg executable'
$vcpkgVersion = @(& $vcpkgCommand version --disable-metrics)
if ($LASTEXITCODE -ne 0 -or
    ($vcpkgVersion -join "`n") -notmatch
    '(?m)^vcpkg package management program version ') {
    throw 'Pinned vcpkg version probe failed'
}
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
$portCmakeContract = $lock.vcpkg.portBuildCMake
$null = Assert-VcpkgPortCMakeManifest `
    -Vcpkg $vcpkg `
    -Contract $portCmakeContract
$portDownloads = Get-StrictDescendantPath `
    -Path (Join-Path $VcpkgStateRoot 'downloads') `
    -Root $VcpkgStateRoot `
    -Description 'vcpkg downloads root'
New-Item -ItemType Directory -Path $portDownloads -Force | Out-Null
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
New-Item -ItemType Directory -Path $portTools -Force | Out-Null
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
$archivePresent = Test-Path -LiteralPath $portArchive -PathType Leaf
$installationPresent = Test-Path `
    -LiteralPath $portInstallation `
    -PathType Container
if ($installationPresent -and -not $archivePresent) {
    throw (
        'Refusing to provision around an unauthenticated existing vcpkg ' +
        'port-build CMake installation'
    )
}
if ($archivePresent) {
    $archiveItem = Get-Item -LiteralPath $portArchive -Force
    if ([long]$archiveItem.Length -ne
        [long]$portCmakeContract.archiveBytes) {
        throw 'vcpkg port-build CMake archive byte count drifted'
    }
    $archivePayload = Get-AuthenticatedZipPayload `
        -Archive $portArchive `
        -ExpectedArchiveSha512 ([string]$portCmakeContract.sha512) `
        -StripPrefix ([string]$portCmakeContract.payload.stripPrefix)
    foreach ($field in @(
        'FileCount',
        'DirectoryCount',
        'TotalBytes',
        'InventorySha256',
        'DirectoryInventorySha256',
        'AggregateSha256'
    )) {
        $contractField = switch ($field) {
            'FileCount' { 'fileCount' }
            'DirectoryCount' { 'directoryCount' }
            'TotalBytes' { 'totalBytes' }
            'InventorySha256' { 'inventorySha256' }
            'DirectoryInventorySha256' { 'directoryInventorySha256' }
            'AggregateSha256' { 'aggregateSha256' }
        }
        if ([string]$archivePayload.$field -cne
            [string]$portCmakeContract.payload.$contractField) {
            throw "vcpkg port-build CMake archive $field drifted"
        }
    }
}
if ($installationPresent) {
    Assert-BuildToolInstallation `
        -Name 'vcpkg port-build CMake' `
        -Archive $portArchive `
        -Installation $portInstallation `
        -Artifact $portCmakeContract | Out-Null
}
else {
    & $vcpkgCommand fetch cmake `
        "--downloads-root=$portDownloads" `
        "--vcpkg-root=$vcpkg"
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to fetch pinned vcpkg port-build CMake'
    }
    if (-not (Test-Path -LiteralPath $portArchive -PathType Leaf)) {
        throw 'vcpkg fetch did not retain the port-build CMake archive'
    }
    $fetchedArchive = Get-Item -LiteralPath $portArchive -Force
    if ([long]$fetchedArchive.Length -ne
        [long]$portCmakeContract.archiveBytes) {
        throw 'Fetched vcpkg port-build CMake archive byte count drifted'
    }
    Assert-BuildToolInstallation `
        -Name 'vcpkg port-build CMake' `
        -Archive $portArchive `
        -Installation $portInstallation `
        -Artifact $portCmakeContract | Out-Null
}
$portCmakeCommand = Get-ProvenanceContainedPath `
    -Root $portInstallation `
    -Relative ([string]$portCmakeContract.executable) `
    -Description 'vcpkg port-build CMake executable'
$null = Assert-FileSha256 `
    -Path $portCmakeCommand `
    -Expected ([string]$portCmakeContract.executableSha256) `
    -Description 'vcpkg port-build CMake executable'
$portCmakeLock = Open-AuthenticatedFileReadLock `
    -Path $portCmakeCommand `
    -ExpectedSha256 ([string]$portCmakeContract.executableSha256) `
    -Description 'vcpkg port-build CMake executable'
try {
    $portCmakeVersion = @(& $portCmakeCommand --version)
    if ($LASTEXITCODE -ne 0 -or
        ($portCmakeVersion -join "`n") -notmatch (
            "(?m)^cmake version " +
            "$([Regex]::Escape([string]$portCmakeContract.version))$"
        )) {
        throw 'Pinned vcpkg port-build CMake version probe failed'
    }
}
finally {
    $portCmakeLock.Stream.Dispose()
}

$python = Get-ProvenanceContainedPath `
    -Root $emsdk `
    -Relative ([string]$lock.emscripten.pythonExecutable) `
    -Description 'pinned Emscripten Python'
$cacheIdentityHelper = (
    Resolve-Path -LiteralPath (
        Join-Path $PSScriptRoot 'emscripten_cache_identity.py'
    ) -ErrorAction Stop
).Path
foreach ($cacheIdentityInput in @(
    [PSCustomObject]@{
        Path = $python
        Description = 'Pinned Emscripten Python'
    },
    [PSCustomObject]@{
        Path = $cacheIdentityHelper
        Description = 'Tracked Emscripten cache identity helper'
    }
)) {
    $cacheIdentityItem = Get-Item `
        -LiteralPath $cacheIdentityInput.Path `
        -Force
    if (($cacheIdentityItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0 -or
        -not (Test-Path -LiteralPath $cacheIdentityInput.Path -PathType Leaf)) {
        throw "$($cacheIdentityInput.Description) must be a regular file"
    }
}

$cacheDirectory = Assert-SafeLeafName `
    -Value ([string]$lock.emscripten.cache.directory) `
    -Field 'emscripten.cache.directory'
$cache = Get-StrictDescendantPath `
    -Path (Join-Path $ToolchainRoot $cacheDirectory) `
    -Root $ToolchainRoot `
    -Description 'Emscripten frozen cache'
$cacheReady = $false
if (Test-Path -LiteralPath $cache) {
    $cacheItem = Get-Item -LiteralPath $cache -Force
    if (($cacheItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0 -or
        -not $cacheItem.PSIsContainer) {
        throw 'Emscripten frozen cache must be a regular directory'
    }
    $cacheChildren = @(Get-ChildItem -LiteralPath $cache -Force)
    if ($cacheChildren.Count -gt 0) {
        $null = Assert-EmscriptenCacheIdentity `
            -CacheRoot $cache `
            -Contract $lock.emscripten.cache `
            -Python $python `
            -Helper $cacheIdentityHelper
        $cacheReady = $true
    }
}
if (-not $cacheReady) {
    if (-not $InitializeEmscriptenCache) {
        throw (
            'The frozen Emscripten cache is not initialized; rerun with ' +
            '-InitializeEmscriptenCache'
        )
    }
    if (-not (Test-Path -LiteralPath $cache)) {
        New-Item -ItemType Directory -Path $cache | Out-Null
    }
    $cache = Assert-DirectoryNotReparse `
        -Path $cache `
        -Description 'Emscripten cache initialization root'
    if (@(Get-ChildItem -LiteralPath $cache -Force).Count -ne 0) {
        throw (
            'Emscripten cache initialization requires the canonical cache ' +
            'root to be exact-empty'
        )
    }
    $emscriptenRoot = Get-ProvenanceContainedPath `
        -Root $emsdk `
        -Relative 'upstream/emscripten' `
        -Description 'Emscripten root'
    $node = Get-ProvenanceContainedPath `
        -Root $emsdk `
        -Relative ([string]$lock.emscripten.nodeExecutable) `
        -Description 'pinned Emscripten Node'
    $embuilder = Get-ProvenanceContainedPath `
        -Root $emsdk `
        -Relative 'upstream/emscripten/embuilder.py' `
        -Description 'pinned Emscripten embuilder'
    $prewarmAdapter = (
        Resolve-Path -LiteralPath (
            Join-Path $PSScriptRoot 'prewarm_emscripten_cache.py'
        ) -ErrorAction Stop
    ).Path
    foreach ($leaf in @($embuilder, $prewarmAdapter)) {
        $item = Get-Item -LiteralPath $leaf -Force
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0 -or
            -not (Test-Path -LiteralPath $leaf -PathType Leaf)) {
            throw "Pinned cache initializer input is not a regular file: $leaf"
        }
    }
    $env:EMSDK = $emsdk
    $env:EMSCRIPTEN_ROOT = $emscriptenRoot
    $env:EMSCRIPTEN_VERSION = $emsdkVersion
    $env:EM_CONFIG = Join-Path $emsdk '.emscripten'
    $env:EMSDK_PYTHON = $python
    $env:EMSDK_NODE = $node
    $env:EM_CACHE = $cache
    $env:EMCC_CORES = [string]$lock.emscripten.cache.prewarmCores
    $env:PYTHONDONTWRITEBYTECODE = '1'
    Remove-Item Env:EM_FROZEN_CACHE -ErrorAction SilentlyContinue
    if ($env:EMCC_CORES -cne '4') {
        throw 'Emscripten cache prewarm fanout must be exactly 4'
    }
    $prefixMap = $lock.emscripten.cache.compilerPathPrefixMap
    if ([string]$prefixMap.injection -cne
        'tracked-python-get_base_cflags-adapter' -or
        [string]$prefixMap.flag -cne '-ffile-prefix-map' -or
        [string]$prefixMap.systemLibsSha256 -notmatch
        '^[0-9a-f]{64}$' -or
        [string]$prefixMap.target -cne '/emsdk/cache') {
        throw 'Emscripten compiler path-prefix-map contract drifted'
    }
    $prewarmLocks = [Collections.Generic.List[IDisposable]]::new()
    try {
        $prewarmLocks.Add((
            Open-AuthenticatedFileReadLock `
                -Path $python `
                -ExpectedSha256 (
                    [string]$lock.emscripten.bootstrapPython.executableSha256
                ) `
                -Description 'Pinned Emscripten Python'
        ).Stream)
        $prewarmLocks.Add((
            Open-AuthenticatedFileReadLock `
                -Path $node `
                -ExpectedSha256 (
                    [string]$lock.emscripten.nodeExecutableSha256
                ) `
                -Description 'Pinned Emscripten Node'
        ).Stream)
        $pythonImportClosure = Open-EmscriptenPythonImportClosure `
            -Root $emscriptenRoot `
            -Contract $lock.emscripten.driverApi.pythonImportClosure
        foreach ($pythonModuleLock in @($pythonImportClosure.Streams)) {
            $prewarmLocks.Add($pythonModuleLock)
        }
        & $python -I -B $prewarmAdapter `
            --emscripten-root $emscriptenRoot `
            --cache-root $cache `
            --prefix-target ([string]$prefixMap.target) `
            --system-libs-sha256 ([string]$prefixMap.systemLibsSha256) `
            --emcc-launcher-sha256 (
                [string]$lock.emscripten.cLauncherSha256
            ) `
            --emcc-py-sha256 (
                [string]$lock.emscripten.driverApi.emccPySha256
            ) `
            --emxx-launcher-sha256 (
                [string]$lock.emscripten.cxxLauncherSha256
            ) `
            --emxx-py-sha256 (
                [string]$lock.emscripten.driverApi.emxxPySha256
            ) `
            --emar-launcher-sha256 (
                [string]$lock.emscripten.driverApi.emarLauncherSha256
            ) `
            --emar-py-sha256 (
                [string]$lock.emscripten.driverApi.emarPySha256
            ) `
            --emranlib-launcher-sha256 (
                [string]$lock.emscripten.driverApi.emranlibLauncherSha256
            ) `
            --emranlib-py-sha256 (
                [string]$lock.emscripten.driverApi.emranlibPySha256
            )
        $prewarmExitCode = $LASTEXITCODE
    }
    finally {
        for (
            $prewarmLockIndex = $prewarmLocks.Count - 1;
            $prewarmLockIndex -ge 0;
            $prewarmLockIndex--
        ) {
            $prewarmLocks[$prewarmLockIndex].Dispose()
        }
    }
    if ($prewarmExitCode -ne 0) {
        throw (
            'Pinned Emscripten SYSTEM cache prewarm failed; the ' +
            'canonical partial cache was retained for diagnosis and must ' +
            'be made exact-empty before an explicit retry'
        )
    }
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
    Remove-EmscriptenVolatileCacheProducts `
        -CacheRoot $cache
    $null = Assert-EmscriptenCacheIdentity `
        -CacheRoot $cache `
        -Contract $lock.emscripten.cache `
        -Python $python `
        -Helper $cacheIdentityHelper
}

Write-Output "EMSDK=$emsdk"
Write-Output "VCPKG_ROOT=$vcpkg"
Write-Output "CMAKE_ROOT=$cmake"
Write-Output "NINJA_ROOT=$ninja"
Write-Output "VCPKG_PORT_CMAKE=$portCmakeCommand"
Write-Output "EM_CACHE=$cache"
