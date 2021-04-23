#FROM cwuensch/vs2010:vcexpress
FROM mcr.microsoft.com/windows/servercore:ltsc2019-amd64

#COPY VCExpressInstall/ C:/

# Install Visual Studio 2010 Express
RUN echo Downloading... \
 && powershell -command " Invoke-WebRequest -Uri http://mc.tms-taps.net/temp/VS/VCExpressInstall.zip -UseBasicParsing -OutFile C:/VCExpressInstall.zip " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Force -Path C:/VCExpressInstall.zip -DestinationPath C:/VCExpress " \
 && del "C:\VCExpressInstall*.zip" \
 && echo Installing... \
 && C:\VCExpress\Setup.exe /quiet \
 && start /wait cmd /c "C:\VCExpress\Setup.exe /quiet || call echo Installation returned: %^errorLevel%" \
 && echo Deleting... \
 && del "C:\VS_EXPBSLN_x64_deu.MSI" \
 && del "C:\VS_EXPBSLN_x64_deu.CAB" \
 && rmdir /s /q "C:\VCExpress" \
 && rmdir /s /q "C:\Program Files\Microsoft SQL Server" \
 && rmdir /s /q "C:\Program Files\Microsoft SQL Server Compact Edition" \
 && rmdir /s /q "C:\Program Files\Microsoft Synchronization Services" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Silverlight" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft SQL Server" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft SQL Server Compact Edition" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft Synchronization Services" \
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

#RUN setx VS100COMNTOOLS "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\"

## Install Windows SDK v7.1 AND create Windows SDK v7.1A
#RUN echo Downloading... \
# && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/WinSDKBuild_x86.msi -UseBasicParsing -OutFile C:\WinSDKBuild_x86.msi " \
# && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab -UseBasicParsing -OutFile C:/cab1.cab " \
# && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab -UseBasicParsing -OutFile C:/cab2.cab " \
# && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab -UseBasicParsing -OutFile C:/cab3.cab " \
# && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WinSDKBuild/cab1.cab -UseBasicParsing -OutFile C:/cab4.cab " \
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
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WindowsSDK_7.1A.zip -UseBasicParsing -OutFile C:\WindowsSDK_7.1A.zip " \
 && echo Extracting... \
 && powershell -command " Expand-Archive -Path C:/WindowsSDK_7.1A.zip -DestinationPath 'C:\Program Files (x86)\Microsoft SDKs\Windows\' " \
 && del "C:\WindowsSDK_7.1A.zip" \
 && echo Copying... \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\" \
 && copy /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include\*.*s" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include\" \
 && copy /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include\WinRes.h" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include\" \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\x64" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\" \
 && xcopy /s /y "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin" "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\" \
 && rmdir /s /q "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A" \
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/WinSDK71.reg -UseBasicParsing -OutFile C:\WinSDK71.reg " \
 && reg import C:\WinSDK71.reg \
 && del C:\WinSDK71.reg


# Install VC 2010 SP1 x86 compilers (incl. cross-compiler for x64)
RUN echo Downloading... \
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/vc_stdx86/vc_stdx86.msi -UseBasicParsing -OutFile C:\vc_stdx86.msi " \
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/vc_stdx86/vc_stdx86.cab -UseBasicParsing -OutFile C:\vc_stdx86.cab " \
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
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/vc_stdamd64/vc_stdamd64.msi -UseBasicParsing -OutFile C:\vc_stdamd64.msi " \
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/vc_stdamd64/vc_stdamd64.cab -UseBasicParsing -OutFile C:\vc_stdamd64.cab " \
 && powershell -command " Invoke-WebRequest -Uri https://github.com/cwuensch/VS2010/raw/master/vcvars64.bat -UseBasicParsing -OutFile 'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin/amd64/vcvars64.bat' " \
 && echo Installing... \
 && msiexec /i C:\vc_stdamd64.msi /quiet /qn \
 && echo Deleting... \
 && del "C:\vc_stdamd64.msi" \
 && del "C:\vc_stdamd64.cab"

# Prefer native x64 compiler (optional)
RUN setx PATH C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\BIN\amd64;%PATH%

# Include MSBuild in path
RUN setx PATH %PATH%;C:\Windows\Microsoft.NET\Framework\v4.0.30319

# Set MSBuild as entrypoint
ENTRYPOINT ["C:/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"]
CMD ["/help"]
