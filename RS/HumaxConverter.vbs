' Humax Converter 1.0
' ===================
' 
' Konvertiert eine Humax PVR8000 Aufnahme (als Parameter übergeben)
' in einen 188-Byte Transport Stream.
' Es werden die Humax-Header, die im Abstand von 32768 Bytes auftreten
' und jeweils eine Länge von 1184 Bytes haben, entfernt.
' Es erfolgt keine Prüfung, ob die Entfernung sinnvoll ist!!
'
' (C) 2020 Christian Wünsch


Option Explicit

Const ForReading = 1, ForWriting = 2, ForAppending = 8
Const HumaxHeaderAnfang = &HFD04417F
Const HumaxHeaderAnfang2 = &H78123456

Dim fso
Dim InputFiles, InFileName, OutFileName
Dim fIn, fOut, Blob, p


' Parameter prüfen
Set InputFiles = WScript.Arguments
If (InputFiles.Count = 0) Then
  MsgBox "Bitte eine oder mehrere zu konvertierende Humax-Dateien (*.vid) übergeben!", VBOKOnly + vbInformation, "Humax Converter"
  WScript.Quit
End If

Set fso = CreateObject("Scripting.FileSystemObject")

' Für alle übergebenen Dateien
Set InputFiles = WScript.Arguments
For Each InFileName In InputFiles

  ' Dateierweiterung prüfen und OutFileName berechnen
  p = InStrRev(InFileName, ".")
  If (p > 0) Then
    If (Mid(InFileName, p) = ".vid") Then
      OutFileName = Mid(InFileName, 1, p - 1) & "_filtered.ts"
    Else
      MsgBox "'" & InFileName & "' ist keine .vid-Datei!", VBOKOnly + vbCritical, "Humax Converter"
      WScript.Quit
    End If
  End If

  ' Input und Output öffnen
  Set fIn = fso.OpenTextFile(InFileName, ForReading)
  Set fOut = fso.OpenTextFile(OutFileName, ForWriting, True)

  ' Daten umkopieren
  Do Until fIn.AtEndOfStream
    Blob = fIn.Read(31584)
    fOut.Write Blob
    Blob = fIn.Skip(1184)
  Loop

  ' Dateien schließen
  fOut.Close
  fIn.Close
Next

MsgBox "Vorgang abgeschlossen! " & InputFiles.Count & " Datei(en) konvertiert.", VBOKOnly + vbInformation, "Humax Converter"
