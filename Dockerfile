FROM mcr.microsoft.com/windows/servercore:ltsc2019-amd64

ADD installChoco.ps1 /installChoco.ps1
RUN powershell .\installChoco.ps1 -Wait; Remove-Item c:\installChoco.ps1 -Force;

VOLUME RS

ENTRYPOINT ["C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"]
