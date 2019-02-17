$ProjFSPackageName = "GVFS.ProjFS"
$ProjFSPackageVersion = "2018.823.1"

$VFS_SCRIPTSDIR = $PSScriptRoot
$VFS_SRCDIR = (Get-Item -Path $VFS_SCRIPTSDIR\..).FullName
$VFS_ENLISTMENTDIR = (Get-Item -Path $VFS_SRCDIR\..).FullName
$VFS_OUTPUTDIR = "$VFS_ENLISTMENTDIR\BuildOutput"
$VFS_PACKAGESDIR = "$VFS_ENLISTMENTDIR\packages"
$VFS_TOOLSDIR = "$VFS_ENLISTMENTDIR\.tools"

Write-Host "-----------------------------------------------------"
Write-Host "VFS_SCRIPTSDIR:     $VFS_SCRIPTSDIR"
Write-Host "VFS_SRCDIR:         $VFS_SRCDIR"
Write-Host "VFS_ENLISTMENTDIR:  $VFS_ENLISTMENTDIR"
Write-Host "VFS_OUTPUTDIR:      $VFS_OUTPUTDIR"
Write-Host "VFS_PACKAGESDIR:    $VFS_PACKAGESDIR"
Write-Host "VFS_TOOLSDIR:       $VFS_TOOLSDIR"
Write-Host "-----------------------------------------------------"

$featureProjFS = Get-WindowsOptionalFeature -FeatureName Client-ProjFS -Online

if ($featureProjFS.State -eq "Enabled")
{
    Write-Host "ProjFS is enabled."
}
elseif ($featureProjFS.State -eq "Disabled")
{
    Write-Host "Enabling ProjFS."
    Enable-WindowsOptionalFeature -Online -FeatureName Client-ProjFS -NoRestart
}
else
{
    Write-Host "Installing ProjFS RS5 from NuGet"

    # Ensure that we have nuget.exe
    $nuget = "$VFS_TOOLSDIR\nuget.exe"

    if (!(Test-Path $nuget))
    {
        New-Item -ItemType Directory -Path "$nuget\.." -ErrorAction SilentlyContinue
        Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile $nuget
    }

    # Get the ProjFS RS5 package
    & $nuget install $ProjFSPackageName -Version $ProjFSPackageVersion

    $ProjFSPackageRoot = "$VFS_PACKAGESDIR\$ProjFSPackageName.$ProjFSPackageVersion"

    if (!(Test-Path $ProjFSPackageRoot))
    {
        Write-Error "Could not find $ProjFSPackageRoot!"
        return 1
    }

    # Install prjflt.sys.
    $RunDll32 = "$env:SystemRoot\System32\rundll32.exe"
    $RunDllArgs = "SETUPAPI.DLL,InstallHinfSection DefaultInstall 132"
    $RunDllInfPath = "$ProjFSPackageRoot\filter\PrjFlt.inf"

    $InstallCommand = "$RunDll32 $RunDllArgs $RunDllInfPath"

    Write-Host "Using INF to install PrjFlt.sys: $InstallCommand"
    Invoke-Expression $InstallCommand

    # Copy the user-mode library DLL into place.
    

    # The rundll32.exe command can take a second or two to take effect.  Poll for
    # the service to arrive.
    while ((Get-Service -Name "prjflt" -ErrorAction SilentlyContinue).Count -eq 0)
    {
        Start-Sleep -Seconds 1
    }

    # Load prjflt.sys.
    Start-Service -Name "prjflt"
}
