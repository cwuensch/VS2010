#FROM cwuensch/vs2010:vcexpress
FROM mcr.microsoft.com/windows/servercore:ltsc2019-amd64

# Install Visual Studio 2010 Express
RUN echo Downloading... \
 && echo  ALT: powershell -command " (New-Object Net.WebClient).DownloadFile('http://mc.tms-taps.net/temp/VS/VCExpressInstall.zip', 'C:\VCExpressInstall.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_1.zip', 'C:\VCExpressInstall_1.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_2.zip', 'C:\VCExpressInstall_2.zip') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_3.zip', 'C:\VCExpressInstall_3.zip') " \
 && echo  ALT2: powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_4.zip', 'C:\VCExpressInstall_4.zip') " \
 && echo  ALT2: powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_5.z01', 'C:\VCExpressInstall_5.z01') " \
 && echo  ALT2: powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_5.z02', 'C:\VCExpressInstall_5.z02') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/VCExpressInstall/VCExpressInstall_dummy.zip', 'C:\VCExpressInstall_dummy.zip') " \
 && echo Extracting... \
 && echo  ALT: powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall.zip -DestinationPath C:\VCExpress " \
 && echo  ALT2: copy /B "C:\VCExpressInstall_5.z01" + "C:\VCExpressInstall_5.z02" "C:\VCExpressInstall_5.zip" \
 && powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall_1.zip -DestinationPath C:\VCExpress " \
 && powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall_2.zip -DestinationPath C:\VCExpress " \
 && powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall_3.zip -DestinationPath C:\VCExpress " \
 && echo  ALT2: powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall_4.zip -DestinationPath C:\VCExpress " \
 && echo  ALT2: powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall_5.zip -DestinationPath C:\VCExpress " \
 && echo powershell -command " Expand-Archive -Force -Path C:\VCExpressInstall_dummy.zip -DestinationPath C:\VCExpress " \
 && del "C:\VCExpressInstall_*" \
 && echo Installing... \
 && ( C:\VCExpress\Setup.exe /quiet || echo. ) \
 && echo Deleting... \
 && del "C:\VS_EXPBSLN_x64_deu.MSI" \
 && del "C:\VS_EXPBSLN_x64_deu.CAB" \
 && rmdir /s /q "C:\VCExpress" \
 && ( rmdir /s /q "C:\Program Files\Microsoft SQL Server" || echo. ) \
 && ( rmdir /s /q "C:\Program Files\Microsoft SQL Server Compact Edition" || echo. ) \
 && ( rmdir /s /q "C:\Program Files\Microsoft Synchronization Services" || echo. ) \
 && ( rmdir /s /q "C:\Program Files (x86)\Microsoft Silverlight" || echo. ) \
 && ( rmdir /s /q "C:\Program Files (x86)\Microsoft SQL Server" || echo. ) \
 && ( rmdir /s /q "C:\Program Files (x86)\Microsoft SQL Server Compact Edition" || echo. ) \
 && ( rmdir /s /q "C:\Program Files (x86)\Microsoft Synchronization Services" || echo. ) \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Microsoft Visual C++ 2010 Express - DEU" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\1031" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\SDK" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Licenses" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Xml" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Packages" \
 && mkdir "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE2\PublicAssemblies" \
 && copy "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\PublicAssemblies\Microsoft.VisualStudio.VCProjectEngine.dll" "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE2\PublicAssemblies\" \
 && copy "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\msobj100.dll" "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE2\" \
 && copy "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\mspdb*.*" "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE2\" \
 && ( del /s /f /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\*" || echo. ) \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE2\*" "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE2"

#RUN setx VS100COMNTOOLS "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\\"


## Install Windows SDK v7.1 AND create Windows SDK v7.1A
#RUN echo Downloading... \
# && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/WinSDKBuild_x86.msi', 'C:\WinSDKBuild_x86.msi') " \
# && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab', 'C:\cab1.cab') " \
# && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab', 'C:\cab2.cab') " \
# && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab', 'C:\cab3.cab') " \
# && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab', 'C:\cab4.cab') " \
# && echo Installing... \
# && msiexec /i C:\WinSDKBuild_x86.msi /quiet /qn \
# && del "C:\WinSDKBuild_x86.msi" \
# && del "C:\cab*.cab" \
# && echo Copying... \
# && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\Lib\x64" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\" \
# && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\Lib\IA64" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\IA64\" \
# && copy /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\Include\*.*s" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include\" \
# && rmdir /s /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1"
#
## Copy missing file from Windows SDK v7.1A
#ARG target="C:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A/Include/"
#COPY WinRes.h ${target}

# Copy headers and libs from Windows SDK v7.1A into v7.0A (identical), copy Tools from v7.1A
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WindowsSDK_7.1A.zip', 'C:\WindowsSDK_7.1A.zip') " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Force -Path C:\WindowsSDK_7.1A.zip -DestinationPath 'C:\Program Files (x86)\Microsoft SDKs\Windows\' " \
 && del "C:\WindowsSDK_7.1A.zip" \
 && echo Copying... \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\" \
 && copy /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include\*.*s" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include\" \
 && copy /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include\WinRes.h" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include\" \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\de" \
 && del /f /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\*.*" \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A" \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/WinSDK71.reg', 'C:\WinSDK71.reg') " \
 && reg import C:\WinSDK71.reg \
 && del C:\WinSDK71.reg


# Install VC 2010 SP1 x86 compilers (incl. cross-compiler for x64)
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/Compiler_SP1/vc_stdx86.msi', 'C:\vc_stdx86.msi') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/Compiler_SP1/vc_stdx86.cab', 'C:\vc_stdx86.cab') " \
 && echo Installing... \
 && msiexec /i C:\vc_stdx86.msi /quiet /qn \
 && echo Deleting... \
 && del "C:\vc_stdx86.msi" \
 && del "C:\vc_stdx86.cab" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\x86_ia64" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\ia64" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\ia64" \
 && ( rmdir /s /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1" || echo. )

# Install VC 2010 SP1 native x64 compilers (optional)
RUN echo Downloading... \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/Compiler_SP1/vc_stdamd64.msi', 'C:\vc_stdamd64.msi') " \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/Compiler_SP1/vc_stdamd64.cab', 'C:\vc_stdamd64.cab') " \
 && echo Installing... \
 && msiexec /i C:\vc_stdamd64.msi /quiet /qn \
 && echo Deleting... \
 && del "C:\vc_stdamd64.msi" \
 && del "C:\vc_stdamd64.cab" \
 && powershell -command " (New-Object Net.WebClient).DownloadFile('https://github.com/cwuensch/VS2010/raw/master/vcvars64.bat', 'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\vcvars64.bat') "


# Include MSBuild in path (prefer native x64 compiler)
RUN setx PATH "%PATH%;C:\Windows\Microsoft.NET\Framework64\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v4.0.30319"

# Set MSBuild as entrypoint
ENTRYPOINT ["C:/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"]
CMD ["/help"]
