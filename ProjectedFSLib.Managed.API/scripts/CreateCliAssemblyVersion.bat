REM Usage:
REM CreateCliAssemblyVersion ProjectName VersionString SolutionDir
set ProjectName=%1
set VersionString=%2
set SolutionDir=%3

mkdir %SolutionDir%\BuildOutput
mkdir %SolutionDir%\BuildOutput\%ProjectName%
echo #include "stdafx.h" > %SolutionDir%\BuildOutput\%ProjectName%\AssemblyVersion.h
echo using namespace System::Reflection; >> %SolutionDir%\BuildOutput\%ProjectName%\AssemblyVersion.h
echo [assembly:AssemblyVersion("%VersionString%")]; >> %SolutionDir%\BuildOutput\%ProjectName%\AssemblyVersion.h
echo [assembly:AssemblyFileVersion("%VersionString%")]; >> %SolutionDir%\BuildOutput\%ProjectName%\AssemblyVersion.h
echo [assembly:AssemblyKeyFileAttribute(LR"(%SolutionDir%ProjectedFSLib.Managed.API\signing\35MSSharedLib1024.snk)")]; >> %SolutionDir%\BuildOutput\%ProjectName%\AssemblyVersion.h
echo [assembly:AssemblyDelaySignAttribute(true)]; >> %SolutionDir%\BuildOutput\%ProjectName%\AssemblyVersion.h