REM Usage:
REM CreateCliAssemblyVersion ProjectName VersionString SolutionDir
set ProjectName=%1
set VersionString=%2
set SolutionDir=%3

mkdir %SolutionDir%\..\BuildOutput
mkdir %SolutionDir%\..\BuildOutput\%ProjectName%
echo #include "stdafx.h" > %SolutionDir%\..\BuildOutput\%ProjectName%\AssemblyVersion.h
echo using namespace System::Reflection; [assembly:AssemblyVersion("%VersionString%")];[assembly:AssemblyFileVersion("%VersionString%")]; >> %SolutionDir%\..\BuildOutput\%ProjectName%\AssemblyVersion.h