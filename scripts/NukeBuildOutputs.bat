@ECHO OFF
CALL %~dp0\InitializeEnvironment.bat || EXIT /b 10

IF EXIST %PROJFS_OUTPUTDIR% (
    ECHO deleting build outputs
    rmdir /s /q %PROJFS_OUTPUTDIR%
) ELSE (
    ECHO no build outputs found
)

IF EXIST %PROJFS_PUBLISHDIR% (
    ECHO deleting published output
    rmdir /s /q %PROJFS_PUBLISHDIR%
) ELSE (
    ECHO no published output found
)

IF EXIST %PROJFS_PACKAGESDIR% (
    ECHO deleting packages
    rmdir /s /q %PROJFS_PACKAGESDIR%
) ELSE (
    ECHO no packages found
)
