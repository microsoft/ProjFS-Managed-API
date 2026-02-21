@ECHO OFF
SETLOCAL

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")

ECHO Running ProjFS Managed API tests (%Configuration%)...
dotnet test "%~dp0\..\ProjectedFSLib.Managed.sln" -c %Configuration% --no-build
IF %ERRORLEVEL% NEQ 0 (
    ECHO Tests failed with error %ERRORLEVEL%
    EXIT /b %ERRORLEVEL%
)

ECHO All tests passed.