@ECHO OFF

:: Set environment variables for interesting paths that scripts might need access to.
PUSHD %~dp0
SET PROJFS_SCRIPTSDIR=%CD%
POPD

CALL :RESOLVEPATH "%PROJFS_SCRIPTSDIR%\.."
SET PROJFS_ENLISTMENTDIR=%_PARSED_PATH_%

SET PROJFS_OUTPUTDIR=%PROJFS_ENLISTMENTDIR%\artifacts
SET PROJFS_PACKAGESDIR=%PROJFS_ENLISTMENTDIR%\artifacts\packages

:: Make the path variables available in the DevOps environment.
@echo ##vso[task.setvariable variable=PROJFS_ENLISTMENTDIR]%PROJFS_ENLISTMENTDIR%
@echo ##vso[task.setvariable variable=PROJFS_OUTPUTDIR]%PROJFS_OUTPUTDIR%
@echo ##vso[task.setvariable variable=PROJFS_PACKAGESDIR]%PROJFS_PACKAGESDIR%

:: Clean up
SET _PARSED_PATH_=

GOTO :EOF

:RESOLVEPATH
SET "_PARSED_PATH_=%~f1"
GOTO :EOF
