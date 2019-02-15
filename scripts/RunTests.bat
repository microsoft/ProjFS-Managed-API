@ECHO OFF
CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")

set RESULT=0

%VFS_OUTPUTDIR%\ProjectedFSLib.Managed.Test\bin\x64\%Configuration%\net461\ProjectedFSLib.Managed.Test.exe --params ProviderExe=%VFS_OUTPUTDIR%\SimpleProviderManaged\bin\x64\%Configuration%\SimpleProviderManaged.exe  || set RESULT=1

exit /b %RESULT%