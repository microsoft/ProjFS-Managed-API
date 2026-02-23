@ECHO OFF
CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF EXIST %PROJFS_OUTPUTDIR% (
    ECHO deleting artifacts
    rmdir /s /q %PROJFS_OUTPUTDIR%
) ELSE (
    ECHO no artifacts found
)

:: Clean per-project bin/obj directories
FOR %%D IN (
    "%PROJFS_ENLISTMENTDIR%\ProjectedFSLib.Managed\bin"
    "%PROJFS_ENLISTMENTDIR%\ProjectedFSLib.Managed\obj"
    "%PROJFS_ENLISTMENTDIR%\ProjectedFSLib.Managed.Test\bin"
    "%PROJFS_ENLISTMENTDIR%\ProjectedFSLib.Managed.Test\obj"
    "%PROJFS_ENLISTMENTDIR%\simpleProviderManaged\bin"
    "%PROJFS_ENLISTMENTDIR%\simpleProviderManaged\obj"
) DO (
    IF EXIST %%D (
        ECHO deleting %%D
        rmdir /s /q %%D
    )
)
