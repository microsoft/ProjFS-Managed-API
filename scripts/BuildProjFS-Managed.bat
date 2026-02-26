@ECHO OFF
SETLOCAL

IF "%1"=="" (SET "Configuration=Debug") ELSE (SET "Configuration=%1")

ECHO Building ProjFS Managed API (%Configuration%)...
dotnet build "%~dp0\..\ProjectedFSLib.Managed.slnx" -c %Configuration%
IF %ERRORLEVEL% NEQ 0 (
    ECHO Build failed with error %ERRORLEVEL%
    EXIT /b %ERRORLEVEL%
)

ECHO Build succeeded.

ENDLOCAL
