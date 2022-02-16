FROM mcr.microsoft.com/windows/servercore:ltsc2019-amd64

# Copy Windows SDK v7.1A as v7.0A (identical, except Bin)
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/WindowsSDK_7.1A.zip', 'C:\WindowsSDK_7.1A.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/NETFX_4.0_Tools.zip', 'C:\NETFX_4.0_Tools.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/WinSDK70.reg', 'C:\WinSDK70.reg') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/WinSDK71.reg', 'C:\WinSDK71.reg') " \
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
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/Reference_Assemblies.zip', 'C:\Reference_Assemblies.zip') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/Reference_Assemblies.zip -DestinationPath 'C:\Program Files (x86)\Reference Assemblies\' " \
 && echo Deleting... \
 && del "C:\Reference_Assemblies.zip"


# Copy MSBuild
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/MSBuild_x86.zip', 'C:\MSBuild_x86.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/MSBuild.reg', 'C:\MSBuild.reg') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/MSBuild_x86.zip -DestinationPath 'C:\Program Files (x86)\MSBuild\' " \
 && echo Registry import... \
 && reg import "C:\MSBuild.reg" \
 && echo Deleting... \
 && del "C:\MSBuild_x86.zip" \
 && del "C:\MSBuild.reg"


# Copy Visual Studio 2010 compilers
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/Compilers_2010_SP1.zip', 'C:\Compilers_2010_SP1.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/VS2010_2.reg', 'C:\VS2010_2.reg') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/VS2010_3.reg', 'C:\VS2010_3.reg') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/Compilers_2010_SP1.zip -DestinationPath 'C:\Program Files (x86)\Microsoft Visual Studio 10.0\' " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/vcvars64.bat', 'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\vcvars64.bat') " \
 && echo Registry import... \
 && reg import "C:\VS2010_2.reg" \
 && reg import "C:\VS2010_3.reg" \
 && echo Deleting... \
 && del "C:\Compilers_2010_SP1.zip" \
 && del "C:\VS2010*.reg"

# Copy VS2010 compiler system files
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/Compilers_Windows.zip', 'C:\Compilers_Windows.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/micro/assembly.NET.zip', 'C:\assembly.NET.zip') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Force -Path C:/Compilers_Windows.zip -DestinationPath 'C:\Windows\' " \
 && powershell -command " Expand-Archive -Force -Path C:/assembly.NET.zip -DestinationPath 'C:\Windows\Microsoft.NET\assembly\' " \
 && echo Deleting... \
 && del "C:\Compilers_Windows.zip" \
 && del "C:\assembly.NET.zip"

RUN setx VS100COMNTOOLS "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\\"

# Set path (experimental)
#RUN setx PATH "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\VCPackages;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin\NETFX 4.0 Tools;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin;%PATH%"
RUN setx PATH "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\;%PATH%"

# Include MSBuild in path (prefer native x64 compiler)
RUN setx PATH "%PATH%;C:\Windows\Microsoft.NET\Framework64\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v4.0.30319"

# Set MSBuild as entrypoint
ENTRYPOINT ["C:/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"]
CMD ["/help"]
