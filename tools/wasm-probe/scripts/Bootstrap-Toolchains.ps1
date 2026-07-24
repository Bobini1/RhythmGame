[CmdletBinding()]
param(
    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Toolchain-Provenance.ps1')

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
$downloads = Assert-DirectoryNotReparse `
    -Path $downloads `
    -Description 'Download directory'

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
        Assert-RepositoryClean `
            -Repository $repositoryPath `
            -ExpectedCommit $ExpectedCommit `
            -Description $DisplayName
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

        Assert-RepositoryClean `
            -Repository $resolvedTemporary `
            -ExpectedCommit $ExpectedCommit `
            -Description $DisplayName
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
    $download = Get-StrictDescendantPath `
        -Path "$archive.download-tmp" `
        -Root $downloads `
        -Description "$Name download path"

    if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
        if (Test-Path -LiteralPath $download) {
            Remove-Item -LiteralPath $download -Force
        }
        try {
            Invoke-WebRequest `
                -Uri ([string]$Artifact.url) `
                -OutFile $download
            $null = Assert-FileSha256 `
                -Path $download `
                -Expected ([string]$Artifact.sha256) `
                -Description "$Name downloaded archive"
            Move-Item -LiteralPath $download -Destination $archive
        }
        finally {
            if (Test-Path -LiteralPath $download) {
                Remove-Item -LiteralPath $download -Force
            }
        }
    }
    $null = Assert-FileSha256 `
        -Path $archive `
        -Expected ([string]$Artifact.sha256) `
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
            Remove-Item -LiteralPath $ownedPath -Recurse -Force
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
            Remove-Item -LiteralPath $temporary -Recurse -Force
        }
    }
}

$emsdkWasPresent = Test-Path -LiteralPath $emsdk
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

$emsdkCommand = Resolve-ApplicationInDirectory `
    -Name 'emsdk' `
    -Directory $emsdk
$vcpkgBootstrap = Resolve-ApplicationInDirectory `
    -Name 'bootstrap-vcpkg' `
    -Directory $vcpkg

Assert-RepositoryClean `
    -Repository $emsdk `
    -ExpectedCommit $emsdkCommit `
    -Description 'emsdk' | Out-Null
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
& $emsdkCommand install $emsdkVersion
if ($LASTEXITCODE -ne 0) {
    throw "Failed to install Emscripten $emsdkVersion"
}
Assert-RepositoryClean `
    -Repository $emsdk `
    -ExpectedCommit $emsdkCommit `
    -Description 'emsdk' | Out-Null
& $emsdkCommand activate $emsdkVersion
if ($LASTEXITCODE -ne 0) {
    throw (
        "Failed to activate Emscripten $emsdkVersion " +
        'in the isolated emsdk tree'
    )
}
Assert-RepositoryClean `
    -Repository $emsdk `
    -ExpectedCommit $emsdkCommit `
    -Description 'emsdk' | Out-Null
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

Assert-RepositoryClean `
    -Repository $vcpkg `
    -ExpectedCommit $vcpkgCommit `
    -Description 'vcpkg' | Out-Null
& $vcpkgBootstrap -disableMetrics
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to bootstrap pinned vcpkg'
}

Write-Output "EMSDK=$emsdk"
Write-Output "VCPKG_ROOT=$vcpkg"
Write-Output "CMAKE_ROOT=$cmake"
Write-Output "NINJA_ROOT=$ninja"
