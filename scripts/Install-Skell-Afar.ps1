[CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='High')]
param(
    [string]$PackageRoot,
    [string]$RollbackFrom,
    [string]$Vst3Directory = (Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'),
    [string]$BackupRoot = (Join-Path $env:LOCALAPPDATA 'Skell Audio\Backups\Skell-Afar')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($PackageRoot) -eq [string]::IsNullOrWhiteSpace($RollbackFrom)) {
    throw 'Supply exactly one of -PackageRoot or -RollbackFrom.'
}
$Product='Skell-Afar'
$Vendor='Skell Audio'
$Cids=@('ABCDEF019182FAEB536B417541666172','ABCDEF011234ABCD536B417541666172')

function FullPath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'Empty path.' }
    [System.IO.Path]::GetFullPath($Path)
}
function TreeHash([string]$Directory) {
    $root=FullPath $Directory
    $files=@(Get-ChildItem -LiteralPath $root -Recurse -File -Force | Sort-Object FullName)
    if ($files.Count -eq 0) { throw "Empty bundle: $root" }
    $lines=@()
    foreach($f in $files) {
        if (($f.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse point rejected: $($f.FullName)" }
        $rel=$f.FullName.Substring($root.Length).TrimStart([char[]]@('\','/')).Replace('\','/')
        $h=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $lines+=($rel+'  '+$h)
    }
    $sha=[System.Security.Cryptography.SHA256]::Create()
    try { [System.BitConverter]::ToString($sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes([string]::Join([char]10,[string[]]$lines)))).Replace('-','').ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function CheckManifest([string]$Root,[string]$Name) {
    $rootPath=FullPath $Root
    $manifest=Join-Path $rootPath $Name
    if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) { throw "Missing $Name" }
    $prefix=$rootPath.TrimEnd([char[]]@('\','/'))+[System.IO.Path]::DirectorySeparatorChar
    $seen=@{}
    $paths=@()
    foreach($line in [System.IO.File]::ReadAllLines($manifest,[System.Text.Encoding]::ASCII)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^(?<hash>[0-9a-fA-F]{64})  (?<path>.+)$') { throw ('Malformed line in '+$Name+': '+$line) }
        $expected=$Matches.hash.ToLowerInvariant()
        $rel=$Matches.path
        if ([System.IO.Path]::IsPathRooted($rel) -or ($rel -split '[\\/]') -contains '..') { throw ('Unsafe path in '+$Name+': '+$rel) }
        $file=FullPath (Join-Path $rootPath ($rel.Replace('/','\')))
        if (-not $file.StartsWith($prefix,[System.StringComparison]::OrdinalIgnoreCase)) { throw "Path escapes package: $rel" }
        if ($seen.ContainsKey($rel)) { throw "Duplicate manifest entry: $rel" }
        $seen[$rel]=$true
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing checksummed file: $rel" }
        $actual=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actual -ne $expected) { throw "Checksum mismatch: $rel" }
        $paths+=$rel.Replace('\','/')
    }
    if ($paths.Count -eq 0) { throw "$Name is empty" }
    $paths
}
function BundleEvidence([string]$Bundle) {
    $b=FullPath $Bundle
    $infoPath=Join-Path $b 'Contents\Resources\moduleinfo.json'
    $module=Join-Path $b 'Contents\x86_64-win\Skell-Afar.vst3'
    if (-not (Test-Path -LiteralPath $infoPath -PathType Leaf)) { throw 'Missing moduleinfo.json' }
    if (-not (Test-Path -LiteralPath $module -PathType Leaf)) { throw 'Missing Windows VST3 module' }
    $modules=@(Get-ChildItem -LiteralPath (Join-Path $b 'Contents') -Recurse -File -Filter '*.vst3')
    if ($modules.Count -ne 1 -or $modules[0].FullName -ne (FullPath $module)) { throw 'Expected exactly one Windows VST3 module.' }
    $info=Get-Content -LiteralPath $infoPath -Raw | ConvertFrom-Json -ErrorAction Stop
    if ($null -eq $info -or $info -is [array] -or $info.Name -ne $Product -or $info.'Factory Info'.Vendor -ne $Vendor) { throw 'VST3 product/vendor metadata mismatch.' }
    if ($null -eq $info.Classes) { throw 'VST3 class list missing.' }
    $got=@($info.Classes | ForEach-Object { [string]$_.CID } | Sort-Object)
    $want=@($Cids | Sort-Object)
    if ([string]::Join('|',$got) -ne [string]::Join('|',$want)) { throw 'VST3 class IDs changed.' }
    $version=[string]$info.Version
    if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Invalid VST3 version.' }
    $bytes=[System.IO.File]::ReadAllBytes($module)
    if ($bytes.Length -lt 128 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) { throw 'Missing MZ header.' }
    $off=[System.BitConverter]::ToInt32($bytes,0x3C)
    if ($off -lt 64 -or $off+26 -gt $bytes.Length -or $bytes[$off] -ne 0x50 -or $bytes[$off+1] -ne 0x45 -or $bytes[$off+2] -ne 0 -or $bytes[$off+3] -ne 0) { throw 'Invalid PE header.' }
    if ([System.BitConverter]::ToUInt16($bytes,$off+4) -ne 0x8664 -or [System.BitConverter]::ToUInt16($bytes,$off+24) -ne 0x20B) { throw 'Module is not AMD64 PE32+.' }
    [pscustomobject]@{
        version=$version
        binary_sha256=(Get-FileHash -LiteralPath $module -Algorithm SHA256).Hash.ToLowerInvariant()
        tree_sha256=(TreeHash $b)
    }
}
function VerifiedPackage([string]$Root) {
    $r=FullPath $Root
    $build=Get-Content -LiteralPath (Join-Path $r 'build-info.json') -Raw | ConvertFrom-Json -ErrorAction Stop
    if ($build.product -ne 'Skell Afar' -or $build.format -ne 'VST3' -or $build.platform -ne 'Windows x86_64' -or $build.release_status -ne 'preview-only; not a commercial release') { throw 'Not a Skell Afar Windows private-preview package.' }
    $main=@(CheckManifest $r 'SHA256SUMS.txt')
    $tools=@(CheckManifest $r 'TOOLS-SHA256SUMS.txt')
    foreach($p in @('build-info.json','vst3-verification.json','PREVIEW-ONLY.txt','RELEASE-NOTES.txt','Skell-Afar.vst3/Contents/Resources/moduleinfo.json','Skell-Afar.vst3/Contents/x86_64-win/Skell-Afar.vst3')) {
        if ($main -notcontains $p) { throw "Main manifest omits $p" }
    }
    foreach($p in @('Install-Skell-Afar.ps1','INSTALL-WINDOWS.txt','THIRD-PARTY-NOTICES.txt')) {
        if ($tools -notcontains $p) { throw "Tool manifest omits $p" }
    }
    $bundle=Join-Path $r 'Skell-Afar.vst3'
    $e=BundleEvidence $bundle
    if ($e.version -ne [string]$build.version) { throw 'Build and module versions differ.' }
    foreach($f in Get-ChildItem -LiteralPath $bundle -Recurse -File -Force) {
        $rel='Skell-Afar.vst3/'+$f.FullName.Substring($bundle.Length).TrimStart([char[]]@('\','/')).Replace('\','/')
        if ($main -notcontains $rel) { throw "Main manifest omits bundle file $rel" }
    }
    $v=Get-Content -LiteralPath (Join-Path $r 'vst3-verification.json') -Raw | ConvertFrom-Json -ErrorAction Stop
    if ($v.binary_sha256 -ne $e.binary_sha256 -or $v.version -ne $e.version) { throw 'VST3 verification manifest mismatch.' }
    if ((Get-Content -LiteralPath (Join-Path $r 'PREVIEW-ONLY.txt') -Raw) -notmatch 'preview-only') { throw 'Preview warning missing.' }
    [pscustomobject]@{ root=$r; bundle=$bundle; version=$e.version; evidence=$e }
}
function NewBackup([string]$Bundle,[string]$Reason,[string]$Target) {
    $e=BundleEvidence $Bundle
    $root=FullPath $BackupRoot
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    $stamp=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $id=[Guid]::NewGuid().ToString('N').Substring(0,8)
    $path=Join-Path $root ("Skell-Afar-"+$e.version+"-"+$stamp+"-"+$id)
    New-Item -ItemType Directory -Path $path | Out-Null
    $copy=Join-Path $path 'Skell-Afar.vst3'
    Copy-Item -LiteralPath $Bundle -Destination $copy -Recurse
    $c=BundleEvidence $copy
    if ($c.tree_sha256 -ne $e.tree_sha256) { throw "Backup verification failed; original remains untouched: $path" }
    [ordered]@{schema_version=1;product='Skell Afar';created_utc=[DateTime]::UtcNow.ToString('o');version=$e.version;binary_sha256=$e.binary_sha256;bundle_tree_sha256=$e.tree_sha256;reason=$Reason;original_target=$Target} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $path 'backup-manifest.json') -Encoding UTF8
    $path
}
function ReplaceBundle([string]$Source,[string]$Target,[string]$TargetRoot,[string]$Reason) {
    $expected=BundleEvidence $Source
    $backup=$null
    if (Test-Path -LiteralPath $Target) {
        if (-not (Test-Path -LiteralPath $Target -PathType Container)) { throw "Existing target is not a bundle: $Target" }
        $current=BundleEvidence $Target
        if ($current.tree_sha256 -eq $expected.tree_sha256) {
            return [pscustomobject]@{status='already-installed';version=$current.version;target=$Target;backup_path=$null;binary_sha256=$current.binary_sha256}
        }
        $backup=NewBackup $Target $Reason $Target
    }
    New-Item -ItemType Directory -Path $TargetRoot -Force | Out-Null
    $id=[Guid]::NewGuid().ToString('N')
    $stage=Join-Path $TargetRoot ('.Skell-Afar-stage-'+$id+'.vst3')
    $old=Join-Path $TargetRoot ('.Skell-Afar-previous-'+$id+'.vst3')
    $oldMoved=$false
    $newMoved=$false
    try {
        Copy-Item -LiteralPath $Source -Destination $stage -Recurse
        $staged=BundleEvidence $stage
        if ($staged.tree_sha256 -ne $expected.tree_sha256) { throw 'Staged bundle hash differs from source.' }
        if (Test-Path -LiteralPath $Target) {
            Move-Item -LiteralPath $Target -Destination $old
            $oldMoved=$true
        }
        Move-Item -LiteralPath $stage -Destination $Target
        $newMoved=$true
        $installed=BundleEvidence $Target
        if ($installed.tree_sha256 -ne $expected.tree_sha256) { throw 'Installed bundle failed post-copy verification.' }
        if (Test-Path -LiteralPath $old) { Remove-Item -LiteralPath $old -Recurse -Force }
    } catch {
        if ($newMoved -and (Test-Path -LiteralPath $Target)) {
            Move-Item -LiteralPath $Target -Destination (Join-Path $TargetRoot ('.Skell-Afar-failed-'+$id)) -ErrorAction SilentlyContinue
        }
        if ($oldMoved -and (Test-Path -LiteralPath $old) -and -not (Test-Path -LiteralPath $Target)) {
            Move-Item -LiteralPath $old -Destination $Target -ErrorAction SilentlyContinue
        }
        throw
    } finally {
        if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue }
    }
    [pscustomobject]@{status='installed';version=$installed.version;target=$Target;backup_path=$backup;binary_sha256=$installed.binary_sha256}
}
function InvokeInstall([bool]$ShouldProceed) {
    $p=VerifiedPackage $PackageRoot
    $root=FullPath $Vst3Directory
    $target=Join-Path $root 'Skell-Afar.vst3'
    if (Test-Path -LiteralPath $target -PathType Container) {
        $current=BundleEvidence $target
        if ($current.tree_sha256 -eq $p.evidence.tree_sha256) {
            return [pscustomobject]@{status='already-installed';version=$current.version;target=$target;backup_path=$null;binary_sha256=$current.binary_sha256}
        }
    }
    if (-not $ShouldProceed) {
        return [pscustomobject]@{status='what-if';version=$p.version;target=$target;backup_path=$null;binary_sha256=$p.evidence.binary_sha256}
    }
    ReplaceBundle $p.bundle $target $root 'pre-install-or-update'
}
function InvokeRollback([bool]$ShouldProceed) {
    $path=FullPath $RollbackFrom
    $manifest=Get-Content -LiteralPath (Join-Path $path 'backup-manifest.json') -Raw | ConvertFrom-Json -ErrorAction Stop
    if ($manifest.schema_version -ne 1 -or $manifest.product -ne 'Skell Afar') { throw 'Unsupported backup manifest.' }
    $bundle=Join-Path $path 'Skell-Afar.vst3'
    $e=BundleEvidence $bundle
    if ($e.version -ne [string]$manifest.version -or $e.binary_sha256 -ne [string]$manifest.binary_sha256 -or $e.tree_sha256 -ne [string]$manifest.bundle_tree_sha256) { throw 'Backup integrity verification failed.' }
    $root=FullPath $Vst3Directory
    $target=Join-Path $root 'Skell-Afar.vst3'
    if (Test-Path -LiteralPath $target -PathType Container) {
        $current=BundleEvidence $target
        if ($current.tree_sha256 -eq $e.tree_sha256) {
            return [pscustomobject]@{status='already-restored';version=$current.version;target=$target;backup_path=$null;binary_sha256=$current.binary_sha256}
        }
    }
    if (-not $ShouldProceed) {
        return [pscustomobject]@{status='what-if';version=$e.version;target=$target;backup_path=$null;binary_sha256=$e.binary_sha256}
    }
    $result=ReplaceBundle $bundle $target $root 'pre-rollback'
    [pscustomobject]@{status='rolled-back';version=$result.version;target=$result.target;backup_path=$result.backup_path;binary_sha256=$result.binary_sha256}
}
if (-not [string]::IsNullOrWhiteSpace($PackageRoot)) {
    $preview=VerifiedPackage $PackageRoot
    $root=FullPath $Vst3Directory
    $target=Join-Path $root 'Skell-Afar.vst3'
    if (Test-Path -LiteralPath $target -PathType Container) {
        $current=BundleEvidence $target
        if ($current.tree_sha256 -eq $preview.evidence.tree_sha256) {
            return [pscustomobject]@{status='already-installed';version=$current.version;target=$target;backup_path=$null;binary_sha256=$current.binary_sha256}
        }
    }
    # ShouldProcess must run in the advanced script scope. Passing $PSCmdlet
    # into a helper function leaves its command runtime detached on PowerShell 7.
    $proceed=$PSCmdlet.ShouldProcess($target,"Install Skell Afar $($preview.version) with verified backup")
    InvokeInstall ([bool]$proceed)
} else {
    $backupPath=FullPath $RollbackFrom
    $backupManifest=Get-Content -LiteralPath (Join-Path $backupPath 'backup-manifest.json') -Raw | ConvertFrom-Json -ErrorAction Stop
    $target=Join-Path (FullPath $Vst3Directory) 'Skell-Afar.vst3'
    if (Test-Path -LiteralPath $target -PathType Container) {
        $current=BundleEvidence $target
        $backupBundle=Join-Path $backupPath 'Skell-Afar.vst3'
        $backupEvidence=BundleEvidence $backupBundle
        if ($current.tree_sha256 -eq $backupEvidence.tree_sha256) {
            return [pscustomobject]@{status='already-restored';version=$current.version;target=$target;backup_path=$null;binary_sha256=$current.binary_sha256}
        }
    }
    $proceed=$PSCmdlet.ShouldProcess($target,"Restore Skell Afar $($backupManifest.version) from verified backup")
    InvokeRollback ([bool]$proceed)
}
