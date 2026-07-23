[CmdletBinding()]
param(
    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'
$lockPath = Join-Path $PSScriptRoot '..\toolchain-lock.json'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
$ToolchainRoot = [IO.Path]::GetFullPath($ToolchainRoot)
$emsdkVersion = [string]$lock.emscripten.version
$emsdkCommit = [string]$lock.emscripten.emsdkCommit
$vcpkgCommit = [string]$lock.vcpkg.baseline
$emsdk = Join-Path $ToolchainRoot "emsdk-$emsdkVersion"
$vcpkg = Join-Path $ToolchainRoot (
    "vcpkg-$($vcpkgCommit.Substring(0, 8))"
)
$downloads = Join-Path $ToolchainRoot 'downloads'

New-Item -ItemType Directory -Path $downloads -Force | Out-Null

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

function Install-Repository {
    param(
        [string]$Repository,
        [string]$Url,
        [string]$ExpectedCommit,
        [string]$DisplayName
    )

    if (Test-Path -LiteralPath $Repository) {
        Assert-Commit -Repository $Repository -Expected $ExpectedCommit
        return
    }

    $temporary = "$Repository.bootstrap-tmp"
    if (Test-Path -LiteralPath $temporary) {
        Remove-Item -LiteralPath $temporary -Recurse -Force
    }

    try {
        & git clone --filter=blob:none $Url $temporary
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            throw "Failed to clone $DisplayName (exit $exitCode)"
        }

        & git -C $temporary checkout --detach $ExpectedCommit
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            throw (
                "Failed to check out pinned $DisplayName " +
                "(exit $exitCode)"
            )
        }

        Assert-Commit `
            -Repository $temporary `
            -Expected $ExpectedCommit
        Move-Item -LiteralPath $temporary -Destination $Repository
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            Remove-Item -LiteralPath $temporary -Recurse -Force
        }
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

function Resolve-ApplicationInDirectory {
    param(
        [string]$Name,
        [string]$Directory
    )

    foreach ($extension in '.exe', '.cmd', '.bat') {
        $candidate = Join-Path $Directory "$Name$extension"
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            continue
        }
        $command = Get-Command `
            -Name $candidate `
            -CommandType Application `
            -ErrorAction Stop
        $source = (Resolve-Path -LiteralPath $command.Source).Path
        if (-not (Test-Descendant -Path $source -Root $Directory)) {
            throw "Resolved $Name outside $Directory`: $source"
        }
        return $source
    }
    throw "Expected $Name application beneath $Directory"
}

function Assert-BuildTool {
    param(
        [string]$Name,
        [string]$Directory,
        [string]$Version
    )

    $commandDirectory = if ($Name -eq 'cmake') {
        Join-Path $Directory 'bin'
    }
    else {
        $Directory
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

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
    $destinationRoot = [IO.Path]::GetFullPath($Destination)
    $destinationPrefix = (
        $destinationRoot.TrimEnd(
            [IO.Path]::DirectorySeparatorChar,
            [IO.Path]::AltDirectorySeparatorChar
        ) + [IO.Path]::DirectorySeparatorChar
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
            $target = [IO.Path]::GetFullPath(
                (Join-Path $destinationRoot $nativeRelative)
            )
            if (-not $target.StartsWith(
                $destinationPrefix,
                [StringComparison]::OrdinalIgnoreCase
            )) {
                throw "Archive entry escapes destination: $relative"
            }

            if (-not $entry.Name) {
                New-Item `
                    -ItemType Directory `
                    -Path $target `
                    -Force | Out-Null
                continue
            }
            $parent = Split-Path -Parent $target
            New-Item `
                -ItemType Directory `
                -Path $parent `
                -Force | Out-Null
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
        [switch]$ArchiveHasTopLevelDirectory
    )

    $directoryName = [string]$Artifact.directory
    $canonical = Join-Path $ToolchainRoot $directoryName
    $version = [string]$Artifact.version
    if (Test-Path -LiteralPath $canonical) {
        Assert-BuildTool `
            -Name $Name `
            -Directory $canonical `
            -Version $version
        return $canonical
    }

    $download = Join-Path $downloads "$directoryName.zip.download-tmp"
    $temporary = "$canonical.bootstrap-tmp"
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
    -ArchiveHasTopLevelDirectory
$ninja = Install-BuildTool `
    -Name 'ninja' `
    -Artifact $lock.buildTools.ninja

& (Join-Path $emsdk 'emsdk.bat') install $emsdkVersion
if ($LASTEXITCODE -ne 0) {
    throw "Failed to install Emscripten $emsdkVersion"
}
& (Join-Path $emsdk 'emsdk.bat') activate $emsdkVersion
if ($LASTEXITCODE -ne 0) {
    throw (
        "Failed to activate Emscripten $emsdkVersion " +
        'in the isolated emsdk tree'
    )
}

& (Join-Path $vcpkg 'bootstrap-vcpkg.bat') -disableMetrics
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to bootstrap pinned vcpkg'
}

Write-Output "EMSDK=$emsdk"
Write-Output "VCPKG_ROOT=$vcpkg"
Write-Output "CMAKE_ROOT=$cmake"
Write-Output "NINJA_ROOT=$ninja"
