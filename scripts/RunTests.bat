@ECHO OFF
CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")

set RESULT=0

%PROJFS_OUTPUTDIR%\ProjectedFSLib.Managed.Test\bin\x64\%Configuration%\net461\ProjectedFSLib.Managed.Test.exe --params ProviderExe=%PROJFS_OUTPUTDIR%\SimpleProviderManaged\bin\x64\%Configuration%\SimpleProviderManaged.exe  || set RESULT=1

exit /b %RESULT%