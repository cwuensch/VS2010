@set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0\
@set VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\
@set VS100COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\
@set DevEnvDir=%VSINSTALLDIR%Common7\IDE\
@set WindowsSdkDir=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\
@set FrameworkDir32=C:\Windows\Microsoft.NET\Framework\
@set FrameworkDir64=C:\Windows\Microsoft.NET\Framework64\
@set FrameworkVersion32=v4.0.30319
@set FrameworkVersion64=v4.0.30319
@set Framework35Version=v3.5

@echo Setting environment for using Microsoft Visual Studio 2010 x64 tools.
@set INCLUDE=%VCINSTALLDIR%INCLUDE;%VCINSTALLDIR%ATLMFC\INCLUDE;%WindowsSdkDir%include
@set LIB=%VCINSTALLDIR%LIB\amd64;%VCINSTALLDIR%ATLMFC\LIB\amd64;%WindowsSdkDir%lib\x64
@set LIBPATH=%FrameworkDir64%%FrameworkVersion64%;%FrameworkDir64%%Framework35Version%;%VCINSTALLDIR%LIB\amd64;%VCINSTALLDIR%ATLMFC\LIB\amd64
@set PATH=%DevEnvDir%;%VCINSTALLDIR%BIN\amd64;%VSINSTALLDIR%Common7\Tools;%FrameworkDir64%%FrameworkVersion64%;%FrameworkDir64%%Framework35Version%;%PATH%
