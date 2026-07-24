function Assert-FileSha256 {
    param(
        [string]$Path,
        [string]$Expected,
        [string]$Description
    )

    if ($Expected -notmatch '^[0-9a-fA-F]{64}$') {
        throw "$Description expected SHA-256 is invalid"
    }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description is missing: $Path"
    }
    $actual = (
        Get-FileHash -LiteralPath $Path -Algorithm SHA256
    ).Hash.ToLowerInvariant()
    $expectedNormalized = $Expected.ToLowerInvariant()
    if ($actual -ne $expectedNormalized) {
        throw (
            "$Description SHA-256 drifted: expected " +
            "$expectedNormalized, got $actual"
        )
    }
    return $actual
}

function Assert-DirectoryNotReparse {
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "$Description is missing: $Path"
    }
    $item = Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Description reparse point is forbidden: $Path"
    }
    return [IO.Path]::GetFullPath($item.FullName)
}

function Assert-RepositoryClean {
    param(
        [string]$Repository,
        [string]$ExpectedCommit,
        [string]$Description
    )

    $headLines = @(& git -C $Repository rev-parse HEAD)
    $headExit = $LASTEXITCODE
    $actualHead = ($headLines -join "`n").Trim()
    if ($headExit -ne 0 -or $actualHead -ne $ExpectedCommit) {
        throw (
            "Expected $Description at $ExpectedCommit, got $actualHead"
        )
    }

    & git -C $Repository diff --quiet --no-ext-diff --no-textconv `
        --ignore-submodules=all --
    $worktreeExit = $LASTEXITCODE
    if ($worktreeExit -eq 1) {
        throw "$Description has unstaged tracked changes"
    }
    if ($worktreeExit -ne 0) {
        throw (
            "Unable to verify $Description tracked worktree " +
            "(git exit $worktreeExit)"
        )
    }

    & git -C $Repository diff --cached --quiet --no-ext-diff `
        --no-textconv --ignore-submodules=all HEAD --
    $indexExit = $LASTEXITCODE
    if ($indexExit -eq 1) {
        throw "$Description has staged tracked changes"
    }
    if ($indexExit -ne 0) {
        throw (
            "Unable to verify $Description index " +
            "(git exit $indexExit)"
        )
    }
    return $actualHead
}

function Assert-ProvenanceRelativePath {
    param(
        [string]$Value,
        [string]$Description
    )

    $normalized = $Value.Replace('\', '/')
    $parts = $normalized.Split('/')
    $invalid = [string]::IsNullOrWhiteSpace($normalized) -or
        [IO.Path]::IsPathRooted($normalized) -or
        $normalized.StartsWith('/') -or
        $normalized -match '^[A-Za-z]:' -or
        $parts -contains '' -or
        $parts -contains '.' -or
        $parts -contains '..'
    if (-not $invalid) {
        foreach ($part in $parts) {
            $baseName = $part.Split('.')[0]
            if ($part.IndexOfAny(
                [IO.Path]::GetInvalidFileNameChars()
            ) -ge 0 -or
                $part.EndsWith(' ') -or
                $part.EndsWith('.') -or
                $baseName -match (
                    '^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$'
                )) {
                $invalid = $true
                break
            }
        }
    }
    if ($invalid) {
        throw "$Description must be a safe relative path: '$Value'"
    }
    return $normalized
}

function Get-ProvenanceContainedPath {
    param(
        [string]$Root,
        [string]$Relative,
        [string]$Description
    )

    $rootFull = [IO.Path]::GetFullPath($Root)
    $relativeSafe = Assert-ProvenanceRelativePath `
        -Value $Relative `
        -Description $Description
    $candidate = [IO.Path]::GetFullPath((
        Join-Path $rootFull $relativeSafe.Replace(
            '/',
            [IO.Path]::DirectorySeparatorChar
        )
    ))
    $check = [IO.Path]::GetRelativePath($rootFull, $candidate)
    if ($check -eq '..' -or
        [IO.Path]::IsPathRooted($check) -or
        $check.StartsWith(
            "..$([IO.Path]::DirectorySeparatorChar)",
            [StringComparison]::Ordinal
        )) {
        throw "$Description escapes $rootFull"
    }
    return $candidate
}

function Get-StreamSha256 {
    param(
        [IO.Stream]$Stream
    )

    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return [Convert]::ToHexString(
            $sha.ComputeHash($Stream)
        ).ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-AuthenticatedZipPayload {
    param(
        [string]$Archive,
        [string]$ExpectedArchiveSha256,
        [AllowEmptyString()]
        [string]$StripPrefix = '',
        [AllowEmptyString()]
        [string]$ExtractTo = ''
    )

    if ($ExpectedArchiveSha256 -notmatch '^[0-9a-fA-F]{64}$') {
        throw 'Build-tool archive expected SHA-256 is invalid'
    }
    if (-not (Test-Path -LiteralPath $Archive -PathType Leaf)) {
        throw "Build-tool archive is missing: $Archive"
    }
    $archiveItem = Get-Item -LiteralPath $Archive -Force
    if (($archiveItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Build-tool archive reparse point is forbidden: $Archive"
    }
    $archiveFull = [IO.Path]::GetFullPath($archiveItem.FullName)

    $extractRoot = $null
    if (-not [string]::IsNullOrEmpty($ExtractTo)) {
        if (-not (Test-Path -LiteralPath $ExtractTo -PathType Container)) {
            throw "Build-tool extraction root is missing: $ExtractTo"
        }
        $extractItem = Get-Item -LiteralPath $ExtractTo -Force
        if (($extractItem.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw (
                'Build-tool extraction root reparse point is forbidden: ' +
                $ExtractTo
            )
        }
        $extractRoot = [IO.Path]::GetFullPath($extractItem.FullName)
    }

    $normalizedPrefix = $StripPrefix.Trim('/').Replace('\', '/')
    if ($normalizedPrefix) {
        $normalizedPrefix = Assert-ProvenanceRelativePath `
            -Value $normalizedPrefix `
            -Description 'Build-tool archive prefix'
    }
    $pathKinds = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    $explicitDirectories = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    $directories = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    $fileRecords = [Collections.Generic.List[object]]::new()
    Add-Type -AssemblyName System.IO.Compression
    $stream = [IO.File]::Open(
        $archiveFull,
        [IO.FileMode]::Open,
        [IO.FileAccess]::Read,
        [IO.FileShare]::Read
    )
    try {
        $hasher = [Security.Cryptography.SHA256]::Create()
        try {
            $archiveHash = [Convert]::ToHexString(
                $hasher.ComputeHash($stream)
            ).ToLowerInvariant()
            $expectedHash = $ExpectedArchiveSha256.ToLowerInvariant()
            if ($archiveHash -cne $expectedHash) {
                throw (
                    'Build-tool archive SHA-256 drifted: expected ' +
                    "$expectedHash, got $archiveHash"
                )
            }

            $stream.Position = 0
            $zip = [IO.Compression.ZipArchive]::new(
                $stream,
                [IO.Compression.ZipArchiveMode]::Read,
                $true
            )
            try {
                # Phase one is a complete name/type/collision preflight. No
                # entry content is read or extracted before every entry passes.
                foreach ($entry in $zip.Entries) {
                    $raw = [string]$entry.FullName
                    if ($raw.Contains('\')) {
                        throw (
                            'Build-tool archive backslash path is forbidden: ' +
                            $raw
                        )
                    }
                    $relative = $raw
                    $isDirectory = [string]::IsNullOrEmpty(
                        [string]$entry.Name
                    )
                    $relative = $relative.TrimEnd('/')
                    if (-not $relative) {
                        if ($isDirectory) {
                            continue
                        }
                        throw 'Build-tool archive contains an empty file path'
                    }

                    if ($normalizedPrefix) {
                        if ($relative -ceq $normalizedPrefix) {
                            if ($isDirectory) {
                                continue
                            }
                            throw (
                                'Build-tool archive prefix is a file: ' +
                                $relative
                            )
                        }
                        $archivePrefix = "$normalizedPrefix/"
                        if (-not $relative.StartsWith(
                            $archivePrefix,
                            [StringComparison]::Ordinal
                        )) {
                            throw "Unexpected build-tool archive entry: $raw"
                        }
                        $relative = $relative.Substring(
                            $archivePrefix.Length
                        )
                    }
                    $relative = Assert-ProvenanceRelativePath `
                        -Value $relative `
                        -Description "Build-tool archive entry '$raw'"

                    $attributeBits = [BitConverter]::ToUInt32(
                        [BitConverter]::GetBytes(
                            [int]$entry.ExternalAttributes
                        ),
                        0
                    )
                    $unixType = ($attributeBits -shr 16) -band 0xf000
                    if ($unixType -eq 0xa000 -or
                        ($attributeBits -band
                            [uint32][IO.FileAttributes]::ReparsePoint) -ne 0) {
                        throw (
                            'Build-tool archive link/reparse entry is ' +
                            "forbidden: $raw"
                        )
                    }
                    $expectedUnixType = if ($isDirectory) {
                        0x4000
                    }
                    else {
                        0x8000
                    }
                    if ($unixType -ne 0 -and
                        $unixType -ne $expectedUnixType) {
                        throw (
                            'Build-tool archive entry type is inconsistent: ' +
                            $raw
                        )
                    }

                    if ($isDirectory) {
                        if ($entry.Length -ne 0) {
                            throw (
                                'Build-tool archive directory has content: ' +
                                $raw
                            )
                        }
                        if (-not $explicitDirectories.Add($relative)) {
                            throw (
                                'Duplicate build-tool archive directory: ' +
                                $relative
                            )
                        }
                        if ($pathKinds.ContainsKey($relative) -and
                            $pathKinds[$relative] -cne 'directory') {
                            throw (
                                'Build-tool archive file/directory collision: ' +
                                $relative
                            )
                        }
                        if (-not $pathKinds.ContainsKey($relative)) {
                            $pathKinds.Add($relative, 'directory')
                            $directories.Add($relative, $relative)
                        }
                        continue
                    }

                    if ($pathKinds.ContainsKey($relative)) {
                        throw (
                            'Duplicate/colliding build-tool archive target: ' +
                            $relative
                        )
                    }
                    $parts = $relative.Split('/')
                    if ($parts.Count -gt 1) {
                        for ($index = 1; $index -lt $parts.Count; $index++) {
                            $parent = ($parts[0..($index - 1)] -join '/')
                            if ($pathKinds.ContainsKey($parent)) {
                                if ($pathKinds[$parent] -cne 'directory') {
                                    throw (
                                        'Build-tool archive file/directory ' +
                                        "collision: $parent"
                                    )
                                }
                            }
                            else {
                                $pathKinds.Add($parent, 'directory')
                                $directories.Add($parent, $parent)
                            }
                        }
                    }
                    $pathKinds.Add($relative, 'file')
                    $fileRecords.Add(
                        [PSCustomObject]@{
                            Relative = $relative
                            Length = [long]$entry.Length
                            Entry = $entry
                            Digest = $null
                        }
                    )
                }

                if ($fileRecords.Count -eq 0) {
                    throw 'Build-tool archive file inventory is empty'
                }

                # Phase two hashes authenticated, fully preflighted entries.
                foreach ($record in $fileRecords) {
                    $entryStream = $record.Entry.Open()
                    try {
                        $record.Digest = [Convert]::ToHexString(
                            $hasher.ComputeHash($entryStream)
                        ).ToLowerInvariant()
                    }
                    finally {
                        $entryStream.Dispose()
                    }
                }

                # Extraction happens only after both the archive hash and the
                # complete entry preflight have succeeded.
                if ($null -ne $extractRoot) {
                    [string[]]$directoryPaths = @($directories.Keys)
                    [Array]::Sort(
                        $directoryPaths,
                        [StringComparer]::Ordinal
                    )
                    foreach ($relative in $directoryPaths) {
                        $directoryTarget = Get-ProvenanceContainedPath `
                            -Root $extractRoot `
                            -Relative $relative `
                            -Description 'Build-tool archive directory'
                        New-Item `
                            -ItemType Directory `
                            -Path $directoryTarget `
                            -Force | Out-Null
                    }
                    foreach ($record in $fileRecords) {
                        $target = Get-ProvenanceContainedPath `
                            -Root $extractRoot `
                            -Relative ([string]$record.Relative) `
                            -Description 'Build-tool archive file'
                        [IO.Compression.ZipFileExtensions]::ExtractToFile(
                            $record.Entry,
                            $target,
                            $false
                        )
                    }
                }
            }
            finally {
                $zip.Dispose()
            }
        }
        finally {
            $hasher.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }

    $relativePaths = [string[]]@(
        $fileRecords | ForEach-Object { [string]$_.Relative }
    )
    [Array]::Sort($relativePaths, [StringComparer]::Ordinal)
    $recordsByPath = [Collections.Generic.Dictionary[string, object]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    foreach ($record in $fileRecords) {
        $recordsByPath.Add([string]$record.Relative, $record)
    }
    [string[]]$directoryPaths = @($directories.Keys)
    [Array]::Sort($directoryPaths, [StringComparer]::Ordinal)
    $utf8 = [Text.UTF8Encoding]::new($false)
    $inventory = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    $directoryInventory = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    $aggregate = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    [long]$totalBytes = 0
    try {
        foreach ($relative in $relativePaths) {
            $record = $recordsByPath[$relative]
            $digest = [string]$record.Digest
            $inventory.AppendData($utf8.GetBytes("$relative`n"))
            $aggregate.AppendData(
                $utf8.GetBytes("$relative`0$digest`n")
            )
            $totalBytes += [long]$record.Length
        }
        foreach ($relative in $directoryPaths) {
            $directoryInventory.AppendData(
                $utf8.GetBytes("$relative/`n")
            )
        }
        $inventoryHash = [Convert]::ToHexString(
            $inventory.GetHashAndReset()
        ).ToLowerInvariant()
        $directoryInventoryHash = [Convert]::ToHexString(
            $directoryInventory.GetHashAndReset()
        ).ToLowerInvariant()
        $aggregateHash = [Convert]::ToHexString(
            $aggregate.GetHashAndReset()
        ).ToLowerInvariant()
    }
    finally {
        $inventory.Dispose()
        $directoryInventory.Dispose()
        $aggregate.Dispose()
    }
    return [PSCustomObject]@{
        ArchiveSha256 = $archiveHash
        FileCount = $fileRecords.Count
        DirectoryCount = $directories.Count
        TotalBytes = $totalBytes
        InventorySha256 = $inventoryHash
        DirectoryInventorySha256 = $directoryInventoryHash
        AggregateSha256 = $aggregateHash
        Files = @(
            $relativePaths | ForEach-Object {
                $record = $recordsByPath[$_]
                [PSCustomObject]@{
                    Relative = [string]$record.Relative
                    Length = [long]$record.Length
                    Digest = [string]$record.Digest
                }
            }
        )
        Directories = @($directoryPaths)
    }
}

function Assert-BuildToolInstallation {
    param(
        [string]$Name,
        [string]$Archive,
        [string]$Installation,
        [PSCustomObject]$Artifact
    )

    $payload = $Artifact.payload
    if ([string]$payload.algorithm -cne
        'sha256-path-null-digest-lf-v1') {
        throw "Unsupported $Name payload digest algorithm"
    }
    $archivePayload = Get-AuthenticatedZipPayload `
        -Archive $Archive `
        -ExpectedArchiveSha256 ([string]$Artifact.sha256) `
        -StripPrefix ([string]$payload.stripPrefix)
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
            'DirectoryInventorySha256' {
                'directoryInventorySha256'
            }
            'AggregateSha256' { 'aggregateSha256' }
        }
        if ([string]$archivePayload.$field -cne
            [string]$payload.$contractField) {
            throw "$Name authenticated archive $field drifted"
        }
    }

    if (-not (Test-Path -LiteralPath $Installation -PathType Container)) {
        throw "$Name installation is missing: $Installation"
    }
    $installationItem = Get-Item -LiteralPath $Installation -Force
    if (($installationItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Name installation reparse point is forbidden"
    }
    $installationFull = [IO.Path]::GetFullPath(
        $installationItem.FullName
    )
    $installed = [Collections.Generic.Dictionary[string, object]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    $installedDirectories = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    foreach ($candidate in @(
        Get-ChildItem -LiteralPath $installationFull -Recurse -Force
    )) {
        if (($candidate.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw (
                "$Name installation reparse point is forbidden: " +
                $candidate.FullName
            )
        }
        if ($candidate.PSIsContainer) {
            $directoryRelative = [IO.Path]::GetRelativePath(
                $installationFull,
                $candidate.FullName
            ).Replace('\', '/')
            if ($installedDirectories.ContainsKey($directoryRelative)) {
                throw (
                    "$Name installation has duplicate directory: " +
                    $directoryRelative
                )
            }
            $installedDirectories.Add(
                $directoryRelative,
                $directoryRelative
            )
            continue
        }
        if (-not (Test-Path -LiteralPath $candidate.FullName -PathType Leaf)) {
            throw (
                "$Name installation contains a non-file entry: " +
                $candidate.FullName
            )
        }
        $relative = [IO.Path]::GetRelativePath(
            $installationFull,
            $candidate.FullName
        ).Replace('\', '/')
        if ($installed.ContainsKey($relative)) {
            throw "$Name installation has duplicate path: $relative"
        }
        $installed.Add($relative, $candidate)
    }
    if ($installed.Count -ne $archivePayload.FileCount) {
        throw (
            "$Name installed file count drifted: expected " +
            "$($archivePayload.FileCount), got $($installed.Count)"
        )
    }
    if ($installedDirectories.Count -ne
        $archivePayload.DirectoryCount) {
        throw (
            "$Name installed directory count drifted: expected " +
            "$($archivePayload.DirectoryCount), got " +
            $installedDirectories.Count
        )
    }
    foreach ($expectedDirectory in @($archivePayload.Directories)) {
        if (-not $installedDirectories.ContainsKey(
            [string]$expectedDirectory
        )) {
            throw (
                "$Name installed directory is missing: " +
                $expectedDirectory
            )
        }
    }
    $hasher = [Security.Cryptography.SHA256]::Create()
    [long]$installedBytes = 0
    try {
        foreach ($expected in @($archivePayload.Files)) {
            $relative = [string]$expected.Relative
            if (-not $installed.ContainsKey($relative)) {
                throw "$Name installed file is missing: $relative"
            }
            $installedFile = $installed[$relative]
            if ([long]$installedFile.Length -ne
                [long]$expected.Length) {
                throw "$Name installed file size drifted: $relative"
            }
            $fileStream = [IO.File]::Open(
                $installedFile.FullName,
                [IO.FileMode]::Open,
                [IO.FileAccess]::Read,
                [IO.FileShare]::Read
            )
            try {
                $actualHash = [Convert]::ToHexString(
                    $hasher.ComputeHash($fileStream)
                ).ToLowerInvariant()
            }
            finally {
                $fileStream.Dispose()
            }
            if ($actualHash -cne [string]$expected.Digest) {
                throw "$Name installed file SHA-256 drifted: $relative"
            }
            $installedBytes += [long]$installedFile.Length
        }
    }
    finally {
        $hasher.Dispose()
    }
    if ($installedBytes -ne $archivePayload.TotalBytes) {
        throw (
            "$Name installed byte count drifted: expected " +
            "$($archivePayload.TotalBytes), got $installedBytes"
        )
    }
    return [PSCustomObject]@{
        ArchiveSha256 = $archivePayload.ArchiveSha256
        FileCount = $archivePayload.FileCount
        DirectoryCount = $archivePayload.DirectoryCount
        TotalBytes = $archivePayload.TotalBytes
        InventorySha256 = $archivePayload.InventorySha256
        DirectoryInventorySha256 = (
            $archivePayload.DirectoryInventorySha256
        )
        AggregateSha256 = $archivePayload.AggregateSha256
    }
}

function Assert-EmscriptenBytecodeContract {
    param(
        [PSCustomObject]$Contract
    )

    $bytecode = $Contract.generatedBytecode
    if ($null -eq $bytecode) {
        throw 'Emscripten generated-bytecode contract is missing'
    }
    $actualKeys = [string[]]@($bytecode.PSObject.Properties.Name)
    [Array]::Sort($actualKeys, [StringComparer]::Ordinal)
    $expectedKeys = [string[]]@(
        'cacheDirectory',
        'fileSuffix',
        'normalization'
    )
    [Array]::Sort($expectedKeys, [StringComparer]::Ordinal)
    if ([string]::Join("`n", $actualKeys) -cne
        [string]::Join("`n", $expectedKeys)) {
        throw 'Emscripten generated-bytecode contract keys drifted'
    }
    if ([string]$bytecode.cacheDirectory -cne '__pycache__' -or
        [string]$bytecode.fileSuffix -cne '.pyc' -or
        [string]$bytecode.normalization -cne (
            'authenticate-non-bytecode-delete-cache-' +
            'authenticate-full-v1'
        )) {
        throw 'Emscripten generated-bytecode contract drifted'
    }
    return $bytecode
}

function Get-EmscriptenBytecodeCacheDirectories {
    param(
        [string]$Emsdk,
        [PSCustomObject]$Contract
    )

    $bytecodeContract = Assert-EmscriptenBytecodeContract `
        -Contract $Contract
    if (-not (Test-Path -LiteralPath $Emsdk -PathType Container)) {
        throw "emsdk root is missing: $Emsdk"
    }
    $rootItem = Get-Item -LiteralPath $Emsdk -Force
    if (($rootItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'emsdk root reparse point is forbidden'
    }
    $root = [IO.Path]::GetFullPath($rootItem.FullName)
    $candidates = @(
        Get-ChildItem -LiteralPath $root -Recurse -Force
    )
    $cacheDirectories = @(
        $candidates |
            Where-Object {
                $_.PSIsContainer -and
                $_.Name -ceq [string]$bytecodeContract.cacheDirectory
            }
    )

    foreach ($candidate in $candidates) {
        $full = [IO.Path]::GetFullPath($candidate.FullName)
        $relative = [IO.Path]::GetRelativePath($root, $full)
        if ($relative -eq '..' -or
            [IO.Path]::IsPathRooted($relative) -or
            $relative.StartsWith(
                "..$([IO.Path]::DirectorySeparatorChar)",
                [StringComparison]::Ordinal
            )) {
            throw "Emscripten bytecode cache escapes emsdk: $full"
        }
        if (-not $candidate.PSIsContainer -and
            $candidate.Name.EndsWith(
                [string]$bytecodeContract.fileSuffix,
                [StringComparison]::Ordinal
            )) {
            $parts = $relative.Split(
                [IO.Path]::DirectorySeparatorChar
            )
            if ($parts -cnotcontains
                [string]$bytecodeContract.cacheDirectory) {
                throw (
                    'Emscripten .pyc outside exact __pycache__ directory ' +
                    "is forbidden: $full"
                )
            }
        }
    }

    foreach ($directory in $cacheDirectories) {
        if (($directory.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw (
                'Emscripten bytecode cache reparse point is forbidden: ' +
                $directory.FullName
            )
        }
        foreach ($descendant in @(
            Get-ChildItem -LiteralPath $directory.FullName -Recurse -Force
        )) {
            if (($descendant.Attributes -band
                [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw (
                    'Emscripten bytecode descendant reparse point is ' +
                    "forbidden: $($descendant.FullName)"
                )
            }
            if ($descendant.PSIsContainer) {
                throw (
                    'Nested Emscripten bytecode cache directory is ' +
                    "forbidden: $($descendant.FullName)"
                )
            }
            if (-not $descendant.Name.EndsWith(
                [string]$bytecodeContract.fileSuffix,
                [StringComparison]::Ordinal
            )) {
                throw (
                    'Non-bytecode file inside Emscripten __pycache__ is ' +
                    "forbidden: $($descendant.FullName)"
                )
            }
        }
    }
    return $cacheDirectories
}

function Remove-EmscriptenBytecodeCaches {
    param(
        [string]$Emsdk,
        [PSCustomObject]$Contract
    )

    $cacheDirectories = @(
        Get-EmscriptenBytecodeCacheDirectories `
            -Emsdk $Emsdk `
            -Contract $Contract
    )
    foreach ($directory in @(
        $cacheDirectories |
            Sort-Object { $_.FullName.Length } -Descending
    )) {
        if (Test-Path -LiteralPath $directory.FullName -PathType Container) {
            Remove-Item -LiteralPath $directory.FullName -Recurse -Force
        }
    }
}

function Get-EmscriptenPayloadIdentity {
    param(
        [string]$Emsdk,
        [PSCustomObject]$Payload
    )

    if ([string]$Payload.algorithm -cne
        'sha256-path-null-digest-lf-v1') {
        throw 'Unsupported Emscripten payload digest algorithm'
    }
    $emsdkFull = [IO.Path]::GetFullPath($Emsdk)
    $prefixes = @(
        $Payload.excludedPrefixes | ForEach-Object {
            (
                Assert-ProvenanceRelativePath `
                    -Value ([string]$_).TrimEnd('/') `
                    -Description 'Emscripten excluded prefix'
            ) + '/'
        }
    )
    $segments = @($Payload.excludedSegments | ForEach-Object { [string]$_ })
    $suffixes = @($Payload.excludedSuffixes | ForEach-Object { [string]$_ })
    $files = [Collections.Generic.Dictionary[string, object]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )

    foreach ($rootValue in @($Payload.roots)) {
        $root = Get-ProvenanceContainedPath `
            -Root $emsdkFull `
            -Relative ([string]$rootValue) `
            -Description 'Emscripten payload root'
        if (-not (Test-Path -LiteralPath $root)) {
            throw "Emscripten payload root is missing: $root"
        }
        $rootItem = Get-Item -LiteralPath $root -Force
        if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne
            0) {
            throw "Emscripten payload reparse point is forbidden: $root"
        }
        $candidates = if ($rootItem.PSIsContainer) {
            @(Get-ChildItem -LiteralPath $root -Recurse -Force)
        }
        else {
            @($rootItem)
        }
        foreach ($candidate in $candidates) {
            if (($candidate.Attributes -band
                [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw (
                    'Emscripten payload reparse point is forbidden: ' +
                    $candidate.FullName
                )
            }
            if ($candidate.PSIsContainer) {
                continue
            }
            $full = [IO.Path]::GetFullPath($candidate.FullName)
            $relative = [IO.Path]::GetRelativePath(
                $emsdkFull,
                $full
            ).Replace('\', '/')
            $parts = $relative.Split('/')
            $excluded = $false
            foreach ($prefix in $prefixes) {
                if ($relative.StartsWith(
                    $prefix,
                    [StringComparison]::Ordinal
                )) {
                    $excluded = $true
                    break
                }
            }
            if (-not $excluded) {
                foreach ($segment in $segments) {
                    if ($parts -ccontains $segment) {
                        $excluded = $true
                        break
                    }
                }
            }
            if (-not $excluded) {
                foreach ($suffix in $suffixes) {
                    if ($relative.EndsWith(
                        $suffix,
                        [StringComparison]::Ordinal
                    )) {
                        $excluded = $true
                        break
                    }
                }
            }
            if ($excluded) {
                continue
            }
            if ($files.ContainsKey($relative)) {
                throw "Duplicate Emscripten payload path: $relative"
            }
            $files.Add(
                $relative,
                [PSCustomObject]@{
                    Relative = $relative
                    Full = $full
                }
            )
        }
    }
    if ($files.Count -eq 0) {
        throw 'Emscripten payload inventory is empty'
    }

    $utf8 = [Text.UTF8Encoding]::new($false)
    $inventory = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    $aggregate = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    try {
        $relativePaths = [string[]]@($files.Keys)
        [Array]::Sort(
            $relativePaths,
            [StringComparer]::Ordinal
        )
        foreach ($relative in $relativePaths) {
            $entry = $files[$relative]
            $digest = (
                Get-FileHash -LiteralPath $entry.Full -Algorithm SHA256
            ).Hash.ToLowerInvariant()
            $inventory.AppendData(
                $utf8.GetBytes("$($entry.Relative)`n")
            )
            $aggregate.AppendData(
                $utf8.GetBytes("$($entry.Relative)`0$digest`n")
            )
        }
        $inventoryHash = [Convert]::ToHexString(
            $inventory.GetHashAndReset()
        ).ToLowerInvariant()
        $aggregateHash = [Convert]::ToHexString(
            $aggregate.GetHashAndReset()
        ).ToLowerInvariant()
    }
    finally {
        $inventory.Dispose()
        $aggregate.Dispose()
    }
    return [PSCustomObject]@{
        FileCount = $files.Count
        InventorySha256 = $inventoryHash
        AggregateSha256 = $aggregateHash
    }
}

function Assert-EmscriptenInstallation {
    param(
        [string]$Emsdk,
        [string]$Version,
        [PSCustomObject]$Contract,
        [switch]$BeforeBytecodeNormalization
    )

    $null = Assert-EmscriptenBytecodeContract -Contract $Contract
    $manifestRelative = Assert-ProvenanceRelativePath `
        -Value ([string]$Contract.releaseManifest.path) `
        -Description 'Emscripten release manifest'
    $manifest = Get-ProvenanceContainedPath `
        -Root $Emsdk `
        -Relative $manifestRelative `
        -Description 'Emscripten release manifest'
    $manifestHash = Assert-FileSha256 `
        -Path $manifest `
        -Expected ([string]$Contract.releaseManifest.sha256) `
        -Description 'Emscripten release manifest'
    $releaseData = Get-Content -LiteralPath $manifest -Raw |
        ConvertFrom-Json
    $actualRelease = [string](
        $releaseData.releases.PSObject.Properties[$Version].Value
    )
    $expectedRelease = [string]$Contract.releaseHash
    if ($actualRelease -cne $expectedRelease) {
        throw (
            "Emscripten $Version release mapping drifted: expected " +
            "$expectedRelease, got $actualRelease"
        )
    }
    $expectedUrl = (
        'https://storage.googleapis.com/webassembly/' +
        'emscripten-releases-builds/win/' +
        "$expectedRelease/wasm-binaries.zip"
    )
    if ([string]$Contract.packageUrl -cne $expectedUrl) {
        throw 'Emscripten release package URL drifted'
    }

    $payloadContract = $Contract.payload
    if ($BeforeBytecodeNormalization) {
        if (@($Contract.payload.excludedSegments).Count -ne 0 -or
            @($Contract.payload.excludedSuffixes).Count -ne 0) {
            throw (
                'Emscripten full payload must have no bytecode segment or ' +
                'suffix exclusions'
            )
        }
        $null = Get-EmscriptenBytecodeCacheDirectories `
            -Emsdk $Emsdk `
            -Contract $Contract
        $payloadContract = [PSCustomObject]@{
            algorithm = [string]$Contract.payload.algorithm
            roots = @($Contract.payload.roots)
            excludedPrefixes = @($Contract.payload.excludedPrefixes)
            excludedSegments = @('__pycache__')
            excludedSuffixes = @()
            fileCount = [int]$Contract.payload.fileCount
            inventorySha256 = [string](
                $Contract.payload.inventorySha256
            )
            aggregateSha256 = [string](
                $Contract.payload.aggregateSha256
            )
        }
    }

    $payload = Get-EmscriptenPayloadIdentity `
        -Emsdk $Emsdk `
        -Payload $payloadContract
    if ($payload.FileCount -ne [int]$Contract.payload.fileCount) {
        throw (
            'Emscripten payload file count drifted: expected ' +
            "$($Contract.payload.fileCount), got $($payload.FileCount)"
        )
    }
    if ($payload.InventorySha256 -cne
        [string]$Contract.payload.inventorySha256) {
        throw 'Emscripten payload inventory SHA-256 drifted'
    }
    if ($payload.AggregateSha256 -cne
        [string]$Contract.payload.aggregateSha256) {
        throw 'Emscripten payload aggregate SHA-256 drifted'
    }
    return [PSCustomObject]@{
        ReleaseManifestSha256 = $manifestHash
        ReleaseHash = $expectedRelease
        PackageUrl = $expectedUrl
        GeneratedBytecode = [PSCustomObject]@{
            CacheDirectory = [string](
                $Contract.generatedBytecode.cacheDirectory
            )
            FileSuffix = [string]$Contract.generatedBytecode.fileSuffix
            Normalization = [string](
                $Contract.generatedBytecode.normalization
            )
        }
        FileCount = $payload.FileCount
        InventorySha256 = $payload.InventorySha256
        AggregateSha256 = $payload.AggregateSha256
    }
}
