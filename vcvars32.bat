@set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0\
@set VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\
@set VS100COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\
@set DevEnvDir=%VSINSTALLDIR%Common7\IDE\
@set WindowsSdkDir=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\
@set FrameworkDir32=C:\Windows\Microsoft.NET\Framework\
@set FrameworkDir64=C:\Windows\Microsoft.NET\Framework64\
@set FrameworkVersion32=v4.0.30319
@set FrameworkVersion64=v4.0.30319
@set Framework35Version=v3.5
@set MSBuildExtensionsPath=C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0

@echo Setting environment for using Microsoft Visual Studio 2010 x86 tools.
@set INCLUDE=%VCINSTALLDIR%INCLUDE;%VCINSTALLDIR%ATLMFC\INCLUDE;%WindowsSdkDir%include
@set LIB=%VCINSTALLDIR%LIB;%VCINSTALLDIR%ATLMFC\LIB;%WindowsSdkDir%lib
@set LIBPATH=%FrameworkDir32%%FrameworkVersion32%;%FrameworkDir32%%Framework35Version%;%VCINSTALLDIR%LIB;%VCINSTALLDIR%ATLMFC\LIB
@set PATH=%DevEnvDir%;%VCINSTALLDIR%BIN;%VSINSTALLDIR%Common7\Tools;%FrameworkDir32%%FrameworkVersion32%;%FrameworkDir32%%Framework35Version%;%PATH%
