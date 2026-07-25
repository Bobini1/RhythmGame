function Clear-WasmBuildEnvironment {
    $exactNames = @(
        'AR',
        'AS',
        'BASH_ENV',
        'BINARYEN_ROOT',
        'CC',
        'CCC_OVERRIDE_OPTIONS',
        'CL',
        'COMSPEC',
        'COMPILER_PATH',
        'CPATH',
        'CPP',
        'CPPFLAGS',
        'C_INCLUDE_PATH',
        'CXX',
        'CFLAGS',
        'CPLUS_INCLUDE_PATH',
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
        'ENV',
        'LD',
        'LDFLAGS',
        'LIB',
        'LIBPATH',
        'LIBRARY_PATH',
        'LLVM_ROOT',
        'NM',
        'NODE_OPTIONS',
        'NODE_PATH',
        'NODE_JS',
        'PYTHONHOME',
        'PYTHONNOUSERSITE',
        'PYTHONPATH',
        'RANLIB',
        'SYSTEMROOT',
        'GCC_EXEC_PREFIX',
        'INCLUDE',
        'IPHONEOS_DEPLOYMENT_TARGET',
        'MACOSX_DEPLOYMENT_TARGET',
        'OBJC_INCLUDE_PATH',
        'OBJCPLUS_INCLUDE_PATH',
        'RC',
        'SDKROOT',
        'SOURCE_DATE_EPOCH',
        'STRIP',
        'QT_RCC_SOURCE_DATE_OVERRIDE',
        'VCPKG_MAX_CONCURRENCY',
        'WINDIR',
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
        'QML_',
        'QT_',
        'RHYTHMGAME_',
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
            Remove-Item `
                -LiteralPath "Env:$($entry.Name)" `
                -ErrorAction SilentlyContinue
        }
    }

    $systemDirectory = [Environment]::SystemDirectory
    if ([string]::IsNullOrWhiteSpace($systemDirectory)) {
        throw 'Native Windows system directory is unavailable'
    }
    $systemDirectory = [IO.Path]::GetFullPath($systemDirectory)
    $windowsDirectory = [IO.Directory]::GetParent($systemDirectory)
    if ($null -eq $windowsDirectory) {
        throw 'Native Windows directory is unavailable'
    }
    $cmd = Join-Path $systemDirectory 'cmd.exe'
    $null = Assert-PathChainNotReparse `
        -Path $cmd `
        -Description 'native Windows command interpreter'
    if (-not (Test-Path -LiteralPath $cmd -PathType Leaf)) {
        throw "Native Windows command interpreter is missing: $cmd"
    }
    $cmdItem = Get-Item -LiteralPath $cmd -Force
    if (($cmdItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Native Windows command interpreter is a reparse point: $cmd"
    }
    $env:ComSpec = [IO.Path]::GetFullPath($cmdItem.FullName)
    $env:SystemRoot = [IO.Path]::GetFullPath($windowsDirectory.FullName)
    $env:windir = $env:SystemRoot
    $env:PATHEXT = '.COM;.EXE;.BAT;.CMD'
}

function Assert-PathChainNotReparse {
    param(
        [string]$Path,
        [string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "$Description path is empty"
    }
    $full = [IO.Path]::GetFullPath($Path)
    $root = [IO.Path]::GetPathRoot($full)
    if ([string]::IsNullOrWhiteSpace($root)) {
        throw "$Description has no filesystem root: $full"
    }

    $relative = $full.Substring($root.Length)
    $parts = @(
        $relative.Split(
            @(
                [IO.Path]::DirectorySeparatorChar,
                [IO.Path]::AltDirectorySeparatorChar
            ),
            [StringSplitOptions]::RemoveEmptyEntries
        )
    )
    $current = $root
    $missingSuffix = $false
    $components = [Collections.Generic.List[string]]::new()
    $components.Add($root)
    foreach ($part in $parts) {
        $current = Join-Path $current $part
        $components.Add($current)
    }

    for ($index = 0; $index -lt $components.Count; $index++) {
        $component = $components[$index]
        $item = Get-Item `
            -LiteralPath $component `
            -Force `
            -ErrorAction SilentlyContinue
        if ($null -eq $item) {
            $missingSuffix = $true
            continue
        }
        if ($missingSuffix) {
            throw (
                "$Description path changed while its component chain was " +
                "checked: $component"
            )
        }
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Description reparse-point component is forbidden: $component"
        }
        if ($index -lt ($components.Count - 1) -and
            -not $item.PSIsContainer) {
            throw "$Description non-directory component is forbidden: $component"
        }
    }
    return $full
}

function Assert-FileSha256 {
    param(
        [string]$Path,
        [string]$Expected,
        [string]$Description
    )

    if ($Expected -notmatch '^[0-9a-fA-F]{64}$') {
        throw "$Description expected SHA-256 is invalid"
    }
    $Path = Assert-PathChainNotReparse `
        -Path $Path `
        -Description $Description
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

function Open-AuthenticatedFileReadLock {
    param(
        [string]$Path,
        [AllowEmptyString()]
        [string]$ExpectedSha256 = '',
        [string]$Description
    )

    $full = Assert-PathChainNotReparse `
        -Path $Path `
        -Description $Description
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        throw "$Description is missing: $full"
    }
    $item = Get-Item -LiteralPath $full -Force
    if (($item.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Description reparse point is forbidden: $full"
    }

    $stream = $null
    try {
        # FILE_SHARE_READ lets authenticated consumers reopen the file while
        # denying replacement, deletion, and write access until Dispose().
        $stream = [IO.File]::Open(
            [IO.Path]::GetFullPath($item.FullName),
            [IO.FileMode]::Open,
            [IO.FileAccess]::Read,
            [IO.FileShare]::Read
        )
        $sha = [Security.Cryptography.SHA256]::Create()
        try {
            $actual = [Convert]::ToHexString(
                $sha.ComputeHash($stream)
            ).ToLowerInvariant()
        }
        finally {
            $sha.Dispose()
        }
        if (-not [string]::IsNullOrEmpty($ExpectedSha256)) {
            if ($ExpectedSha256 -notmatch '^[0-9a-fA-F]{64}$') {
                throw "$Description expected SHA-256 is invalid"
            }
            if ($actual -cne $ExpectedSha256.ToLowerInvariant()) {
                throw (
                    "$Description SHA-256 drifted: expected " +
                    "$($ExpectedSha256.ToLowerInvariant()), got $actual"
                )
            }
        }
        $stream.Position = 0
        $result = [PSCustomObject]@{
            Path = [IO.Path]::GetFullPath($item.FullName)
            Sha256 = $actual
            Stream = $stream
        }
        $stream = $null
        return $result
    }
    finally {
        if ($null -ne $stream) {
            $stream.Dispose()
        }
    }
}

function Get-EmscriptenPythonImportFiles {
    param(
        [string]$Root
    )

    $rootFull = Assert-DirectoryNotReparse `
        -Path $Root `
        -Description 'Emscripten Python import root'
    $files = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    foreach ($candidate in @(
        Get-ChildItem -LiteralPath $rootFull -Recurse -Force
    )) {
        if (($candidate.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw (
                'Emscripten Python import surface reparse point is ' +
                "forbidden: $($candidate.FullName)"
            )
        }
        if ($candidate.PSIsContainer -or
            -not [string]::Equals(
                $candidate.Extension,
                '.py',
                [StringComparison]::OrdinalIgnoreCase
            )) {
            continue
        }
        $full = Assert-PathChainNotReparse `
            -Path $candidate.FullName `
            -Description 'Emscripten Python import module'
        $relative = [IO.Path]::GetRelativePath(
            $rootFull,
            $full
        ).Replace('\', '/')
        if ($relative -eq '..' -or
            [IO.Path]::IsPathRooted($relative) -or
            $relative.StartsWith(
                '../',
                [StringComparison]::Ordinal
            )) {
            throw "Emscripten Python import module escaped root: $full"
        }
        if ($files.ContainsKey($relative)) {
            throw (
                'Case-insensitive Emscripten Python module collision: ' +
                $relative
            )
        }
        $files.Add($relative, $full)
    }
    if ($files.Count -eq 0) {
        throw 'Emscripten Python import surface is empty'
    }
    return $files
}

function Open-EmscriptenPythonImportClosure {
    param(
        [string]$Root,
        [PSCustomObject]$Contract
    )

    if ([string]$Contract.algorithm -cne
        'sha256-path-null-digest-lf-v1') {
        throw 'Unsupported Emscripten Python import digest algorithm'
    }
    $files = Get-EmscriptenPythonImportFiles -Root $Root
    $relativePaths = [string[]]@($files.Keys)
    [Array]::Sort($relativePaths, [StringComparer]::Ordinal)

    $locks = [Collections.Generic.List[IDisposable]]::new()
    $lockedPaths = [Collections.Generic.List[string]]::new()
    $utf8 = [Text.UTF8Encoding]::new($false)
    $inventory = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    $aggregate = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    [long]$totalBytes = 0
    try {
        foreach ($relative in $relativePaths) {
            $opened = Open-AuthenticatedFileReadLock `
                -Path $files[$relative] `
                -Description "Emscripten Python module '$relative'"
            $locks.Add($opened.Stream)
            $lockedPaths.Add($relative)
            $totalBytes += $opened.Stream.Length
            $inventory.AppendData($utf8.GetBytes("$relative`n"))
            $aggregate.AppendData(
                $utf8.GetBytes("$relative`0$($opened.Sha256)`n")
            )
        }
        $inventoryHash = [Convert]::ToHexString(
            $inventory.GetHashAndReset()
        ).ToLowerInvariant()
        $aggregateHash = [Convert]::ToHexString(
            $aggregate.GetHashAndReset()
        ).ToLowerInvariant()

        # Re-enumerate only after every authenticated handle is open. This
        # catches an added, removed, or case-swapped module in the gap between
        # the initial inventory and the final lifetime lock set.
        $rechecked = Get-EmscriptenPythonImportFiles -Root $Root
        $recheckedPaths = [string[]]@($rechecked.Keys)
        [Array]::Sort($recheckedPaths, [StringComparer]::Ordinal)
        if ($recheckedPaths.Count -ne $lockedPaths.Count) {
            throw 'Emscripten Python import path set changed while locking'
        }
        for ($index = 0; $index -lt $recheckedPaths.Count; $index++) {
            if ($recheckedPaths[$index] -cne $lockedPaths[$index]) {
                throw (
                    'Emscripten Python import path set changed while ' +
                    "locking: expected '$($lockedPaths[$index])', got " +
                    "'$($recheckedPaths[$index])'"
                )
            }
        }

        if ($lockedPaths.Count -ne [int]$Contract.fileCount) {
            throw (
                'Emscripten Python import file count drifted: expected ' +
                "$($Contract.fileCount), got $($lockedPaths.Count)"
            )
        }
        if ($totalBytes -ne [long]$Contract.totalBytes) {
            throw (
                'Emscripten Python import byte count drifted: expected ' +
                "$($Contract.totalBytes), got $totalBytes"
            )
        }
        if ($inventoryHash -cne
            [string]$Contract.inventorySha256) {
            throw 'Emscripten Python import inventory SHA-256 drifted'
        }
        if ($aggregateHash -cne
            [string]$Contract.aggregateSha256) {
            throw 'Emscripten Python import aggregate SHA-256 drifted'
        }
        $result = [PSCustomObject]@{
            FileCount = $lockedPaths.Count
            TotalBytes = $totalBytes
            InventorySha256 = $inventoryHash
            AggregateSha256 = $aggregateHash
            Streams = @($locks)
        }
        $locks = $null
        return $result
    }
    finally {
        $inventory.Dispose()
        $aggregate.Dispose()
        if ($null -ne $locks) {
            for ($index = $locks.Count - 1; $index -ge 0; $index--) {
                $locks[$index].Dispose()
            }
        }
    }
}

function Get-QualificationClosureFiles {
    param(
        [object[]]$Roots,
        [object[]]$Files
    )

    $entries = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    $physicalPaths = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )

    function Add-QualificationClosureFile {
        param(
            [string]$Logical,
            [string]$Path,
            [string]$Description
        )

        if ([string]::IsNullOrWhiteSpace($Logical) -or
            $Logical.StartsWith('/') -or
            $Logical.EndsWith('/') -or
            $Logical.Contains('\') -or
            $Logical.Split('/') -contains '..' -or
            $Logical.Split('/') -contains '.') {
            throw "Unsafe qualification closure logical path: '$Logical'"
        }
        $full = Assert-PathChainNotReparse `
            -Path $Path `
            -Description $Description
        if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
            throw "$Description is missing: $full"
        }
        $item = Get-Item -LiteralPath $full -Force
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Description reparse point is forbidden: $full"
        }
        if ($entries.ContainsKey($Logical)) {
            throw (
                'Case-insensitive qualification closure logical collision: ' +
                $Logical
            )
        }
        if ($physicalPaths.ContainsKey($full)) {
            throw (
                'Qualification closure physical file is modeled twice: ' +
                "$full as '$($physicalPaths[$full])' and '$Logical'"
            )
        }
        $entries.Add($Logical, $full)
        $physicalPaths.Add($full, $Logical)
    }

    foreach ($rootEntry in @($Roots)) {
        $label = [string]$rootEntry.Label
        if ($label -notmatch '^[a-z0-9][a-z0-9-]*$') {
            throw "Unsafe qualification closure root label: '$label'"
        }
        $rawExcludedSuffixes = @()
        if ($null -ne $rootEntry.PSObject.Properties['ExcludedSuffixes']) {
            $rawExcludedSuffixes = @($rootEntry.ExcludedSuffixes)
        }
        $excludedSuffixes = @(
            $rawExcludedSuffixes | ForEach-Object {
                $suffix = [string]$_
                if ($suffix -notmatch '^\.[a-z0-9-]+$') {
                    throw (
                        'Unsafe qualification closure excluded suffix: ' +
                        "'$suffix'"
                    )
                }
                $suffix
            }
        )
        $root = Assert-DirectoryNotReparse `
            -Path ([string]$rootEntry.Path) `
            -Description "Qualification closure root '$label'"
        foreach ($candidate in @(
            Get-ChildItem -LiteralPath $root -Recurse -Force
        )) {
            if (($candidate.Attributes -band
                [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw (
                    "Qualification closure root '$label' contains a " +
                    "reparse point: $($candidate.FullName)"
                )
            }
            if ($candidate.PSIsContainer) {
                continue
            }
            if ($excludedSuffixes -ccontains
                $candidate.Extension.ToLowerInvariant()) {
                continue
            }
            $full = [IO.Path]::GetFullPath($candidate.FullName)
            $relative = [IO.Path]::GetRelativePath(
                $root,
                $full
            ).Replace('\', '/')
            if ($relative -eq '..' -or
                [IO.Path]::IsPathRooted($relative) -or
                $relative.StartsWith(
                    '../',
                    [StringComparison]::Ordinal
                )) {
                throw (
                    "Qualification closure root '$label' escaped: $full"
                )
            }
            Add-QualificationClosureFile `
                -Logical "$label/$relative" `
                -Path $full `
                -Description "Qualification closure '$label/$relative'"
        }
    }

    foreach ($fileEntry in @($Files)) {
        Add-QualificationClosureFile `
            -Logical ([string]$fileEntry.Logical) `
            -Path ([string]$fileEntry.Path) `
            -Description (
                "Qualification closure '$([string]$fileEntry.Logical)'"
            )
    }
    if ($entries.Count -eq 0) {
        throw 'Qualification closure is empty'
    }
    return $entries
}

function Open-QualificationClosure {
    param(
        [object[]]$Roots,
        [object[]]$Files
    )

    $entries = Get-QualificationClosureFiles `
        -Roots $Roots `
        -Files $Files
    $logicalPaths = [string[]]@($entries.Keys)
    [Array]::Sort($logicalPaths, [StringComparer]::Ordinal)

    $locks = [Collections.Generic.List[IDisposable]]::new()
    $lockedPaths = [Collections.Generic.List[string]]::new()
    $utf8 = [Text.UTF8Encoding]::new($false)
    $inventory = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    $aggregate = [Security.Cryptography.IncrementalHash]::CreateHash(
        [Security.Cryptography.HashAlgorithmName]::SHA256
    )
    [long]$totalBytes = 0
    try {
        foreach ($logical in $logicalPaths) {
            $opened = Open-AuthenticatedFileReadLock `
                -Path $entries[$logical] `
                -Description "Qualification closure '$logical'"
            $locks.Add($opened.Stream)
            $lockedPaths.Add($logical)
            $length = [long]$opened.Stream.Length
            $totalBytes += $length
            $inventory.AppendData($utf8.GetBytes("$logical`n"))
            $aggregate.AppendData(
                $utf8.GetBytes(
                    "$logical`0$length`0$($opened.Sha256)`n"
                )
            )
        }
        $inventoryHash = [Convert]::ToHexString(
            $inventory.GetHashAndReset()
        ).ToLowerInvariant()
        $aggregateHash = [Convert]::ToHexString(
            $aggregate.GetHashAndReset()
        ).ToLowerInvariant()

        # No file may be added, removed, case-swapped, or redirected between
        # enumeration and the point where all immutable inputs are locked.
        $rechecked = Get-QualificationClosureFiles `
            -Roots $Roots `
            -Files $Files
        $recheckedPaths = [string[]]@($rechecked.Keys)
        [Array]::Sort($recheckedPaths, [StringComparer]::Ordinal)
        if ($recheckedPaths.Count -ne $lockedPaths.Count) {
            throw 'Qualification closure path set changed while locking'
        }
        for ($index = 0; $index -lt $recheckedPaths.Count; $index++) {
            $logical = $recheckedPaths[$index]
            if ($logical -cne $lockedPaths[$index] -or
                -not [string]::Equals(
                    $rechecked[$logical],
                    $entries[$logical],
                    [StringComparison]::OrdinalIgnoreCase
                )) {
                throw (
                    'Qualification closure path set changed while locking: ' +
                    $logical
                )
            }
        }

        $result = [PSCustomObject]@{
            Algorithm = 'sha256-logical-null-bytes-null-digest-lf-v1'
            FileCount = $lockedPaths.Count
            TotalBytes = $totalBytes
            InventorySha256 = $inventoryHash
            AggregateSha256 = $aggregateHash
            Streams = @($locks)
        }
        $locks = $null
        return $result
    }
    finally {
        $inventory.Dispose()
        $aggregate.Dispose()
        if ($null -ne $locks) {
            for ($index = $locks.Count - 1; $index -ge 0; $index--) {
                $locks[$index].Dispose()
            }
        }
    }
}

function Assert-DirectoryNotReparse {
    param(
        [string]$Path,
        [string]$Description
    )

    $Path = Assert-PathChainNotReparse `
        -Path $Path `
        -Description $Description
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

function Assert-TrackedRepositoryFile {
    param(
        [string]$Repository,
        [string]$Relative,
        [string]$Description,
        [string]$ExpectedSha256
    )

    $relativePath = Assert-ProvenanceRelativePath `
        -Value $Relative.Replace('\', '/') `
        -Description "$Description tracked path"
    $candidate = Get-ProvenanceContainedPath `
        -Root $Repository `
        -Relative $relativePath `
        -Description $Description
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "$Description is missing: $candidate"
    }
    $item = Get-Item -LiteralPath $candidate -Force
    if (($item.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Description reparse point is forbidden: $candidate"
    }
    $tracked = @(
        & git -C $Repository ls-files --error-unmatch -- $relativePath
    )
    if ($LASTEXITCODE -ne 0 -or
        $tracked.Count -ne 1 -or
        [string]$tracked[0] -cne $relativePath) {
        throw (
            "$Description must be the exact tracked repository file " +
            $relativePath
        )
    }
    $null = Assert-FileSha256 `
        -Path $candidate `
        -Expected $ExpectedSha256 `
        -Description "$Description working bytes"
    return [IO.Path]::GetFullPath($item.FullName)
}

function Assert-VcpkgPortCMakeManifest {
    param(
        [string]$Vcpkg,
        [PSCustomObject]$Contract
    )

    $manifest = Get-ProvenanceContainedPath `
        -Root $Vcpkg `
        -Relative ([string]$Contract.toolsManifest) `
        -Description 'vcpkg tools manifest'
    $manifestSha256 = Assert-FileSha256 `
        -Path $manifest `
        -Expected ([string]$Contract.toolsManifestSha256) `
        -Description 'vcpkg tools manifest'
    $parsed = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
    if ([int]$parsed.'schema-version' -ne 1) {
        throw 'vcpkg tools manifest schema must be exactly 1'
    }
    $matches = @(
        $parsed.tools | Where-Object {
            [string]$_.name -ceq 'cmake' -and
            [string]$_.os -ceq 'windows' -and
            [string]$_.arch -ceq 'amd64'
        }
    )
    if ($matches.Count -ne 1) {
        throw (
            'vcpkg tools manifest must contain exactly one Windows amd64 ' +
            'CMake entry'
        )
    }
    $entry = $matches[0]
    [string[]]$actualKeys = @($entry.PSObject.Properties.Name)
    [Array]::Sort($actualKeys, [StringComparer]::Ordinal)
    [string[]]$expectedKeys = @(
        'arch',
        'archive',
        'executable',
        'name',
        'os',
        'sha512',
        'url',
        'version'
    )
    [Array]::Sort($expectedKeys, [StringComparer]::Ordinal)
    if (($actualKeys -join "`n") -cne ($expectedKeys -join "`n")) {
        throw 'vcpkg Windows amd64 CMake manifest entry keys drifted'
    }
    $expected = [ordered]@{
        name = 'cmake'
        os = 'windows'
        arch = 'amd64'
        version = [string]$Contract.version
        executable = [string]$Contract.executable
        url = [string]$Contract.url
        sha512 = [string]$Contract.sha512
        archive = [string]$Contract.archiveFile
    }
    foreach ($key in $expected.Keys) {
        if ([string]$entry.$key -cne [string]$expected[$key]) {
            throw "vcpkg Windows amd64 CMake manifest $key drifted"
        }
    }
    return [PSCustomObject]@{
        Path = $manifest
        Sha256 = $manifestSha256
        Entry = $entry
    }
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

    $rootFull = Assert-PathChainNotReparse `
        -Path $Root `
        -Description "$Description root"
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
    return (
        Assert-PathChainNotReparse `
            -Path $candidate `
            -Description $Description
    )
}

function Remove-ProvenanceOwnedTree {
    param(
        [string]$Path,
        [string]$OwnedRoot,
        [string]$Description
    )

    $ownedRootFull = Assert-PathChainNotReparse `
        -Path $OwnedRoot `
        -Description "$Description owned root"
    $pathFull = [IO.Path]::GetFullPath($Path)
    $relative = [IO.Path]::GetRelativePath($ownedRootFull, $pathFull)
    if ($relative -eq '.' -or
        $relative -eq '..' -or
        [IO.Path]::IsPathRooted($relative) -or
        $relative.StartsWith(
            "..$([IO.Path]::DirectorySeparatorChar)",
            [StringComparison]::Ordinal
        ) -or
        $relative.StartsWith(
            "..$([IO.Path]::AltDirectorySeparatorChar)",
            [StringComparison]::Ordinal
        )) {
        throw (
            "$Description removal target must be a strict descendant of " +
            "$ownedRootFull, got $pathFull"
        )
    }
    $pathFull = Assert-PathChainNotReparse `
        -Path $pathFull `
        -Description "$Description removal target"
    if (-not (Test-Path -LiteralPath $pathFull)) {
        return
    }
    $rootItem = Get-Item -LiteralPath $pathFull -Force
    if (-not $rootItem.PSIsContainer) {
        throw "$Description removal target is not a directory: $pathFull"
    }
    foreach ($descendant in @(
        Get-ChildItem -LiteralPath $pathFull -Recurse -Force
    )) {
        if (($descendant.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw (
                "$Description removal tree contains a reparse point: " +
                $descendant.FullName
            )
        }
    }
    $null = Assert-PathChainNotReparse `
        -Path $pathFull `
        -Description "$Description removal target"
    Remove-Item -LiteralPath $pathFull -Recurse -Force
}

function Get-SourceArchivePayload {
    param(
        [string]$Archive,
        [PSCustomObject]$Artifact,
        [string]$Description
    )

    if ([string]$Artifact.payload.algorithm -cne
        'sha256-path-null-digest-lf-v1') {
        throw "Unsupported $Description payload digest algorithm"
    }
    $payload = Get-AuthenticatedZipPayload `
        -Archive $Archive `
        -ExpectedArchiveSha256 ([string]$Artifact.sha256) `
        -StripPrefix ([string]$Artifact.payload.stripPrefix)
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
        if ([string]$payload.$field -cne
            [string]$Artifact.payload.$contractField) {
            throw "$Description archive $field drifted"
        }
    }
    return $payload
}

function Assert-SourceArchiveInstallation {
    param(
        [string]$Archive,
        [string]$Installation,
        [PSCustomObject]$Artifact,
        [string[]]$AllowedExtraPrefixes = @(),
        [string[]]$AllowedExtraFiles = @(),
        [string]$Description
    )

    $installationFull = Assert-DirectoryNotReparse `
        -Path $Installation `
        -Description "$Description installation"
    $payload = Get-SourceArchivePayload `
        -Archive $Archive `
        -Artifact $Artifact `
        -Description $Description
    $expectedFiles = [Collections.Generic.Dictionary[string, object]]::new(
        [StringComparer]::Ordinal
    )
    foreach ($record in @($payload.Files)) {
        $expectedFiles.Add([string]$record.Relative, $record)
    }
    $expectedDirectories = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal
    )
    foreach ($relative in @($payload.Directories)) {
        $null = $expectedDirectories.Add([string]$relative)
    }
    [string[]]$prefixes = @(
        $AllowedExtraPrefixes | ForEach-Object {
            (
                Assert-ProvenanceRelativePath `
                    -Value ([string]$_).TrimEnd('/') `
                    -Description "$Description allowed extra prefix"
            ) + '/'
        }
    )
    $extraFiles = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::Ordinal
    )
    foreach ($relative in $AllowedExtraFiles) {
        $null = $extraFiles.Add(
            (
                Assert-ProvenanceRelativePath `
                    -Value ([string]$relative) `
                    -Description "$Description allowed extra file"
            )
        )
    }

    foreach ($relative in @($expectedDirectories)) {
        $directory = Get-ProvenanceContainedPath `
            -Root $installationFull `
            -Relative $relative `
            -Description "$Description source directory"
        $null = Assert-DirectoryNotReparse `
            -Path $directory `
            -Description "$Description source directory"
    }
    foreach ($relative in @($expectedFiles.Keys)) {
        $record = $expectedFiles[$relative]
        $file = Get-ProvenanceContainedPath `
            -Root $installationFull `
            -Relative $relative `
            -Description "$Description source file"
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
            throw "$Description source file is missing: $relative"
        }
        $item = Get-Item -LiteralPath $file -Force
        if ([long]$item.Length -ne [long]$record.Length) {
            throw "$Description source file byte count drifted: $relative"
        }
        $null = Assert-FileSha256 `
            -Path $file `
            -Expected ([string]$record.Digest `
            ) `
            -Description "$Description source file '$relative'"
    }

    foreach ($item in @(
        Get-ChildItem -LiteralPath $installationFull -Recurse -Force
    )) {
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw (
                "$Description installation reparse point is forbidden: " +
                $item.FullName
            )
        }
        $relative = [IO.Path]::GetRelativePath(
            $installationFull,
            [IO.Path]::GetFullPath($item.FullName)
        ).Replace('\', '/')
        if ($item.PSIsContainer) {
            if ($expectedDirectories.Contains($relative)) {
                continue
            }
            $directoryAllowed = $false
            foreach ($prefix in $prefixes) {
                $prefixRoot = $prefix.TrimEnd('/')
                if ($relative -ceq $prefixRoot -or
                    $relative.StartsWith(
                        $prefix,
                        [StringComparison]::Ordinal
                    ) -or
                    $prefix.StartsWith(
                        "$relative/",
                        [StringComparison]::Ordinal
                    )) {
                    $directoryAllowed = $true
                    break
                }
            }
            if (-not $directoryAllowed) {
                foreach ($extraFile in $extraFiles) {
                    if ($extraFile.StartsWith(
                        "$relative/",
                        [StringComparison]::Ordinal
                    )) {
                        $directoryAllowed = $true
                        break
                    }
                }
            }
            if (-not $directoryAllowed) {
                throw "$Description unexpected directory: $relative"
            }
            continue
        }
        if ($expectedFiles.ContainsKey($relative) -or
            $extraFiles.Contains($relative)) {
            continue
        }
        $fileAllowed = $false
        foreach ($prefix in $prefixes) {
            if ($relative.StartsWith(
                $prefix,
                [StringComparison]::Ordinal
            )) {
                $fileAllowed = $true
                break
            }
        }
        if (-not $fileAllowed) {
            throw "$Description unexpected file: $relative"
        }
    }
    return $payload
}

function Install-AuthenticatedDownload {
    param(
        [string]$Uri,
        [string]$Destination,
        [string]$ExpectedSha256,
        [string]$Description,
        [scriptblock]$DownloadAction
    )

    if (-not [Uri]::IsWellFormedUriString(
        $Uri,
        [UriKind]::Absolute
    )) {
        throw "$Description URL is invalid"
    }
    $destinationFull = Assert-PathChainNotReparse `
        -Path $Destination `
        -Description "$Description destination"
    $parent = Split-Path -Parent $destinationFull
    $null = Assert-DirectoryNotReparse `
        -Path $parent `
        -Description "$Description destination directory"
    if (Test-Path -LiteralPath $destinationFull) {
        $null = Assert-FileSha256 `
            -Path $destinationFull `
            -Expected $ExpectedSha256 `
            -Description $Description
        return $destinationFull
    }

    $temporary = Join-Path $parent (
        ([IO.Path]::GetFileName($destinationFull)) +
        '.download-' +
        [Guid]::NewGuid().ToString('N') +
        '.tmp'
    )
    $temporary = Assert-PathChainNotReparse `
        -Path $temporary `
        -Description "$Description private download"
    try {
        if ($null -eq $DownloadAction) {
            Invoke-WebRequest -Uri $Uri -OutFile $temporary
        }
        else {
            & $DownloadAction $Uri $temporary
        }
        $null = Assert-FileSha256 `
            -Path $temporary `
            -Expected $ExpectedSha256 `
            -Description "$Description downloaded bytes"
        $null = Assert-PathChainNotReparse `
            -Path $destinationFull `
            -Description "$Description destination"
        [IO.File]::Move($temporary, $destinationFull, $false)
        $null = Assert-FileSha256 `
            -Path $destinationFull `
            -Expected $ExpectedSha256 `
            -Description $Description
        return $destinationFull
    }
    finally {
        if (Test-Path -LiteralPath $temporary) {
            $null = Assert-PathChainNotReparse `
                -Path $temporary `
                -Description "$Description private download cleanup"
            $item = Get-Item -LiteralPath $temporary -Force
            if ($item.PSIsContainer) {
                throw (
                    "$Description private download cleanup target became " +
                    "a directory: $temporary"
                )
            }
            Remove-Item -LiteralPath $temporary -Force
        }
    }
}

function Invoke-TrustedBatchFile {
    param(
        [string]$CommandInterpreter,
        [string]$BatchFile,
        [string[]]$Arguments = @()
    )

    $expectedCmd = [IO.Path]::GetFullPath(
        (Join-Path ([Environment]::SystemDirectory) 'cmd.exe')
    )
    $command = Assert-PathChainNotReparse `
        -Path $CommandInterpreter `
        -Description 'trusted Windows command interpreter'
    if (-not [string]::Equals(
        $command,
        $expectedCmd,
        [StringComparison]::OrdinalIgnoreCase
    )) {
        throw (
            'Windows command interpreter must be the native System32 ' +
            "cmd.exe: $command"
        )
    }
    $null = Assert-FileSha256 `
        -Path $command `
        -Expected (
            Get-FileHash -LiteralPath $expectedCmd -Algorithm SHA256
        ).Hash `
        -Description 'trusted Windows command interpreter'
    $batch = Assert-PathChainNotReparse `
        -Path $BatchFile `
        -Description 'trusted batch launcher'
    if (-not (Test-Path -LiteralPath $batch -PathType Leaf) -or
        [IO.Path]::GetExtension($batch) -ine '.bat') {
        throw "Trusted batch launcher must be a .bat file: $batch"
    }
    $batchItem = Get-Item -LiteralPath $batch -Force
    if (($batchItem.Attributes -band
        [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Trusted batch launcher is a reparse point: $batch"
    }
    if ($batch.IndexOfAny([char[]]'"%!^&|<>()') -ge 0) {
        throw (
            'Trusted batch launcher path contains a forbidden cmd.exe ' +
            "metacharacter: $batch"
        )
    }

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $command
    $startInfo.UseShellExecute = $false
    $commandParts = [Collections.Generic.List[string]]::new()
    $commandParts.Add(('"{0}"' -f $batch))
    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        if ($Arguments[$index] -notmatch '^[A-Za-z0-9._:+/-]+$') {
            throw (
                'Trusted batch argument is outside the fixed safe grammar at ' +
                "index ${index}: '$($Arguments[$index])'"
            )
        }
        $commandParts.Add($Arguments[$index])
    }
    $startInfo.Arguments = (
        '/D /V:OFF /S /C "' +
        ($commandParts -join ' ') +
        '"'
    )
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw "Failed to start trusted batch launcher: $batch"
        }
        $process.WaitForExit()
        return $process.ExitCode
    }
    finally {
        $process.Dispose()
    }
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
        [AllowEmptyString()]
        [string]$ExpectedArchiveSha256 = '',
        [AllowEmptyString()]
        [string]$ExpectedArchiveSha512 = '',
        [AllowEmptyString()]
        [string]$StripPrefix = '',
        [AllowEmptyString()]
        [string]$ExtractTo = ''
    )

    $usesSha256 = -not [string]::IsNullOrEmpty(
        $ExpectedArchiveSha256
    )
    $usesSha512 = -not [string]::IsNullOrEmpty(
        $ExpectedArchiveSha512
    )
    if ($usesSha256 -eq $usesSha512) {
        throw (
            'Build-tool archive requires exactly one expected SHA-256 ' +
            'or SHA-512'
        )
    }
    if ($usesSha256 -and
        $ExpectedArchiveSha256 -notmatch '^[0-9a-fA-F]{64}$') {
        throw 'Build-tool archive expected SHA-256 is invalid'
    }
    if ($usesSha512 -and
        $ExpectedArchiveSha512 -notmatch '^[0-9a-fA-F]{128}$') {
        throw 'Build-tool archive expected SHA-512 is invalid'
    }
    $Archive = Assert-PathChainNotReparse `
        -Path $Archive `
        -Description 'Build-tool archive'
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
        $ExtractTo = Assert-PathChainNotReparse `
            -Path $ExtractTo `
            -Description 'Build-tool extraction root'
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
        $archiveHasher = if ($usesSha512) {
            [Security.Cryptography.SHA512]::Create()
        }
        else {
            [Security.Cryptography.SHA256]::Create()
        }
        $entryHasher = [Security.Cryptography.SHA256]::Create()
        try {
            $archiveHash = [Convert]::ToHexString(
                $archiveHasher.ComputeHash($stream)
            ).ToLowerInvariant()
            $expectedHash = if ($usesSha512) {
                $ExpectedArchiveSha512.ToLowerInvariant()
            }
            else {
                $ExpectedArchiveSha256.ToLowerInvariant()
            }
            if ($archiveHash -cne $expectedHash) {
                $algorithm = if ($usesSha512) { 'SHA-512' } else { 'SHA-256' }
                throw (
                    "Build-tool archive $algorithm drifted: expected " +
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
                            $entryHasher.ComputeHash($entryStream)
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
                        $null = Assert-DirectoryNotReparse `
                            -Path $directoryTarget `
                            -Description 'Build-tool extracted directory'
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
                        $null = Assert-PathChainNotReparse `
                            -Path $target `
                            -Description 'Build-tool extracted file'
                        $targetItem = Get-Item -LiteralPath $target -Force
                        if (($targetItem.Attributes -band
                            [IO.FileAttributes]::ReparsePoint) -ne 0 -or
                            $targetItem.PSIsContainer) {
                            throw (
                                'Build-tool extracted file is not a regular ' +
                                "file: $target"
                            )
                        }
                    }
                }
            }
            finally {
                $zip.Dispose()
            }
        }
        finally {
            $entryHasher.Dispose()
            $archiveHasher.Dispose()
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
        ArchiveHashAlgorithm = if ($usesSha512) { 'sha512' } else { 'sha256' }
        ArchiveHash = $archiveHash
        ArchiveSha256 = if ($usesSha256) { $archiveHash } else { $null }
        ArchiveSha512 = if ($usesSha512) { $archiveHash } else { $null }
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
    $archiveArguments = @{
        Archive = $Archive
        StripPrefix = [string]$payload.stripPrefix
    }
    if ($Artifact.PSObject.Properties.Name -ccontains 'sha512') {
        $archiveArguments.ExpectedArchiveSha512 = [string]$Artifact.sha512
    }
    else {
        $archiveArguments.ExpectedArchiveSha256 = [string]$Artifact.sha256
    }
    $archivePayload = Get-AuthenticatedZipPayload @archiveArguments
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
        ArchiveHashAlgorithm = $archivePayload.ArchiveHashAlgorithm
        ArchiveHash = $archivePayload.ArchiveHash
        ArchiveSha256 = $archivePayload.ArchiveSha256
        ArchiveSha512 = $archivePayload.ArchiveSha512
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

function Remove-EmscriptenVolatileCacheProducts {
    param(
        [string]$CacheRoot
    )

    if (-not (Test-Path -LiteralPath $CacheRoot -PathType Container)) {
        throw "Emscripten cache root is missing: $CacheRoot"
    }
    $cache = Assert-DirectoryNotReparse `
        -Path $CacheRoot `
        -Description 'Emscripten cache root'
    $sanity = Join-Path $cache 'sanity.txt'
    if (Test-Path -LiteralPath $sanity) {
        $item = Get-Item -LiteralPath $sanity -Force
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0 -or
            -not (Test-Path -LiteralPath $sanity -PathType Leaf)) {
            throw 'Emscripten cache sanity.txt must be a regular file'
        }
        Remove-Item -LiteralPath $sanity -Force
    }
    $symbols = Join-Path $cache 'symbol_lists'
    if (Test-Path -LiteralPath $symbols) {
        $symbolsItem = Get-Item -LiteralPath $symbols -Force
        if (($symbolsItem.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0 -or
            -not $symbolsItem.PSIsContainer) {
            throw 'Emscripten cache symbol_lists must be a regular directory'
        }
        foreach ($candidate in @(
            Get-ChildItem -LiteralPath $symbols -Recurse -Force
        )) {
            if (($candidate.Attributes -band
                [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw (
                    'Emscripten cache volatile reparse point is forbidden: ' +
                    $candidate.FullName
                )
            }
            if ($candidate.PSIsContainer) {
                throw (
                    'Nested Emscripten symbol_lists directory is forbidden: ' +
                    $candidate.FullName
                )
            }
            if ($candidate.Extension -cne '.json' -or
                -not (Test-Path -LiteralPath $candidate.FullName -PathType Leaf)) {
                throw (
                    'Unexpected Emscripten symbol_lists product: ' +
                    $candidate.FullName
                )
            }
        }
        Remove-Item -LiteralPath $symbols -Recurse -Force
    }
}

function Assert-EmscriptenCacheIdentity {
    param(
        [string]$CacheRoot,
        [PSCustomObject]$Contract,
        [string]$Python,
        [string]$Helper
    )

    if ($null -eq $Contract -or $null -eq $Contract.payload) {
        throw 'Emscripten cache contract is missing'
    }
    foreach ($inputFile in @(
        [PSCustomObject]@{
            Path = $Python
            Description = 'Pinned Emscripten cache identity Python'
        },
        [PSCustomObject]@{
            Path = $Helper
            Description = 'Tracked Emscripten cache identity helper'
        }
    )) {
        if (-not (Test-Path -LiteralPath $inputFile.Path -PathType Leaf)) {
            throw "$($inputFile.Description) is missing: $($inputFile.Path)"
        }
        $item = Get-Item -LiteralPath $inputFile.Path -Force
        if (($item.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$($inputFile.Description) reparse point is forbidden"
        }
    }
    $expected = $Contract.payload |
        ConvertTo-Json -Compress -Depth 10
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Python
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @(
        '-I',
        '-B',
        $Helper,
        '--cache-root',
        $CacheRoot,
        '--expected-json',
        $expected
    )) {
        $null = $startInfo.ArgumentList.Add($argument)
    }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw 'Failed to start Emscripten cache identity helper'
        }
        $stdout = $process.StandardOutput.ReadToEnd()
        $stderr = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw (
                'Emscripten frozen cache identity failed: ' +
                ($stderr + $stdout).Trim()
            )
        }
        try {
            return $stdout | ConvertFrom-Json
        }
        catch {
            throw (
                'Emscripten cache identity helper returned invalid JSON: ' +
                $stdout.Trim()
            )
        }
    }
    finally {
        $process.Dispose()
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
