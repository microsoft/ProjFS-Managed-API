REM Usage:
REM CreateVersionHeader.bat ProjectName VersionString SolutionDir
set ProjectName=%1
set VersionString=%2
set SolutionDir=%3

mkdir %SolutionDir%\BuildOutput
mkdir %SolutionDir%\BuildOutput\%ProjectName%

set comma_version_string=%VersionString%
set comma_version_string=%comma_version_string:.=,%

echo #define PRJLIB_FILE_VERSION %comma_version_string% > %SolutionDir%\BuildOutput\%ProjectName%\VersionHeader.h
echo #define PRJLIB_FILE_VERSION_STRING "%VersionString%" >> %SolutionDir%\BuildOutput\%ProjectName%\VersionHeader.h
echo #define PRJLIB_PRODUCT_VERSION %comma_version_string% >> %SolutionDir%\BuildOutput\%ProjectName%\VersionHeader.h
echo #define PRJLIB_PRODUCT_VERSION_STRING "%VersionString%" >> %SolutionDir%\BuildOutput\%ProjectName%\VersionHeader.h