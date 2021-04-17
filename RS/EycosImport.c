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
#include <time.h>
#include "type.h"
#include "RecStrip.h"
#include "RecHeader.h"
#include "RebuildInf.h"
#include "TtxProcessor.h"
#include "CutProcessor.h"
#include "EycosHeader.h"
#include "HumaxHeader.h"


static time_t MakeUnixDate(word Year, byte Month, byte Day, byte Hour, byte Min, byte Sec)
{
  time_t                UnixTime;
  struct tm             timeinfo;

  TRACEENTER;
  timeinfo.tm_year = Year - 1900;
  timeinfo.tm_mon  = Month - 1;    //months since January - [0,11]
  timeinfo.tm_mday = Day;          //day of the month - [1,31] 
  timeinfo.tm_hour = Hour;         //hours since midnight - [0,23]
  timeinfo.tm_min  = Min;          //minutes after the hour - [0,59]
  timeinfo.tm_sec  = Sec;          //seconds after the minute - [0,59]
  timeinfo.tm_isdst = -1;          //detect Daylight Saving Time according to the timestamp's date
  UnixTime = mktime(&timeinfo);

  TRACEEXIT;
  return UnixTime;
}

char* EycosGetPart(char *const OutEycosPart, const char* AbsTrpName, int NrPart)
{
  TRACEENTER;
  if (OutEycosPart && AbsTrpName && *AbsTrpName)
  {
    strcpy(OutEycosPart, AbsTrpName);
    if (NrPart > 0)
      snprintf(&OutEycosPart[strlen(OutEycosPart)-4], 5, ".%03u", NrPart % 1000);
  }
  return OutEycosPart;
  TRACEEXIT;
}

int EycosGetNrParts(const char* AbsTrpName)
{
  char CurFileName[FBLIB_DIR_SIZE];
  int i = 0;

  TRACEENTER;
  if (AbsTrpName && *AbsTrpName)
  {
    strcpy(CurFileName, AbsTrpName);
    for (i = 1; i < 999; i++)
    {
      snprintf(&CurFileName[strlen(CurFileName)-4], 5, ".%03u", i);
      if(!HDD_FileExist(CurFileName)) break;
    }
  }
  return i;
  TRACEEXIT;
}


bool LoadEycosHeader(char *AbsTrpFileName, byte *const PATPMTBuf, TYPE_RecHeader_TMSS *RecInf)
{
  FILE                 *fIfo, *fTxt, *fIdx;
  char                  IfoFile[FBLIB_DIR_SIZE], TxtFile[FBLIB_DIR_SIZE], IdxFile[FBLIB_DIR_SIZE], *p;
  tEycosHeader          EycosHeader;
  tEycosEvent           EycosEvent;
  tTSPacket            *Packet = NULL;
  tTSPAT               *PAT = NULL;
  tTSPMT               *PMT = NULL;
  tElemStream          *Elem = NULL;
  dword                *CRC = NULL;
  word                  AudioPID = (word) -1;
  int                   Offset = 0;
//  long long             FilePos = ftello64(fIn);
  int                   j, k;
  bool                  ret = FALSE;

  TRACEENTER;
  InitInfStruct(RecInf);
  memset(PATPMTBuf, 0, 2*192);

  // Zusatz-Dateien von Eycos laden
  strcpy(IfoFile, AbsTrpFileName);
  if((p = strrchr(IfoFile, '.'))) *p = '\0';  // ".trp" entfernen
  strcpy(TxtFile, IfoFile);
  strcpy(IdxFile, IfoFile);
  strcat(IfoFile, ".ifo");
  strcat(TxtFile, ".txt");
  strcat(IdxFile, ".idx");

  // Ifo-Datei laden
  if ((fIfo = fopen(IfoFile, "rb")))
  {
    ret = TRUE;
    if ((ret = (ret && fread(&EycosHeader, sizeof(tEycosHeader), 1, fIfo))))
    {
      printf("  Importing Eycos header\n");
      VideoPID              = EycosHeader.VideoPid;
      AudioPID              = EycosHeader.AudioPid;
      TeletextPID           = (word) -1;

      // PAT/PMT initialisieren
      Packet = (tTSPacket*) &PATPMTBuf[4];
      PAT = (tTSPAT*) &Packet->Data[1 /*+ Packet->Data[0]*/];

      Packet->SyncByte      = 'G';
      Packet->PID1          = 0;
      Packet->PID2          = 0;
      Packet->Payload_Unit_Start = 1;
      Packet->Payload_Exists = 1;

      PAT->TableID          = TABLE_PAT;
      PAT->SectionLen1      = 0;
      PAT->SectionLen2      = sizeof(tTSPAT) - 3;
      PAT->Reserved1        = 3;
      PAT->Private          = 0;
      PAT->SectionSyntax    = 1;
      PAT->TS_ID1           = 0;
      PAT->TS_ID2           = 6;  // ??
      PAT->CurNextInd       = 1;
      PAT->VersionNr        = 3;
      PAT->Reserved2        = 3;
      PAT->SectionNr        = 0;
      PAT->LastSection      = 0;
      PAT->ProgramNr1       = 0;
      PAT->ProgramNr2       = 1;
      PAT->PMTPID1          = EycosHeader.PMTPid / 256;
      PAT->PMTPID2          = EycosHeader.PMTPid % 256;
      PAT->Reserved111      = 7;
//      PAT->CRC32            = rocksoft_crc((byte*)PAT, sizeof(tTSPAT)-4);    // CRC: 0x786989a2
//      PAT->CRC32            = crc32m((byte*)PAT, sizeof(tTSPAT)-4);          // CRC: 0x786989a2
      PAT->CRC32            = crc32m_tab((byte*)PAT, sizeof(tTSPAT)-4);      // CRC: 0x786989a2
  
      Offset = 1 + /*Packet->Data[0] +*/ sizeof(tTSPAT);
      memset(&Packet->Data[Offset], 0xff, 184 - Offset);

      Packet = (tTSPacket*) &PATPMTBuf[196];
      PMT = (tTSPMT*) &Packet->Data[1 /*+ Packet->Data[0]*/];

      Packet->SyncByte      = 'G';
      Packet->PID1          = EycosHeader.PMTPid / 256;
      Packet->PID2          = EycosHeader.PMTPid % 256;
      Packet->Payload_Unit_Start = 1;
      Packet->Payload_Exists = 1;

      PMT->TableID          = TABLE_PMT;
      PMT->SectionLen1      = 0;
      PMT->SectionLen2      = sizeof(tTSPMT) - 3 + 4;
      PMT->Reserved1        = 3;
      PMT->Private          = 0;
      PMT->SectionSyntax    = 1;
      PMT->ProgramNr1       = 0;
      PMT->ProgramNr2       = 1;
      PMT->CurNextInd       = 1;
      PMT->VersionNr        = 0;
      PMT->Reserved2        = 3;
      PMT->SectionNr        = 0;
      PMT->LastSection      = 0;

      PMT->Reserved3        = 7;

      PMT->ProgInfoLen1     = 0;
      PMT->ProgInfoLen2     = 0;
      PMT->Reserved4        = 15;

      Offset = 1 + /*Packet->Data[0] +*/ sizeof(tTSPMT);

      RecInf->TransponderInfo.Frequency = EycosHeader.Frequency;

      RecInf->ServiceInfo.ServiceType     = 0;  // SVC_TYPE_Tv
      RecInf->ServiceInfo.ServiceID       = EycosHeader.ServiceID;
      RecInf->ServiceInfo.PMTPID          = EycosHeader.PMTPid;
      RecInf->ServiceInfo.VideoPID        = VideoPID;
      RecInf->ServiceInfo.PCRPID          = VideoPID;
      RecInf->ServiceInfo.AudioPID        = AudioPID;
      RecInf->RecHeaderInfo.StartTime     = 0;  // TODO
//      RecInf->RecHeaderInfo.DurationMin   = 0;  // TODO
//      RecInf->RecHeaderInfo.DurationSec   = 0;  // TODO
      ContinuityPIDs[0] = VideoPID;
      printf("    PMTPID=%hu, SID=%hu, PCRPID=%hu, Stream=0x%hhx, VPID=%hu, TtxPID=%hu\n", RecInf->ServiceInfo.PMTPID, RecInf->ServiceInfo.ServiceID, RecInf->ServiceInfo.PCRPID, RecInf->ServiceInfo.VideoStreamType, VideoPID, TeletextPID);

      strncpy(RecInf->ServiceInfo.ServiceName, EycosHeader.SenderName, sizeof(RecInf->ServiceInfo.ServiceName) - 1);

      // Bookmarks importieren
      ResetSegmentMarkers();
      SegmentMarker[NrSegmentMarker++].Timems = 0;
      for (j = 0; j < min(EycosHeader.NrBookmarks, NRSEGMENTMARKER); j++)
      {
        if (EycosHeader.Bookmarks[j] == 0) break;
        if (EycosHeader.Bookmarks[j] > SegmentMarker[NrSegmentMarker].Timems)
          SegmentMarker[NrSegmentMarker++].Timems = EycosHeader.Bookmarks[j];
        else
          printf("  Eycos-Import: Invalid Bookmark %d: %u is less than %u.\n", j, EycosHeader.Bookmarks[j], SegmentMarker[NrSegmentMarker].Timems);
      }
      SegmentMarker[NrSegmentMarker++].Timems = 0;

      // PIDs in PMT eintragen
      for (j = 0; j < EycosHeader.NrPids; j++)
      {
        tTSAudioDesc *Desc    = NULL;

        if (EycosHeader.Pids[j].PID == AudioPID)
        {
          switch (EycosHeader.Pids[j].Type)
          {
            case STREAM_AUDIO_MPEG1:
            case STREAM_AUDIO_MPEG2:
            case 0x05:
              RecInf->ServiceInfo.AudioStreamType = STREAM_AUDIO_MPEG2;
              RecInf->ServiceInfo.AudioTypeFlag = 0;
              break;
            case  0x0a:
              RecInf->ServiceInfo.AudioStreamType = STREAM_AUDIO_MPEG4_AC3_PLUS;
              RecInf->ServiceInfo.AudioTypeFlag = 1;
              break;
            default:
              RecInf->ServiceInfo.AudioStreamType = STREAM_AUDIO_MPEG2;
              RecInf->ServiceInfo.AudioTypeFlag = 3;
          }
        }

        Elem = (tElemStream*) &Packet->Data[Offset];
        Elem->ESPID1          = EycosHeader.Pids[j].PID / 256;
        Elem->ESPID2          = EycosHeader.Pids[j].PID % 256;
        Elem->Reserved1       = 7;
        Elem->Reserved2       = 0xf;
        Elem->ESInfoLen1      = 0;
        Elem->ESInfoLen2      = 0;

        switch (EycosHeader.Pids[j].Type)
        {
          // Video
          case 0x0b:
            if (EycosHeader.Pids[j].PID == VideoPID)
            {
              isHDVideo = TRUE;  // fortsetzen...
              RecInf->ServiceInfo.VideoStreamType = STREAM_VIDEO_MPEG4_H264;
            }
            Elem->stream_type = STREAM_VIDEO_MPEG4_H264;
            // (fall-through!)

          case STREAM_VIDEO_MPEG1:
          case STREAM_VIDEO_MPEG2:
          {
            Offset              += sizeof(tElemStream);
            PMT->SectionLen2    += sizeof(tElemStream);
            if (RecInf->ServiceInfo.VideoStreamType == 0xff)
            {
              Elem->stream_type  = STREAM_VIDEO_MPEG2;
              if (EycosHeader.Pids[j].PID == VideoPID)
                RecInf->ServiceInfo.VideoStreamType = STREAM_VIDEO_MPEG2;
            }
            printf("    Video Stream: PID=%hu, Type=0x%hhx, HD=%d\n", VideoPID, RecInf->ServiceInfo.VideoStreamType, isHDVideo);
            if (EycosHeader.Pids[j].PID != VideoPID)
              printf("  Eycos-Import: Video stream (PID=%hu, Type=0x%hx) differs from Video PID %hu.\n", EycosHeader.Pids[j].PID, EycosHeader.Pids[j].Type, VideoPID);
            break;
          }

          // Audio normal / AC3
          case STREAM_AUDIO_MPEG1:
          case STREAM_AUDIO_MPEG2:
          case 0x05:
          case 0x0a:
          {
            Offset               += sizeof(tElemStream);
            if (EycosHeader.Pids[j].Type == 0x0a)
            {
              tTSAC3Desc *Desc0   = (tTSAC3Desc*) &Packet->Data[Offset];
              Desc0->DescrTag     = DESC_AC3;
              Desc0->DescrLength  = 1;
              Desc                = (tTSAudioDesc*) &Packet->Data[Offset + sizeof(tTSAC3Desc)];
              Elem->stream_type   = STREAM_AUDIO_MPEG4_AC3_PLUS;
              Elem->ESInfoLen2    = sizeof(tTSAC3Desc) + sizeof(tTSAudioDesc);
            }
            else
            {
              Desc = (tTSAudioDesc*) &Packet->Data[Offset];
              Elem->stream_type   = STREAM_AUDIO_MPEG2;
              Elem->ESInfoLen2    = sizeof(tTSAudioDesc);
            }
            if (Desc)
            {
              int NrAudio;
              Desc->DescrTag      = DESC_Audio;
              Desc->DescrLength   = 4;

              NrAudio = (int) strlen((char*)EycosHeader.AudioNames) / 2;
              for (k = 0; k < NrAudio; k++)
              {
                if (EycosHeader.AudioNames[k] == EycosHeader.Pids[j].PID)
                {
                  strncpy(Desc->LanguageCode, &((char*)EycosHeader.AudioNames)[NrAudio*2 + 2 + k*4], 3);
                  break;
                }
              }
            }
            printf("    Audio Track %d: PID=%d, Type=0x%x (%s) \n", j, EycosHeader.Pids[j].PID, Elem->stream_type, Desc->LanguageCode);
            
            if (NrContinuityPIDs < MAXCONTINUITYPIDS)
            {
              for (k = 1; k < NrContinuityPIDs; k++)
              {
                if (ContinuityPIDs[k] == EycosHeader.Pids[j].PID)
                  break;
              }
              if (k >= NrContinuityPIDs)
                ContinuityPIDs[NrContinuityPIDs++] = EycosHeader.Pids[j].PID;
            }

            Offset           += Elem->ESInfoLen2;
            PMT->SectionLen2 += (sizeof(tElemStream) + Elem->ESInfoLen2);
            break;
          }

          // Ignorieren
          case 0x09: break;

          // Anderes
          default:
          {
            printf("  Eycos-Import: Unknown elementary stream type 0x%hx (PID=%hu)\n", EycosHeader.Pids[j].Type, EycosHeader.Pids[j].PID);
            break;
          }
        }
      }
      PMT->PCRPID1          = VideoPID / 256;
      PMT->PCRPID2          = VideoPID % 256;
      CRC                   = (dword*) &Packet->Data[Offset];
//     *CRC                  = rocksoft_crc((byte*)PMT, (int)CRC - (int)PMT);        // CRC: 0x0043710d  (0xb3ad75b7?)
//     *CRC                  = crc32m((byte*)PMT, (int)CRC - (int)PMT);              // CRC: 0x0043710d  (0xb3ad75b7?)
      *CRC                  = crc32m_tab((byte*)PMT, (byte*)CRC - (byte*)PMT);     // CRC: 0x0043710d  (0xb3ad75b7?)
      Offset               += 4;
      memset(&Packet->Data[Offset], 0xff, 184 - Offset);
    }
    fclose(fIfo);
  }

  // Txt-Datei laden
  if ((fTxt = fopen(TxtFile, "rb")))
  {
    if ((ret = (ret && fread(&EycosEvent, sizeof(tEycosEvent), 1, fIfo))))
    {
      time_t EvtStartUnix, EvtEndUnix, DisplayTime;
      int NameLen = (int) min(strlen(EycosEvent.Title), sizeof(RecInf->EventInfo.EventNameDescription) - 1);
      int TextLen = sizeof(RecInf->EventInfo.EventNameDescription) - NameLen - 1;

      RecInf->EventInfo.ServiceID = RecInf->ServiceInfo.ServiceID;
      RecInf->EventInfo.EventNameLength = NameLen;
      strncpy(RecInf->EventInfo.EventNameDescription, EycosEvent.Title, NameLen);
      strncpy(&RecInf->EventInfo.EventNameDescription[NameLen], EycosEvent.ShortDesc, TextLen - 1);

      RecInf->ExtEventInfo.ServiceID = RecInf->ServiceInfo.ServiceID;
      TextLen = 0;
      for (k = 0; k < 5; k++)
      {
        char *pCurDesc = ((k > 0) && ((byte)(EycosEvent.LongDesc[k].DescBlock[0]) < 0x20)) ? &EycosEvent.LongDesc[k].DescBlock[1] : EycosEvent.LongDesc[k].DescBlock;
        NameLen = (int) min(strlen(pCurDesc), sizeof(RecInf->ExtEventInfo.Text) - TextLen - 1);
        strncpy(&RecInf->ExtEventInfo.Text[TextLen], pCurDesc, sizeof(RecInf->ExtEventInfo.Text) - TextLen - 1);
        TextLen += NameLen;
        if(NameLen < 248) break;
      }
      RecInf->ExtEventInfo.TextLength = TextLen;
#ifdef _DEBUG
if (strlen(RecInf->ExtEventInfo.Text) != RecInf->ExtEventInfo.TextLength)
  printf("ASSERT: ExtEventTextLength (%d) != length of ExtEventText (%d)!\n", RecInf->ExtEventInfo.TextLength, strlen(RecInf->ExtEventInfo.Text));
#endif
      EvtStartUnix = MakeUnixDate(EycosEvent.EvtStartYear, EycosEvent.EvtStartMonth, EycosEvent.EvtStartDay, EycosEvent.EvtStartHour, EycosEvent.EvtStartMin, 0);
      EvtEndUnix = MakeUnixDate(EycosEvent.EvtEndYear, EycosEvent.EvtEndMonth, EycosEvent.EvtEndDay, EycosEvent.EvtEndHour, EycosEvent.EvtEndMin, 0);
      RecInf->EventInfo.StartTime       = Unix2TFTime(EvtStartUnix, NULL, TRUE);  // DATE(UnixToMJD(EvtStartUnix), EycosEvent.EvtStartHour, EycosEvent.EvtStartMin);
      RecInf->EventInfo.EndTime         = Unix2TFTime(EvtEndUnix, NULL, TRUE);    // DATE(UnixToMJD(EvtEndUnix), EycosEvent.EvtEndHour, EycosEvent.EvtEndMin);
#ifdef _DEBUG
if (HOUR(RecInf->EventInfo.StartTime) != EycosEvent.EvtStartHour || MINUTE(RecInf->EventInfo.StartTime) != EycosEvent.EvtStartMin)
  printf("ASSERT: Eycos Header StartTime (%u:%u) differs from converted time (%u:%u)!\n", EycosEvent.EvtStartHour, EycosEvent.EvtStartMin, HOUR(RecInf->EventInfo.StartTime), MINUTE(RecInf->EventInfo.StartTime));
if (HOUR(RecInf->EventInfo.EndTime) != EycosEvent.EvtEndHour || MINUTE(RecInf->EventInfo.EndTime) != EycosEvent.EvtEndMin)
  printf("ASSERT: Eycos Header EndTime (%u:%u) differs from converted time (%u:%u)!\n", EycosEvent.EvtEndHour, EycosEvent.EvtEndMin, HOUR(RecInf->EventInfo.EndTime), MINUTE(RecInf->EventInfo.EndTime));
#endif
      RecInf->RecHeaderInfo.StartTime   = RecInf->EventInfo.StartTime;
      RecInf->RecHeaderInfo.DurationMin = (word)((EvtEndUnix - EvtStartUnix) / 60);
      RecInf->EventInfo.DurationHour    = RecInf->RecHeaderInfo.DurationMin / 60;
      RecInf->EventInfo.DurationMin     = RecInf->RecHeaderInfo.DurationMin % 60;

      DisplayTime = TF2UnixTime(RecInf->RecHeaderInfo.StartTime, 0, FALSE);
      printf("    Start Time: %s\n", TimeStr(&DisplayTime));
    }
    fclose(fTxt);
  }

  if ((fIdx = fopen(IdxFile, "rb")))
  {
    tEycosIdxEntry EycosIdx;
    size_t ReadOk = fread(&EycosIdx, sizeof(tEycosIdxEntry), 1, fIdx);

    for (j = 1; j < NrSegmentMarker-1 && ReadOk; j++)
    {
      while ((EycosIdx.Timems + 100 < SegmentMarker[j].Timems) && ReadOk)
        ReadOk = fread(&EycosIdx, sizeof(tEycosIdxEntry), 1, fIdx);
      if (ReadOk)
      {
        SegmentMarker[j].Position = EycosIdx.PacketNr * 188;
        BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks++] = (dword)(EycosIdx.PacketNr / 48);
      }
    }
    fclose(fIdx);
  }

//  fseeko64(fIn, FilePos, SEEK_SET);
  if (ret)
    OutCutVersion = 4;
  else
    printf("  Failed to read the Eycos header from rec.\n");
  TRACEEXIT;
  return ret;
}
