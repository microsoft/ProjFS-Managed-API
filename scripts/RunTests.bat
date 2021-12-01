@ECHO OFF
CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")

set RESULT_FRAMEWORK=0
set TESTDIR=%PROJFS_OUTPUTDIR%\ProjectedFSLib.Managed.Test\bin\AnyCPU\%Configuration%\net48
pushd %TESTDIR%
%TESTDIR%\ProjectedFSLib.Managed.Test.exe --params ProviderExe=%PROJFS_OUTPUTDIR%\SimpleProviderManaged\bin\AnyCPU\%Configuration%\net48\SimpleProviderManaged.exe  || set RESULT_FRAMEWORK=1
popd

set RESULT_CORE=0
set TESTDIR=%PROJFS_OUTPUTDIR%\ProjectedFSLib.Managed.Test\bin\AnyCPU\%Configuration%\netcoreapp3.1
pushd %TESTDIR%
%TESTDIR%\ProjectedFSLib.Managed.Test.exe --params ProviderExe=%PROJFS_OUTPUTDIR%\SimpleProviderManaged\bin\AnyCPU\%Configuration%\netcoreapp3.1\SimpleProviderManaged.exe  || set RESULT_CORE=1
popd

set RESULT=0
if "%RESULT_FRAMEWORK% %RESULT_CORE%" neq "0 0" set RESULT=1

exit /b %RESULT%