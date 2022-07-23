# VS2010

For compiling Visual Studio C/C++ projects with Visual Studio 2010 compilers (Windows SDK 7.1, support for Windows XP).

Usage:
------
```
docker run --rm -v ${{ github.workspace }}:C:/source/ --workdir "C:/source" --entrypoint "C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" cwuensch/vs2010:minimal /m /p:Configuration=Release /p:Platform=Win32 RecStrip_VS\RecStrip.sln

docker run --rm -v ${{ github.workspace }}:C:/source/ --workdir "C:/source" --entrypoint "cmd" cwuensch/vs2010:minimal /c "C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /m /p:Configuration=Release /p:Platform=Win32 RecStrip_VS\RecStrip.sln && C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe /m /p:Configuration=Release /p:Platform=x64 RecStrip_VS\RecStrip.sln"
```
