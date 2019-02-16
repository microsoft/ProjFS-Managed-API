$ProjFSPackageName = "GVFS.ProjFS"
$ProjFSPackageVersion = "2018.823.1"

$VFS_SCRIPTSDIR = (Get-Location).Path
$VFS_SRCDIR = (Get-Item -Path $VFS_SCRIPTSDIR\..).FullName
$VFS_ENLISTMENTDIR = (Get-Item -Path $VFS_SRCDIR\..).FullName
$VFS_OUTPUTDIR = "$VFS_ENLISTMENTDIR\BuildOutput"
$VFS_PACKAGESDIR = "$VFS_ENLISTMENTDIR\packages"
$VFS_PUBLISHDIR = "$VFS_ENLISTMENTDIR\Publish"
$VFS_TOOLSDIR = "$VFS_ENLISTMENTDIR\.tools"

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

    $nuget = "$VFS_TOOLSDIR\nuget.exe"

    if (!(Test-Path $nuget))
    {
        New-Item -ItemType Directory -Path "$nuget\.." -ErrorAction SilentlyContinue
        Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile $nuget
    }

    & $nuget install $ProjFSPackageName -Version $ProjFSPackageVersion

    $ProjFSPackageRoot = "$VFS_PACKAGESDIR\$ProjFSPackageName.$ProjFSPackageVersion"

    if (!(Test-Path $ProjFSPackageRoot))
    {
        Write-Error "Failed to fetch $ProjFSPackageName.$ProjFSPackageVersion nupkg!"
        return 1
    }

    $RunDll32 = "$env:SystemRoot\System32\rundll32.exe"
    $RunDllArgs = "SETUPAPI.DLL,InstallHinfSection DefaultInstall 132"
    $RunDllInfPath = "$ProjFSPackageRoot\filter\PrjFlt.inf"

	$InstallCommand = "$RunDll32 $RunDllArgs $RunDllInfPath"

	Write-Host "Using INF to install PrjFlt.sys: $InstallCommand"
    iex $InstallCommand

	# The above takes a moment to take effect.  Poll for the service to arrive.
	while ((Get-Service -Name "prjflt" -ErrorAction SilentlyContinue).Count -eq 0)
	{
		Start-Sleep -Seconds 1
	}

	Start-Service -Name "prjflt"
}
