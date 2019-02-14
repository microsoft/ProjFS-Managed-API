@ECHO OFF
CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF EXIST %VFS_OUTPUTDIR% (
    ECHO deleting build outputs
    rmdir /s /q %VFS_OUTPUTDIR%
) ELSE (
    ECHO no build outputs found
)

IF EXIST %VFS_PUBLISHDIR% (
    ECHO deleting published output
    rmdir /s /q %VFS_PUBLISHDIR%
) ELSE (
    ECHO no packages found
)

IF EXIST %VFS_PACKAGESDIR% (
    ECHO deleting packages
    rmdir /s /q %VFS_PACKAGESDIR%
) ELSE (
    ECHO no packages found
)
