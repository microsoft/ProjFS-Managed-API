@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")
IF "%2"=="" (SET "ProjFSManagedVersion=0.3.0.0") ELSE (SET "ProjFSManagedVersion=%2")

SET SolutionConfiguration=%Configuration%

:: Make the build version available in the DevOps environment.
@echo ##vso[task.setvariable variable=PROJFS_MANAGED_VERSION]%ProjFSManagedVersion%

SET nuget="%PROJFS_TOOLSDIR%\nuget.exe"
IF NOT EXIST %nuget% (
  mkdir %nuget%\..
  powershell -ExecutionPolicy Bypass -Command "Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile %nuget%"
)

:: Acquire vswhere to find dev15/dev16 installations reliably.
SET vswherever=2.8.4
%nuget% install vswhere -Version %vswherever% || exit /b 1
SET vswhere=%PROJFS_PACKAGESDIR%\vswhere.%vswherever%\tools\vswhere.exe

:: Use vswhere to find the latest VS installation (including prerelease installations) with the msbuild component.
:: See https://github.com/Microsoft/vswhere/wiki/Find-MSBuild
set WINSDK_BUILD=18362
for /f "usebackq tokens=*" %%i in (`%vswhere% -all -prerelease -latest -version "[16.4,17.0)" -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Workload.ManagedDesktop Microsoft.VisualStudio.Workload.NativeDesktop Microsoft.VisualStudio.Workload.NetCoreTools Microsoft.VisualStudio.Component.Windows10SDK.%WINSDK_BUILD% Microsoft.VisualStudio.Component.VC.CLI.Support -property installationPath`) do (
  set VsInstallDir=%%i
)

IF NOT DEFINED VsInstallDir (
  echo ERROR: Could not locate a Visual Studio installation with required components.
  echo Refer to Readme.md for a list of the required Visual Studio components.
  exit /b 10
)

SET msbuild="%VsInstallDir%\MSBuild\15.0\Bin\amd64\msbuild.exe"
SET PlatformToolset="v141"
IF NOT EXIST %msbuild% (
  :: dev16 has msbuild.exe at a different location
  SET msbuild="%VsInstallDir%\MSBuild\Current\Bin\amd64\msbuild.exe"
  SET PlatformToolset="v142"
  IF NOT EXIST !msbuild! (
    echo ERROR: Could not find msbuild
    exit /b 1
  )
)

:: Restore all dependencies.
%nuget% restore %PROJFS_SRCDIR%\ProjectedFSLib.Managed.sln
dotnet restore %PROJFS_SRCDIR%\ProjectedFSLib.Managed.sln /p:Configuration=%SolutionConfiguration% /p:VCTargetsPath="C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\MSBuild\Microsoft\VC\v160\Platforms\x64\PlatformToolsets\!PlatformToolset!" --packages %PROJFS_PACKAGESDIR% || exit /b 1

:: Kick off the build.
!msbuild! %PROJFS_SRCDIR%\ProjectedFSLib.Managed.sln /p:ProjFSManagedVersion=%ProjFSManagedVersion% /p:Configuration=%SolutionConfiguration% /p:Platform=x64 /p:PlatformToolset=!PlatformToolset! || exit /b 1

ENDLOCAL
