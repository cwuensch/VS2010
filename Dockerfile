FROM mcr.microsoft.com/windows/servercore:ltsc2019-amd64

# Copy Windows SDK v7.1A as v7.0A (identical, except Bin)
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/WindowsSDK_7.1A.zip', 'C:\WindowsSDK_7.1A.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/NETFX_4.0_Tools.zip', 'C:\NETFX_4.0_Tools.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/WinSDK70.reg', 'C:\WinSDK70.reg') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/WinSDK71.reg', 'C:\WinSDK71.reg') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/WindowsSDK_7.1A.zip -DestinationPath 'C:\Program Files (x86)\Microsoft SDKs\Windows\' " \
 && powershell -command " Expand-Archive -Path C:/NETFX_4.0_Tools.zip -DestinationPath 'C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\' " \
 && ren "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A" v7.0A \
 && echo Registry import... \
 && reg import "C:\WinSDK70.reg" \
 && reg import "C:\WinSDK71.reg" \
 && echo Deleting... \
 && del "C:\WindowsSDK_7.1A.zip" \
 && del "C:\NETFX_4.0_Tools.zip" \
 && del "C:\*.reg"


# Copy .NET Reference assemblies
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/Reference_Assemblies.zip', 'C:\Reference_Assemblies.zip') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/Reference_Assemblies.zip -DestinationPath 'C:\Program Files (x86)\Reference Assemblies\' " \
 && echo Deleting... \
 && del "C:\Reference_Assemblies.zip"


# Copy MSBuild
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/MSBuild_x86.zip', 'C:\MSBuild_x86.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/MSBuild.reg', 'C:\MSBuild.reg') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/MSBuild_x86.zip -DestinationPath 'C:\Program Files (x86)\MSBuild\' " \
 && echo Registry import... \
 && reg import "C:\MSBuild.reg" \
 && echo Deleting... \
 && del "C:\MSBuild_x86.zip" \
 && del "C:\MSBuild.reg"


# Import VS2010 registry entries
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/VS.reg', 'C:\VS.reg') " \
 && echo Registry import... \
 && reg import "C:\VS.reg" \
 && echo Deleting... \
 && del "C:\VS.reg"

RUN setx VS100COMNTOOLS "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\\"


# Install VC 2010 SP1 x86 compilers (incl. cross-compiler for x64)
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/Compiler_SDK7/vc_stdx86.msi', 'C:\vc_stdx86.msi') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/Compiler_SDK7/vc_stdx86.cab', 'C:\vc_stdx86.cab') " \
 && echo Installing... \
 && start /w msiexec.exe /i C:\vc_stdx86.msi /quiet /qn \
 && echo Deleting... \
 && del "C:\vc_stdx86.msi" \
 && del "C:\vc_stdx86.cab" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\x86_ia64" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\ia64" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\ia64" \
 && ( rmdir /s /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1" || echo. )

# Install VC 2010 SP1 native x64 compilers (optional)
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/Compiler_SDK7/vc_stdamd64.msi', 'C:\vc_stdamd64.msi') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/Compiler_SDK7/vc_stdamd64.cab', 'C:\vc_stdamd64.cab') " \
 && echo Installing... \
 && start /w msiexec.exe /i C:\vc_stdamd64.msi /quiet /qn \
 && echo Deleting... \
 && del "C:\vc_stdamd64.msi" \
 && del "C:\vc_stdamd64.cab" \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/minimal/vcvars64.bat', 'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\vcvars64.bat') "


# Set path (experimental)
#RUN setx PATH "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\VCPackages;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin\NETFX 4.0 Tools;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin;%PATH%"
RUN setx PATH "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\;%PATH%"

# Include MSBuild in path (prefer native x64 compiler)
RUN setx PATH "%PATH%;C:\Windows\Microsoft.NET\Framework64\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v4.0.30319"

# Set MSBuild as entrypoint
ENTRYPOINT ["C:/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"]
CMD ["/help"]
