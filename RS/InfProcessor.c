#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "type.h"
#include "InfProcessor.h"
#include "RebuildInf.h"
#include "RecStrip.h"
#include "HumaxHeader.h"
#include "EycosHeader.h"
#include "TtxProcessor.h"

extern bool StrToUTF8(const char *SourceString, char *DestString, byte DefaultISO8859CharSet);


// Globale Variablen
byte                   *InfBuffer = NULL;    // dirty hack: erreichbar machen für OpenInputFiles()
TYPE_RecHeader_Info    *RecHeaderInfo = NULL;
static char             OldEventText[1025];
static size_t           InfSize = 0;
tPVRTime                OrigStartTime = 0;  //, CurrentStartTime = 0;
byte                    OrigStartSec = 0;   //, CurrentStartSec = 0;


// ----------------------------------------------
// *****  READ AND WRITE INF FILE  *****
// ----------------------------------------------

static SYSTEM_TYPE DetermineInfType(const byte *const InfBuffer, const unsigned long long InfFileSize)
{
  int                   AntiS = 0, AntiT = 0, AntiC = 0;
  SYSTEM_TYPE           SizeType = ST_UNKNOWN, Result = ST_UNKNOWN;

  TYPE_RecHeader_TMSS  *Inf_TMSS = (TYPE_RecHeader_TMSS*)InfBuffer;
  TYPE_RecHeader_TMSC  *Inf_TMSC = (TYPE_RecHeader_TMSC*)InfBuffer;
  TYPE_RecHeader_TMST  *Inf_TMST = (TYPE_RecHeader_TMST*)InfBuffer;

  TRACEENTER;

  // Dateigröße beurteilen
  if (((InfFileSize % 122312) % 1024 == 84) || ((InfFileSize % 122312) % 1024 == 248))
  {
//    PointsS++;
//    PointsT++;
    SizeType = ST_TMSS;
  }
  else if (((InfFileSize % 122312) % 1024 == 80) || ((InfFileSize % 122312) % 1024 == 244))
  {
//    PointsC++;
    SizeType = ST_TMSC;
  }

  // Frequenzdaten

  //ST_TMSS: Frequency = dword @ 0x0578 (10000...13000)
  //         SymbolRate = word @ 0x057c (2000...30000)
  //         Flags1 = word     @ 0x0575 (& 0xf000 == 0)
  //
  //ST_TMST: Frequency = dword @ 0x0578 (47000...862000)
  //         Bandwidth = byte  @ 0x0576 (6..8)
  //         LPHP = byte       @ 0x057e (& 0xfe == 0)
  //
  //ST_TMSC: Frequency = dword @ 0x0574 (174000...230000 || 470000...862000)
  //         SymbolRate = word @ 0x0578 (2000...30000)
  //         Modulation = byte @ 0x057e (<= 4)

  if (Inf_TMSS->TransponderInfo.Frequency    &&  !((Inf_TMSS->TransponderInfo.Frequency      >=     10700)  &&  (Inf_TMSS->TransponderInfo.Frequency  <=     12750)))  AntiS++;
  if (Inf_TMSS->TransponderInfo.SymbolRate   &&  !((Inf_TMSS->TransponderInfo.SymbolRate     >=     10000)  &&  (Inf_TMSS->TransponderInfo.SymbolRate <=     30000)))  AntiS++;
  if (Inf_TMSS->TransponderInfo.UnusedFlags1  ||  Inf_TMSS->TransponderInfo.Unknown2)  AntiS++;
  if (Inf_TMSS->TransponderInfo.TPMode > 1)  AntiS++;
  if ((dword)Inf_TMSS->BookmarkInfo.NrBookmarks > 64)  AntiS++;

  if (Inf_TMST->TransponderInfo.Frequency    &&  !((Inf_TMST->TransponderInfo.Frequency      >=     47000)  &&  (Inf_TMST->TransponderInfo.Frequency  <=    862000)))  AntiT++;
  if (Inf_TMST->TransponderInfo.Bandwidth    &&  !((Inf_TMST->TransponderInfo.Bandwidth      >=         6)  &&  (Inf_TMST->TransponderInfo.Bandwidth  <=         8)))  AntiT++;
  if (Inf_TMST->TransponderInfo.LPHP > 1  ||  Inf_TMST->TransponderInfo.unused2)  AntiT++;
  if (Inf_TMST->TransponderInfo.SatIdx != 0)  AntiT++;
  if ((dword)Inf_TMST->BookmarkInfo.NrBookmarks > 64)  AntiT++;

  if (Inf_TMSC->TransponderInfo.Frequency    &&  !((Inf_TMSC->TransponderInfo.Frequency      >=     47000)  &&  (Inf_TMSC->TransponderInfo.Frequency  <=    862000)))  AntiC++;
  if (Inf_TMSC->TransponderInfo.SymbolRate   &&  !((Inf_TMSC->TransponderInfo.SymbolRate     >=      6111)  &&  (Inf_TMSC->TransponderInfo.SymbolRate <=      6900)))  AntiC++;
  if (Inf_TMSC->TransponderInfo.ModulationType > 4)  AntiC++;
  if (Inf_TMSC->TransponderInfo.SatIdx != 0)  AntiC++;
  if ((dword)Inf_TMSC->BookmarkInfo.NrBookmarks > 64)  AntiC++;

  printf("  Determine SystemType: DVBs = %d, DVBt = %d, DVBc = %d Points\n", -AntiS, -AntiT, -AntiC);

  if ((AntiC == 0 && AntiS > 0 && AntiT > 0) || (AntiC == 1 && AntiS > 1 && AntiT > 1))
    Result = ST_TMSC;
  else
    if ((AntiS == 0 && AntiC > 0 && AntiT > 0) || (AntiS == 1 && AntiC > 1 && AntiT > 1))
      Result = ST_TMSS;
    else if ((AntiT == 0 && AntiS > 0 && AntiC > 0) || (AntiT == 1 && AntiS > 1 && AntiC > 1))
      Result = ST_TMST;
    else if (/*(Inf_TMSS->BookmarkInfo.NrBookmarks == 0) &&*/ (Inf_TMSC->BookmarkInfo.NrBookmarks == 0))
      Result = ST_TMSS;

  printf("   -> SystemType=ST_TMS%c\n", (Result==ST_TMSS ? 's' : ((Result==ST_TMSC) ? 'c' : ((Result==ST_TMST) ? 't' : '?'))));
  if (Result != SizeType && SizeType && !(Result == ST_TMST && SizeType == ST_TMSS))
    printf("   -> DEBUG! Assertion error: SystemType in inf (%u) not consistent to filesize (%u)!\n", Result, SizeType);

  TRACEEXIT;
  return Result;
}


bool InfProcessor_Init()
{
  TRACEENTER;

  //Allocate and clear the buffer
  memset(OldEventText, 0, sizeof(OldEventText));
  OrigStartTime = 0;  // CurrentStartTime = 0;
  OrigStartSec = 0;   // CurrentStartSec = 0;
  RecHeaderInfo = NULL;
  BookmarkInfo  = NULL;
  
  InfSize = sizeof(TYPE_RecHeader_TMSS);
  InfBuffer = (byte*) malloc(max(InfSize, 32768));
  if(InfBuffer)
  {
    memset(InfBuffer, 0, max(InfSize, 32768));
    RecHeaderInfo = (TYPE_RecHeader_Info*) InfBuffer;
    BookmarkInfo = &(((TYPE_RecHeader_TMSS*)InfBuffer)->BookmarkInfo);
    TRACEEXIT;
    return TRUE;
  }
  else
  {
    printf("  LoadInfFile() E0901: Not enough memory.\n");
    TRACEEXIT;
    return FALSE;
  }
}

bool LoadInfFromRec(char *AbsRecFileName)
{
  FILE *fIn = NULL;
  bool Result = FALSE;

  TRACEENTER;
  if(!InfBuffer)
  {
    TRACEEXIT;
    return FALSE;
  }

  // Get inf infos from rec
  fIn = fopen(AbsRecFileName, "rb");
  if (fIn)
  {
    setvbuf(fIn, NULL, _IOFBF, BUFSIZE);
//    if (GetPacketSize(fIn, &FileOffset))
//      fseeko64(fIn, FileOffset, SEEK_SET);
  }
  else
  {
    printf("  LoadInfFile() E0902: Cannot open source rec file.\n");
//    InfProcessor_Free();
    TRACEEXIT;
    return FALSE;
  }

  if (HumaxSource)
  {
    Result = LoadHumaxHeader(fIn, PATPMTBuf, (TYPE_RecHeader_TMSS*)InfBuffer);
    if (!Result) HumaxSource = FALSE;
  }
  else if (EycosSource)
  {
    Result = LoadEycosHeader(AbsRecFileName, PATPMTBuf, (TYPE_RecHeader_TMSS*)InfBuffer);
    if (!Result) EycosSource = FALSE;
  }

  Result = GenerateInfFile(fIn, (TYPE_RecHeader_TMSS*)InfBuffer);
  
//  CurrentStartTime = ((TYPE_RecHeader_Info*)InfBuffer)->StartTime;
//  CurrentStartSec  = ((TYPE_RecHeader_Info*)InfBuffer)->StartTimeSec;
  if(!OrigStartTime)
  {
    OrigStartTime = ((TYPE_RecHeader_Info*)InfBuffer)->StartTime;
    OrigStartSec  = ((TYPE_RecHeader_Info*)InfBuffer)->StartTimeSec;
  }

  // OldEventText setzen
//  memset(OldEventText, 0, sizeof(OldEventText));
//  strncpy(OldEventText, ((TYPE_RecHeader_TMSS*)InfBuffer)->ExtEventInfo.Text, min(((TYPE_RecHeader_TMSS*)InfBuffer)->ExtEventInfo.TextLength, sizeof(OldEventText)-1));

  fclose(fIn);
  TRACEEXIT;
  return Result;
}

static void RemoveItemizedText(TYPE_RecHeader_TMSS *RecHeader, char *const NewEventText, int NewTextLen)
{
  TRACEENTER;
  {
    int j = 0, k = 0, p = 0;
    while ((j < 2*RecHeader->ExtEventInfo.NrItemizedPairs) && (p < RecHeader->ExtEventInfo.TextLength))
      if (RecHeader->ExtEventInfo.Text[p++] == '\0')  j++;

    if (j == 2*RecHeader->ExtEventInfo.NrItemizedPairs)
    {
      strncpy(NewEventText, &RecHeader->ExtEventInfo.Text[p], min(RecHeader->ExtEventInfo.TextLength - p, NewTextLen-1));

      p = 0;
      for (k = 0; k < j; k++)
      {
        if((byte)RecHeader->ExtEventInfo.Text[p] < 0x20)  p++;
        snprintf(&NewEventText[strlen(NewEventText)], NewTextLen-strlen(NewEventText), ((k % 2 == 0) ? (((byte)NewEventText[0] >= 0x15) ? "\xC2\x8A%s: " : "\x8A%s: ") : "%s"), &RecHeader->ExtEventInfo.Text[p]);
        p += (int)strlen(&RecHeader->ExtEventInfo.Text[p]) + 1;
      }
    }
    else
      strncpy(NewEventText, RecHeader->ExtEventInfo.Text, min(RecHeader->ExtEventInfo.TextLength, NewTextLen-1));
  }
  TRACEEXIT;
}

bool LoadInfFile(char *AbsInfName, bool FirstTime)
{
  FILE                 *fInfIn = NULL;
  TYPE_RecHeader_TMSS  *RecHeader = NULL;
  TYPE_Service_Info    *ServiceInfo = NULL;
  unsigned long long    InfFileSize = 0;
  SYSTEM_TYPE           curSystemType = ST_UNKNOWN;
  size_t                curInfSize, p;
  bool                  HDFound = FALSE, Result = FALSE;
  int                   k;

  TRACEENTER;
  if(!InfBuffer || !RecHeaderInfo)
  {
    TRACEEXIT;
    return FALSE;
  }
  RecHeader = (TYPE_RecHeader_TMSS*)InfBuffer;
  ServiceInfo = &(RecHeader->ServiceInfo);

  //Read the source .inf
  if (AbsInfName && !RebuildInf)
    fInfIn = fopen(AbsInfName, "rb");
  if(fInfIn)
  {
    fseeko64(fInfIn, 0, SEEK_END);
    InfFileSize = ftell(fInfIn);
    rewind(fInfIn);
    Result = (fread(InfBuffer, 1, InfSize, fInfIn) + 4 >= InfSize);
    fclose(fInfIn);
  }
  else
  {
    if(!RebuildInf) printf("  Cannot open inf file %s.\n", AbsInfName);
    if (AbsInfName) AbsInfName[0] = '\0';
    SystemType = ST_TMSS;
//    BookmarkInfo = &(((TYPE_RecHeader_TMSS*)InfBuffer)->BookmarkInfo);
  }

  //Check for correct inf file header
  if ((strncmp(((TYPE_RecHeader_Info*)InfBuffer)->Magic, "TFrc", 4) != 0) || (((TYPE_RecHeader_Info*)InfBuffer)->Version != 0x8000))
  {
    printf("  Invalid inf file header %s.\n", AbsInfName);
    if(RebuildInf)
    {
      if (AbsInfName) AbsInfName[0] = '\0';
      SystemType = ST_TMSS;
    }
    else
    {
      TRACEEXIT;
      return FALSE;
    }
  }
  
  //Decode the source .inf
  if (Result)
  {
    curSystemType = DetermineInfType(InfBuffer, InfFileSize);
    switch (curSystemType)
    {
      case ST_TMSS:
        curInfSize = sizeof(TYPE_RecHeader_TMSS);
        BookmarkInfo = &(((TYPE_RecHeader_TMSS*)InfBuffer)->BookmarkInfo);
        break;
      case ST_TMSC:
        curInfSize = sizeof(TYPE_RecHeader_TMSC);
        BookmarkInfo = &(((TYPE_RecHeader_TMSC*)InfBuffer)->BookmarkInfo);
        break;
      case ST_TMST:
        curInfSize = sizeof(TYPE_RecHeader_TMST);
        BookmarkInfo = &(((TYPE_RecHeader_TMST*)InfBuffer)->BookmarkInfo);
        break;
      default:
        printf("  LoadInfFile() E0903: Incompatible system type.\n");
//        InfProcessor_Free();
        TRACEEXIT;
        return FALSE;
    }
    if (FirstTime)
    {
      SystemType = curSystemType;
      InfSize = curInfSize;
    }

    // Event-Strings von Datenmüll reinigen
    p = (int)strlen(RecHeader->EventInfo.EventNameDescription);
    if (p < sizeof(RecHeader->EventInfo.EventNameDescription))
      memset(&RecHeader->EventInfo.EventNameDescription[p], 0, sizeof(RecHeader->EventInfo.EventNameDescription) - p);
    p = RecHeader->ExtEventInfo.TextLength;
    if (p < sizeof(RecHeader->ExtEventInfo.Text))
      memset(&RecHeader->ExtEventInfo.Text[p], 0, sizeof(RecHeader->ExtEventInfo.Text) - p);

    // Prüfe auf verschlüsselte Aufnahme
    if (((RecHeaderInfo->CryptFlag & 1) != 0) && !*RecFileOut)
    {
      printf("  LoadInfFile() E0904: Recording is encrypted.\n");
//      InfProcessor_Free();
      TRACEEXIT;
      return FALSE;
    }

    // Prüfe auf HD-Video
    if ((ServiceInfo->VideoStreamType==STREAM_VIDEO_MPEG4_PART2) || (ServiceInfo->VideoStreamType==STREAM_VIDEO_MPEG4_H264) || (ServiceInfo->VideoStreamType==STREAM_VIDEO_MPEG4_H263))
    {
      HDFound = TRUE;
      printf("  INF: VideoStream=0x%x, VideoPID=%hu, HD=%d\n", ServiceInfo->VideoStreamType, ServiceInfo->VideoPID, HDFound);
    }
    else if ((ServiceInfo->VideoStreamType==STREAM_VIDEO_MPEG1) || (ServiceInfo->VideoStreamType==STREAM_VIDEO_MPEG2))
    {
      HDFound = FALSE;
      printf("  INF: VideoStream=0x%x, VideoPID=%hu, HD=%d\n", ServiceInfo->VideoStreamType, ServiceInfo->VideoPID, HDFound);
    }
    else
    {
      VideoPID = (word) -1;
      printf("  LoadInfFile() W0901: Unknown video stream type.\n");
    }

if (RecHeaderInfo->Reserved != 0)
  printf("  DEBUG! Assertion Error: Reserved-Flags is not 0.\n");

    if (VideoPID != ServiceInfo->VideoPID)
    {
      printf("  LoadInfFile() E0905: Inconsistant video PID: inf=%hd, rec=%hd.\n", ServiceInfo->VideoPID, VideoPID);
//      InfProcessor_Free();
      VideoPID = ServiceInfo->VideoPID;
      ContinuityPIDs[0] = VideoPID;
      TRACEEXIT;
      return FALSE;
    }
    if (HDFound != isHDVideo)
    {
      printf("  LoadInfFile() E0906: Inconsistant video type: inf=%s, rec=%s.\n", (HDFound ? "HD" : "SD"), (isHDVideo ? "HD" : "SD"));
//      InfProcessor_Free();
      isHDVideo = HDFound;
      TRACEEXIT;
      return FALSE;
    }

    // Prüfe, ob Audio-PID in Continuity-Scan-Liste enthalten ist
    for (k = 1; k < NrContinuityPIDs; k++)
    {
      if (ContinuityPIDs[k] == ServiceInfo->AudioPID)
        break;
    }
    if (k >= NrContinuityPIDs)
    {
      if (NrContinuityPIDs < MAXCONTINUITYPIDS)
        NrContinuityPIDs++;
      for (k = NrContinuityPIDs-1; k > 1; k--)
        ContinuityPIDs[k] = ContinuityPIDs[k-1];
      ContinuityPIDs[1] = ServiceInfo->AudioPID;
    }

    if(FirstTime)
    {
      if(RecHeaderInfo->rs_HasBeenStripped)  AlreadyStripped = TRUE;

      if (abs((int)(RecHeaderInfo->StartTime - OrigStartTime)) > 1)
      {
        time_t StartTimeUnix = TF2UnixTime(RecHeaderInfo->StartTime, RecHeaderInfo->StartTimeSec, FALSE);
        printf("  INF: StartTime (%s) differs from TS start! Taking %s.\n", TimeStr(&StartTimeUnix), ((RecHeaderInfo->StartTimeSec != 0 || OrigStartSec == 0) ? "inf" : "TS"));
      }
      if (RecHeaderInfo->StartTimeSec != 0 || OrigStartSec == 0)
      {
        OrigStartTime = RecHeaderInfo->StartTime;
        OrigStartSec  = RecHeaderInfo->StartTimeSec;
      }
      else
      {
        RecHeaderInfo->StartTime    = OrigStartTime;
        RecHeaderInfo->StartTimeSec = OrigStartSec;
      }
    }
/*    if (RecHeaderInfo->StartTimeSec)
    {
      CurrentStartTime = RecHeaderInfo->StartTime;
      CurrentStartSec  = RecHeaderInfo->StartTimeSec;
    } */

    InfDuration = 60*RecHeaderInfo->DurationMin + RecHeaderInfo->DurationSec;
  }

  // OldEventText setzen + ggf. Itemized Items in ExtEventText entfernen
  memset(OldEventText, 0, sizeof(OldEventText));
  RemoveItemizedText(RecHeader, OldEventText, sizeof(OldEventText));

  TRACEEXIT;
  return TRUE;
}

void SetInfEventText(const char *pCaption)
{
  char *NewEventText = NULL;
  TYPE_RecHeader_TMSS *RecHeader = (TYPE_RecHeader_TMSS*)InfBuffer;

  TRACEENTER;
  memset(RecHeader->ExtEventInfo.Text, 0, sizeof(RecHeader->ExtEventInfo.Text));

  if (pCaption)
  {
/*    if ((NewEventText = (char*)malloc(2 * strlen(pCaption))))
    {
      if ((byte)OldEventText[0] >= 0x15)
      {
        StrToUTF8(pCaption, NewEventText, 9);
        p = strlen(NewEventText);
        if (p < sizeof(RecHeader->ExtEventInfo.Text) - 4)
        {
          NewEventText[p++] = '\xC2'; NewEventText[p++] = '\x8A'; NewEventText[p++] = '\xC2'; NewEventText[p++] = '\x8A';
        }
      }
      else
      {
        NewEventText[0] = '\5';
        strncpy(&NewEventText[1], pCaption, sizeof(RecHeader->ExtEventInfo.Text) - 2);
        p = strlen(NewEventText);
        if (p < sizeof(RecHeader->ExtEventInfo.Text) - 2)
        {
          NewEventText[p++] = '\x8A'; NewEventText[p++] = '\x8A';
        }
      }
      strncpy(&NewEventText[p], &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0], sizeof(RecHeader->ExtEventInfo.Text) - p - 1);
      if (RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 2] != 0)
        snprintf(&RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 4], 4, "...");
      free(NewEventText);
    } */

    if ((NewEventText = (char*)malloc(2 * strlen(pCaption) + strlen(OldEventText) + 5)))
    {
      if ((byte)OldEventText[0] >= 0x15)
      {
        StrToUTF8(pCaption, NewEventText, 9);
        sprintf(&NewEventText[strlen(NewEventText)], "\xC2\x8A\xC2\x8A%s", &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0]);
      }
      else
        sprintf(NewEventText, "\5%s\x8A\x8A%s", pCaption, &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0]);
      strncpy(RecHeader->ExtEventInfo.Text, NewEventText, sizeof(RecHeader->ExtEventInfo.Text) - 1);
      if (RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 2] != 0)
        snprintf(&RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 4], 4, "...");
      free(NewEventText);
    }
  }
  else
    strncpy(RecHeader->ExtEventInfo.Text, OldEventText, sizeof(RecHeader->ExtEventInfo.Text));
  RecHeader->ExtEventInfo.TextLength = (word) min(strlen(RecHeader->ExtEventInfo.Text), sizeof(RecHeader->ExtEventInfo.Text));
  RecHeader->ExtEventInfo.NrItemizedPairs = 0;

  TRACEEXIT;
}

bool SetInfCryptFlag(const char *AbsInfFile)
{
  FILE                 *fInfIn;
  TYPE_RecHeader_Info   RecHeaderInfo;
  bool                  ret = FALSE;

  TRACEENTER;
  if (AbsInfFile)
  {
    if ((fInfIn = fopen(AbsInfFile, "r+b")))
    {
      if ((fread(&RecHeaderInfo, 1, 18, fInfIn) >= 6)
        && ((strncmp(RecHeaderInfo.Magic, "TFrc", 4) == 0) && (RecHeaderInfo.Version == 0x8000)))
      {
        rewind(fInfIn);
        RecHeaderInfo.rs_ScrambledPackets = TRUE;
        RecHeaderInfo.CryptFlag = RecHeaderInfo.CryptFlag | 1;
        ret = (fwrite(&RecHeaderInfo, 1, 18, fInfIn) == 18);
        ret = fclose(fInfIn) && ret;
      }
      else
        fclose(fInfIn);
    }
  }
  TRACEEXIT;
  return ret;
}

bool GetInfStripFlags(const char *AbsInfFile, bool *const OutHasBeenStripped, bool *const OutToBeStripped)
{
  FILE                 *fInfIn;
  TYPE_RecHeader_Info   RecHeaderInfo;

  TRACEENTER;
  if(AbsInfFile)
  {
    if ((fInfIn = fopen(AbsInfFile, "r+b")))
    {
      if ((fread(&RecHeaderInfo, 1, 8, fInfIn) >= 6)
        && ((strncmp(RecHeaderInfo.Magic, "TFrc", 4) == 0) && (RecHeaderInfo.Version == 0x8000)))
      {
        rewind(fInfIn);
        if (OutHasBeenStripped)
          *OutHasBeenStripped = RecHeaderInfo.rs_HasBeenStripped;
        if (OutToBeStripped)
          *OutToBeStripped = RecHeaderInfo.rs_ToBeStripped;
        TRACEEXIT;
        return (fclose(fInfIn));
      }
      else
        fclose(fInfIn);
    }
  }
  TRACEEXIT;
  return FALSE;
}

bool SetInfStripFlags(const char *AbsInfFile, bool SetHasBeenScanned, bool ResetToBeStripped, bool SetStartTime)
{
  FILE                 *fInfIn;
  TYPE_RecHeader_Info   RecHeaderInfo;
  bool                  ret = FALSE;

  TRACEENTER;
  if(AbsInfFile)
  {
    if ((fInfIn = fopen(AbsInfFile, "r+b")))
    {
      if ((fread(&RecHeaderInfo, 1, 12, fInfIn) >= 6)
        && ((strncmp(RecHeaderInfo.Magic, "TFrc", 4) == 0) && (RecHeaderInfo.Version == 0x8000)))
      {
        rewind(fInfIn);
        if (SetHasBeenScanned)
          RecHeaderInfo.rbn_HasBeenScanned = TRUE;
        if (ResetToBeStripped)
          RecHeaderInfo.rs_ToBeStripped = FALSE;
        if (SetStartTime)
        {
          RecHeaderInfo.StartTime   = OrigStartTime;
          RecHeaderInfo.StartTimeSec = OrigStartSec;
        }
        ret = (int) fwrite(&RecHeaderInfo, 1, 12, fInfIn);
        ret = fclose(fInfIn) && ret;
      }
      else
        fclose(fInfIn);
    }
  }
  TRACEEXIT;
  return ret;
}


bool SaveInfFile(const char *AbsDestInf, const char *AbsSourceInf)
{
  FILE                 *fInfIn = NULL, *fInfOut = NULL;
  size_t                BytesRead;
  bool                  Result = FALSE;

  TRACEENTER;
  if(!InfBuffer || !RecHeaderInfo)
  {
    TRACEEXIT;
    return FALSE;
  }

  //Encode the new inf and write it to the disk
  if(AbsDestInf && *AbsDestInf)
    fInfOut = fopen(AbsDestInf, "wb");
  if(fInfOut)
  {
    if ((DoStrip && (DoMerge != 1 || AlreadyStripped)) || MedionStrip)
    {
      RecHeaderInfo->rs_ToBeStripped = FALSE;
      RecHeaderInfo->rs_HasBeenStripped = TRUE;
    }
    if (DoMerge != 1)
      RecHeaderInfo->rbn_HasBeenScanned = TRUE;
    if (NewDurationMS)
    {
      RecHeaderInfo->DurationMin = (word)((NewDurationMS + 500) / 60000);
      RecHeaderInfo->DurationSec = ((NewDurationMS + 500) / 1000) % 60;
    }
    if (NewStartTimeOffset > 0)
      RecHeaderInfo->StartTime = AddTimeSec(OrigStartTime, OrigStartSec, &RecHeaderInfo->StartTimeSec, NewStartTimeOffset / 1000);
    Result = (fwrite(InfBuffer, 1, InfSize, fInfOut) == InfSize);

    // Öffne die Source-inf (falls vorhanden)
    if(AbsSourceInf)
      fInfIn = fopen(AbsSourceInf, "r+b");

    // Kopiere den Rest der Source-inf (falls vorhanden) in die neue inf hinein
    if (fInfIn && !RebuildInf)
    {
      byte *InfBuffer2 = (byte*) malloc(32768);
      fseeko64(fInfIn, InfSize, SEEK_SET);
      do {
        BytesRead = fread(InfBuffer2, 1, 32768, fInfIn);
        if (BytesRead > 0)
          Result = (fwrite(InfBuffer2, 1, BytesRead, fInfOut) == BytesRead) && Result;
      } while (BytesRead > 0);
      free(InfBuffer2);
    }
    if (fInfIn) fclose(fInfIn);

    // Schließe die bearbeitete Datei
//    Result = (fflush(fInfOut) == 0) && Result;
    Result = (fclose(fInfOut) == 0) && Result;
  }
  else
  {
    if (AbsDestInf && *AbsDestInf)
      printf("SaveInfFile() E0908: New inf not created.\n");
    Result = FALSE;
  }
  
  TRACEEXIT;
  return Result;
}

void InfProcessor_Free()
{
  TRACEENTER;
  free(InfBuffer); InfBuffer = NULL;
  RecHeaderInfo = NULL;
  BookmarkInfo = NULL;
  TRACEEXIT;
}
