<#
.SYNOPSIS
    Builds the ProjFS Managed API and creates NuGet packages.

.DESCRIPTION
    Builds the ProjectedFSLib.Managed project in Release configuration
    and produces .nupkg and .snupkg packages in the artifacts/ directory.

.PARAMETER Configuration
    Build configuration (default: Release).

.PARAMETER OutputDirectory
    Directory for NuGet packages (default: artifacts/packages).

.EXAMPLE
    .\Pack-NuGet.ps1
    .\Pack-NuGet.ps1 -Configuration Debug
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\artifacts\packages')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $repoRoot 'ProjectedFSLib.Managed' 'ProjectedFSLib.Managed.csproj'

if (-not (Test-Path $project)) {
    Write-Error "Project not found: $project"
    exit 1
}

# Ensure output directory exists
if (-not (Test-Path $OutputDirectory)) {
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
}

Write-Host "Building and packing $Configuration..." -ForegroundColor Cyan

dotnet pack $project `
    -c $Configuration `
    -o $OutputDirectory `
    --include-symbols `
    -p:SymbolPackageFormat=snupkg

if ($LASTEXITCODE -ne 0) {
    Write-Error "dotnet pack failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

$packages = Get-ChildItem $OutputDirectory -Filter '*.nupkg' | Sort-Object LastWriteTime -Descending
Write-Host "`nPackages created:" -ForegroundColor Green
foreach ($pkg in $packages) {
    Write-Host "  $($pkg.Name)" -ForegroundColor Green
}
