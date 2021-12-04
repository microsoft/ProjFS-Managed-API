@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")
IF "%2"=="" (SET "ProjFSManagedVersion=0.2.173.2") ELSE (SET "ProjFSManagedVersion=%2")

SET SolutionConfiguration=%Configuration%

:: Make the build version available in the DevOps environment.
@echo ##vso[task.setvariable variable=PROJFS_MANAGED_VERSION]%ProjFSManagedVersion%

SET nuget="%PROJFS_TOOLSDIR%\nuget.exe"
IF NOT EXIST %nuget% (
  mkdir %nuget%\..
  powershell -ExecutionPolicy Bypass -Command "Invoke-WebRequest 'https://dist.nuget.org/win-x86-commandline/latest/nuget.exe' -OutFile %nuget%"
)

:: Use vswhere to find the latest VS installation (including prerelease installations) with the msbuild component.
:: See https://github.com/Microsoft/vswhere/wiki/Find-MSBuild
SET vswherever=2.8.4
%nuget% install vswhere -Version %vswherever% -OutputDirectory %PROJFS_PACKAGESDIR% || exit /b 1
SET vswhere=%PROJFS_PACKAGESDIR%\vswhere.%vswherever%\tools\vswhere.exe
set WINSDK_BUILD=19041
echo Checking for VS installation:
echo %vswhere% -all -prerelease -latest -version "[16.4,18.0)" -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Workload.ManagedDesktop Microsoft.VisualStudio.Workload.NativeDesktop Microsoft.VisualStudio.Component.Windows10SDK.%WINSDK_BUILD% Microsoft.VisualStudio.Component.VC.CLI.Support -property installationPath
for /f "usebackq tokens=*" %%i in (`%vswhere% -all -prerelease -latest -version "[16.4,18.0)" -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Workload.ManagedDesktop Microsoft.VisualStudio.Workload.NativeDesktop Microsoft.VisualStudio.Component.Windows10SDK.%WINSDK_BUILD% Microsoft.VisualStudio.Component.VC.CLI.Support -property installationPath`) do (
  set VsDir=%%i
)

IF NOT DEFINED VsDir (
  echo All installed Visual Studio instances:
  %vswhere% -all -prerelease -products * -format json
  echo ERROR: Could not locate a Visual Studio installation with required components.
  echo Refer to Readme.md for a list of the required Visual Studio components.
  exit /b 10
)

echo Setting up the VS Developer Command Prompt environment variables from %VsDir%
call "%VsDir%\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
@rem pushd "%VsDir%"
@rem call "%VsDir%\Common7\Tools\VsDevCmd.bat"
@rem popd
 
:: Restore all dependencies and run the build.
pushd "%PROJFS_SRCDIR%"
msbuild /t:Restore ProjectedFSLib.Managed.sln
msbuild ProjectedFSLib.Managed.sln /p:ProjFSManagedVersion=%ProjFSManagedVersion% /p:Configuration=%SolutionConfiguration% /p:Platform=x64 || exit /b 1
popd

ENDLOCAL
