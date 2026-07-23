[CmdletBinding()]
param(
    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'

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

$lockPath = Join-Path $PSScriptRoot '..\toolchain-lock.json'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
$ToolchainRoot = [IO.Path]::GetFullPath($ToolchainRoot)
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

function Assert-Commit {
    param(
        [string]$Repository,
        [string]$Expected
    )

    $repositoryCandidate = Get-StrictDescendantPath `
        -Path $Repository `
        -Root $ToolchainRoot `
        -Description 'Repository path'
    $repositoryPath = (
        Resolve-Path -LiteralPath $repositoryCandidate
    ).Path
    $repositoryPath = Get-StrictDescendantPath `
        -Path $repositoryPath `
        -Root $ToolchainRoot `
        -Description 'Resolved repository path'
    $actualLines = @(& git -C $repositoryPath rev-parse HEAD)
    $exitCode = $LASTEXITCODE
    $actual = ($actualLines -join "`n").Trim()
    if ($exitCode -ne 0 -or $actual -ne $Expected) {
        throw "Expected $repositoryPath at $Expected, got $actual"
    }
}

function Install-Repository {
    param(
        [string]$Repository,
        [string]$Url,
        [string]$ExpectedCommit,
        [string]$DisplayName
    )

    $repositoryPath = Get-StrictDescendantPath `
        -Path $Repository `
        -Root $ToolchainRoot `
        -Description "$DisplayName canonical path"
    if (Test-Path -LiteralPath $repositoryPath) {
        Assert-Commit `
            -Repository $repositoryPath `
            -Expected $ExpectedCommit
        return
    }

    $temporary = Get-StrictDescendantPath `
        -Path "$repositoryPath.bootstrap-tmp" `
        -Root $ToolchainRoot `
        -Description "$DisplayName temporary path"
    if (Test-Path -LiteralPath $temporary) {
        Remove-Item -LiteralPath $temporary -Recurse -Force
    }

    try {
        & git clone --filter=blob:none $Url $temporary
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            throw "Failed to clone $DisplayName (exit $exitCode)"
        }

        $resolvedTemporary = (
            Resolve-Path -LiteralPath $temporary
        ).Path
        $resolvedTemporary = Get-StrictDescendantPath `
            -Path $resolvedTemporary `
            -Root $ToolchainRoot `
            -Description "$DisplayName resolved temporary path"
        & git -C $resolvedTemporary checkout --detach $ExpectedCommit
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            throw (
                "Failed to check out pinned $DisplayName " +
                "(exit $exitCode)"
            )
        }

        Assert-Commit `
            -Repository $resolvedTemporary `
            -Expected $ExpectedCommit
        Move-Item `
            -LiteralPath $resolvedTemporary `
            -Destination $repositoryPath
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Recurse -Force
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
        [string]$Version
    )

    $toolDirectory = Get-StrictDescendantPath `
        -Path $Directory `
        -Root $ToolchainRoot `
        -Description "$Name tool directory"
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
    $versionLines = @(& $command --version)
    $exitCode = $LASTEXITCODE
    $versionText = ($versionLines -join "`n").Trim()
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

function Expand-ArtifactArchive {
    param(
        [string]$ArchivePath,
        [string]$Destination,
        [AllowEmptyString()]
        [string]$StripPrefix
    )

    $archiveCandidate = Get-StrictDescendantPath `
        -Path $ArchivePath `
        -Root $downloads `
        -Description 'Artifact archive path'
    $archivePathResolved = (
        Resolve-Path -LiteralPath $archiveCandidate
    ).Path
    $archivePathResolved = Get-StrictDescendantPath `
        -Path $archivePathResolved `
        -Root $downloads `
        -Description 'Resolved artifact archive path'
    $destinationCandidate = Get-StrictDescendantPath `
        -Path $Destination `
        -Root $ToolchainRoot `
        -Description 'Artifact extraction destination'
    $destinationRoot = (
        Resolve-Path -LiteralPath $destinationCandidate
    ).Path
    $destinationRoot = Get-StrictDescendantPath `
        -Path $destinationRoot `
        -Root $ToolchainRoot `
        -Description 'Resolved artifact extraction destination'

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead(
        $archivePathResolved
    )
    $normalizedPrefix = $StripPrefix.Trim('/').Replace('\', '/')
    try {
        foreach ($entry in $archive.Entries) {
            $relative = $entry.FullName.Replace('\', '/')
            if ($normalizedPrefix) {
                if ($relative -eq $normalizedPrefix) {
                    continue
                }
                $archivePrefix = "$normalizedPrefix/"
                if (-not $relative.StartsWith(
                    $archivePrefix,
                    [StringComparison]::Ordinal
                )) {
                    throw "Unexpected archive entry: $relative"
                }
                $relative = $relative.Substring($archivePrefix.Length)
            }
            if (-not $relative) {
                continue
            }

            $nativeRelative = $relative.Replace(
                '/',
                [IO.Path]::DirectorySeparatorChar
            )
            $target = Get-StrictDescendantPath `
                -Path (Join-Path $destinationRoot $nativeRelative) `
                -Root $destinationRoot `
                -Description "Archive entry '$relative'"

            if (-not $entry.Name) {
                New-Item `
                    -ItemType Directory `
                    -Path $target `
                    -Force | Out-Null
                continue
            }
            $parent = Split-Path -Parent $target
            if ($parent -ne $destinationRoot) {
                $parent = Get-StrictDescendantPath `
                    -Path $parent `
                    -Root $destinationRoot `
                    -Description "Archive entry parent '$relative'"
                New-Item `
                    -ItemType Directory `
                    -Path $parent `
                    -Force | Out-Null
            }
            [IO.Compression.ZipFileExtensions]::ExtractToFile(
                $entry,
                $target,
                $true
            )
        }
    }
    finally {
        $archive.Dispose()
    }
}

function Install-BuildTool {
    param(
        [string]$Name,
        [PSCustomObject]$Artifact,
        [string]$DirectoryName,
        [switch]$ArchiveHasTopLevelDirectory
    )

    $directoryName = Assert-SafeLeafName `
        -Value $DirectoryName `
        -Field "buildTools.$Name.directory"
    $canonical = Get-StrictDescendantPath `
        -Path (Join-Path $ToolchainRoot $directoryName) `
        -Root $ToolchainRoot `
        -Description "$Name canonical path"
    $version = [string]$Artifact.version
    if (Test-Path -LiteralPath $canonical) {
        Assert-BuildTool `
            -Name $Name `
            -Directory $canonical `
            -Version $version
        return $canonical
    }

    $download = Get-StrictDescendantPath `
        -Path (Join-Path $downloads "$directoryName.zip.download-tmp") `
        -Root $downloads `
        -Description "$Name download path"
    $temporary = Get-StrictDescendantPath `
        -Path "$canonical.bootstrap-tmp" `
        -Root $ToolchainRoot `
        -Description "$Name temporary path"
    foreach ($ownedPath in $download, $temporary) {
        if (Test-Path -LiteralPath $ownedPath) {
            Remove-Item -LiteralPath $ownedPath -Recurse -Force
        }
    }

    try {
        Invoke-WebRequest `
            -Uri ([string]$Artifact.url) `
            -OutFile $download
        $actualHash = (
            Get-FileHash -LiteralPath $download -Algorithm SHA256
        ).Hash.ToLowerInvariant()
        $expectedHash = ([string]$Artifact.sha256).ToLowerInvariant()
        if ($actualHash -ne $expectedHash) {
            throw (
                "Expected $Name archive SHA-256 $expectedHash, " +
                "got $actualHash"
            )
        }

        New-Item `
            -ItemType Directory `
            -Path $temporary `
            -Force | Out-Null
        $stripPrefix = if ($ArchiveHasTopLevelDirectory) {
            $directoryName
        }
        else {
            ''
        }
        Expand-ArtifactArchive `
            -ArchivePath $download `
            -Destination $temporary `
            -StripPrefix $stripPrefix
        Assert-BuildTool `
            -Name $Name `
            -Directory $temporary `
            -Version $version
        Move-Item -LiteralPath $temporary -Destination $canonical
        return $canonical
    }
    finally {
        if (Test-Path -LiteralPath $download) {
            Remove-Item -LiteralPath $download -Force
        }
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Recurse -Force
        }
    }
}

Install-Repository `
    -Repository $emsdk `
    -Url 'https://github.com/emscripten-core/emsdk.git' `
    -ExpectedCommit $emsdkCommit `
    -DisplayName 'emsdk'
Install-Repository `
    -Repository $vcpkg `
    -Url 'https://github.com/microsoft/vcpkg.git' `
    -ExpectedCommit $vcpkgCommit `
    -DisplayName 'vcpkg'

$cmake = Install-BuildTool `
    -Name 'cmake' `
    -Artifact $lock.buildTools.cmake `
    -DirectoryName $cmakeDirectory `
    -ArchiveHasTopLevelDirectory
$ninja = Install-BuildTool `
    -Name 'ninja' `
    -Artifact $lock.buildTools.ninja `
    -DirectoryName $ninjaDirectory

$emsdkCommand = Resolve-ApplicationInDirectory `
    -Name 'emsdk' `
    -Directory $emsdk
$vcpkgBootstrap = Resolve-ApplicationInDirectory `
    -Name 'bootstrap-vcpkg' `
    -Directory $vcpkg

& $emsdkCommand install $emsdkVersion
if ($LASTEXITCODE -ne 0) {
    throw "Failed to install Emscripten $emsdkVersion"
}
& $emsdkCommand activate $emsdkVersion
if ($LASTEXITCODE -ne 0) {
    throw (
        "Failed to activate Emscripten $emsdkVersion " +
        'in the isolated emsdk tree'
    )
}

& $vcpkgBootstrap -disableMetrics
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to bootstrap pinned vcpkg'
}

Write-Output "EMSDK=$emsdk"
Write-Output "VCPKG_ROOT=$vcpkg"
Write-Output "CMAKE_ROOT=$cmake"
Write-Output "NINJA_ROOT=$ninja"
