[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$PackageRoot,[string]$TestRoot=(Join-Path ([System.IO.Path]::GetTempPath()) ('Skell-Afar-Test-'+[Guid]::NewGuid().ToString('N'))))
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
function Assert([bool]$Ok,[string]$Message) { if (-not $Ok) { throw $Message } }
function CopyTree([string]$Source,[string]$Destination) {
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    foreach($item in Get-ChildItem -LiteralPath $Source -Force) { Copy-Item -LiteralPath $item.FullName -Destination $Destination -Recurse -Force }
}
function RehashMain([string]$Root) {
    $r=[System.IO.Path]::GetFullPath($Root)
    $lines=@()
    foreach($f in Get-ChildItem -LiteralPath $r -Recurse -File -Force | Where-Object {
        $_.FullName -ne (Join-Path $r 'SHA256SUMS.txt') -and
        $_.FullName -ne (Join-Path $r 'TOOLS-SHA256SUMS.txt') -and
        $_.FullName -ne (Join-Path $r 'Install-Skell-Afar.ps1') -and
        $_.FullName -ne (Join-Path $r 'INSTALL-WINDOWS.txt') -and
        $_.FullName -ne (Join-Path $r 'THIRD-PARTY-NOTICES.txt')
    } | Sort-Object FullName) {
        $rel=$f.FullName.Substring($r.Length).TrimStart([char[]]@('\','/')).Replace('\','/')
        $hash=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $lines+=($hash+'  '+$rel)
    }
    [System.IO.File]::WriteAllLines((Join-Path $r 'SHA256SUMS.txt'),[string[]]$lines,[System.Text.Encoding]::ASCII)
}
function Throws([scriptblock]$Action,[string]$Message) {
    $didThrow=$false
    try { & $Action | Out-Null } catch { $didThrow=$true }
    if (-not $didThrow) { throw $Message }
}
$root=[System.IO.Path]::GetFullPath($PackageRoot)
$installer=Join-Path $root 'Install-Skell-Afar.ps1'
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) { throw 'Package installer is missing.' }
New-Item -ItemType Directory -Path $TestRoot -Force | Out-Null
try {
    $whatIfTarget=Join-Path $TestRoot 'WhatIf\VST3'
    $whatIfBackup=Join-Path $TestRoot 'WhatIf\Backups'
    $dry=& $installer -PackageRoot $root -Vst3Directory $whatIfTarget -BackupRoot $whatIfBackup -WhatIf
    Assert ($dry.status -eq 'what-if') 'WhatIf did not return a dry-run status.'
    Assert (-not (Test-Path -LiteralPath $whatIfTarget)) 'WhatIf created the install directory.'
    Assert (-not (Test-Path -LiteralPath $whatIfBackup)) 'WhatIf created the backup directory.'

    $target=Join-Path $TestRoot 'Installed\VST3'
    $backups=Join-Path $TestRoot 'Installed\Backups'
    $fresh=& $installer -PackageRoot $root -Vst3Directory $target -BackupRoot $backups
    Assert ($fresh.status -eq 'installed' -and $fresh.version -eq '0.6.0') 'Fresh install failed.'
    Assert ($null -eq $fresh.backup_path) 'Fresh install unexpectedly made a backup.'
    Assert (-not (Test-Path -LiteralPath $backups)) 'Fresh install created an unnecessary backup directory.'
    Assert (Test-Path -LiteralPath (Join-Path $target 'Skell-Afar.vst3\Contents\x86_64-win\Skell-Afar.vst3')) 'Installed VST3 module is missing.'

    $same=& $installer -PackageRoot $root -Vst3Directory $target -BackupRoot $backups
    Assert ($same.status -eq 'already-installed') 'Repeat install was not idempotent.'
    Assert ($null -eq $same.backup_path) 'Idempotent install unexpectedly returned a backup.'

    $upgrade=Join-Path $TestRoot 'UpgradePackage'
    CopyTree $root $upgrade
    $moduleInfo=Join-Path $upgrade 'Skell-Afar.vst3\Contents\Resources\moduleinfo.json'
    $info=Get-Content -LiteralPath $moduleInfo -Raw | ConvertFrom-Json
    $info.Version='0.6.1'
    foreach($c in $info.Classes) { if ($c.PSObject.Properties['Version']) { $c.Version='0.6.1' } }
    $info | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $moduleInfo -Encoding UTF8
    $build=Join-Path $upgrade 'build-info.json'
    $b=Get-Content -LiteralPath $build -Raw | ConvertFrom-Json
    $b.version='0.6.1'
    $b | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $build -Encoding UTF8
    $verification=Join-Path $upgrade 'vst3-verification.json'
    $v=Get-Content -LiteralPath $verification -Raw | ConvertFrom-Json
    $v.version='0.6.1'
    $v | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $verification -Encoding UTF8
    RehashMain $upgrade
    $updated=& (Join-Path $upgrade 'Install-Skell-Afar.ps1') -PackageRoot $upgrade -Vst3Directory $target -BackupRoot $backups
    Assert ($updated.status -eq 'installed' -and $updated.version -eq '0.6.1') 'Upgrade failed.'
    Assert (-not [string]::IsNullOrWhiteSpace([string]$updated.backup_path)) 'Upgrade did not preserve a backup.'
    Assert (Test-Path -LiteralPath (Join-Path $updated.backup_path 'backup-manifest.json')) 'Backup manifest missing.'
    Assert (Test-Path -LiteralPath (Join-Path $updated.backup_path 'Skell-Afar.vst3')) 'Verified backup bundle missing.'

    $rollback=& $installer -RollbackFrom $updated.backup_path -Vst3Directory $target -BackupRoot $backups
    Assert ($rollback.status -eq 'rolled-back' -and $rollback.version -eq '0.6.0') 'Rollback failed to restore the original version.'
    Assert (-not [string]::IsNullOrWhiteSpace([string]$rollback.backup_path)) 'Rollback did not preserve the replaced version.'

    $tampered=Join-Path $TestRoot 'TamperedPackage'
    CopyTree $root $tampered
    Add-Content -LiteralPath (Join-Path $tampered 'PREVIEW-ONLY.txt') -Value 'tampered'
    $tamperedTarget=Join-Path $TestRoot 'Tampered\VST3'
    Throws { & (Join-Path $tampered 'Install-Skell-Afar.ps1') -PackageRoot $tampered -Vst3Directory $tamperedTarget -BackupRoot (Join-Path $TestRoot 'Tampered\Backups') } 'Tampered package was accepted.'
    Assert (-not (Test-Path -LiteralPath $tamperedTarget)) 'Tampered package changed the install destination.'
    Write-Host 'Skell Afar install/update/rollback tests: 16 assertions passed.'
}
finally {
    if (Test-Path -LiteralPath $TestRoot) { Remove-Item -LiteralPath $TestRoot -Recurse -Force -ErrorAction SilentlyContinue }
}
