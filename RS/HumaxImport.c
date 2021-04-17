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
#include "type.h"
#include "RecStrip.h"
#include "RecHeader.h"
#include "RebuildInf.h"
#include "TtxProcessor.h"
#include "HumaxHeader.h"


static const dword crc_table[] = {
  0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
  0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
  0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
  0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
};

dword crc32m_tab(const unsigned char *buf, size_t len)
{
  dword crc = 0xffffffff;
  while (len--) {
    crc ^= (dword)(*buf++) << 24;
    crc = (crc << 4) ^ crc_table[crc >> 28];
    crc = (crc << 4) ^ crc_table[crc >> 28];
  }

  crc = ((crc & 0x000000ff) << 24 | (crc & 0x0000ff00) << 8 | (crc & 0x00ff0000) >> 8 | (crc & 0xff000000) >> 24);
  return crc;
}

static dword crc32m(const unsigned char *buf, size_t len)
{
  dword crc = 0xffffffff;
  int i;

  while (len--) {
    crc ^= (dword)(*buf++) << 24;
    for (i = 0; i < 8; i++)
      crc = crc & 0x80000000 ? (crc << 1) ^ 0x04c11db7 : crc << 1;
  }

  crc = ((crc & 0x000000ff) << 24 | (crc & 0x0000ff00) << 8 | (crc & 0x00ff0000) >> 8 | (crc & 0xff000000) >> 24);
  return crc;
}

/* static dword rocksoft_crc(const byte data[], int len)
{
  cm_t cm;
  ulong crc;
  int i;

  cm.cm_width = 32;
  cm.cm_poly  = 0x04c11db7;
  cm.cm_init  = 0xffffffff;
  cm.cm_refin = FALSE;
  cm.cm_refot = FALSE;
  cm.cm_xorot = 0;
  cm_ini(&cm);

  for (i = 0; i < len; i++)
    cm_nxt(&cm, data[i]);

  crc = cm_crc(&cm);
  crc = ((crc & 0x000000ff) << 24 | (crc & 0x0000ff00) << 8 | (crc & 0x00ff0000) >> 8 | (crc & 0xff000000) >> 24);
  return crc;
} */

bool SaveHumaxHeader(char *const VidFileName, char *const OutFileName)
{
  FILE                 *fIn, *fOut;
  byte                  HumaxHeader[HumaxHeaderLaenge];
  int                   i;
  bool                  ret = TRUE;

  if ((fIn = fopen(VidFileName, "rb")))
  {
    if ((fOut = fopen(OutFileName, "wb")))
    {
      for (i = 1; i <= 4; i++)
      {
        fseeko64(fIn, (i*HumaxHeaderIntervall) - HumaxHeaderLaenge, SEEK_SET);
        if (fread(&HumaxHeader, HumaxHeaderLaenge, 1, fIn))
        {
          if (*(dword*)HumaxHeader == HumaxHeaderAnfang)
            ret = fwrite(&HumaxHeader, HumaxHeaderLaenge, 1, fOut) && ret;
          else
            ret = FALSE;
        }
        else
          ret = FALSE;
      }
      ret = (fclose(fOut) == 0) && ret;
    }
    fclose(fIn);
  }
  return ret;
}

bool LoadHumaxHeader(FILE *fIn, byte *const PATPMTBuf, TYPE_RecHeader_TMSS *RecInf)
{
  tHumaxHeader          HumaxHeader;
  tTSPacket            *Packet = NULL;
  tTSPAT               *PAT = NULL;
  tTSPMT               *PMT = NULL;
  tElemStream          *Elem = NULL;
  char                  FirstSvcName[32];
  dword                *CRC = NULL;
  int                   Offset = 0;
//  long long             FilePos = ftello64(fIn);
  int                   i, j, k;
  bool                  ret = TRUE;

  TRACEENTER;
  InitInfStruct(RecInf);
  memset(PATPMTBuf, 0, 2*192);

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
  PAT->PMTPID1          = 0;
  PAT->PMTPID2          = 64;
  PAT->Reserved111      = 7;
//  PAT->CRC32            = rocksoft_crc((byte*)PAT, sizeof(tTSPAT)-4);    // CRC: 0x786989a2
//  PAT->CRC32            = crc32m((byte*)PAT, sizeof(tTSPAT)-4);          // CRC: 0x786989a2
  PAT->CRC32            = crc32m_tab((byte*)PAT, sizeof(tTSPAT)-4);      // CRC: 0x786989a2
  
  Offset = 1 + /*Packet->Data[0] +*/ sizeof(tTSPAT);
  memset(&Packet->Data[Offset], 0xff, 184 - Offset);

  Packet = (tTSPacket*) &PATPMTBuf[196];
  PMT = (tTSPMT*) &Packet->Data[1 /*+ Packet->Data[0]*/];

  Packet->SyncByte      = 'G';
  Packet->PID1          = 0;
  Packet->PID2          = 64;
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

  rewind(fIn);
  for (i = 1; ret && (i <= 4); i++)
  {
    fseeko64(fIn, (i*HumaxHeaderIntervall) - HumaxHeaderLaenge, SEEK_SET);
    if ((ret = (ret && fread(&HumaxHeader, sizeof(tHumaxHeader), 1, fIn))))
    {
      ret = ret && (HumaxHeader.Allgemein.Anfang == HumaxHeaderAnfang);
      for (j = 0; ret && (j < 8); j++)
        ret = ret && (HumaxHeader.Allgemein.Anfang2[j] == HumaxHeaderAnfang2);

      if (ret)
      {
        char *p = strrchr(HumaxHeader.Allgemein.Dateiname, '_');

        if (i == 1)  // Header 1: Programm-Information
        {
          time_t DisplayTime;
          printf("  Importing Humax header\n");
          VideoPID                            = HumaxHeader.Allgemein.VideoPID;
          TeletextPID                         = HumaxHeader.Allgemein.TeletextPID;
          RecInf->ServiceInfo.ServiceType     = 0;  // SVC_TYPE_Tv
          RecInf->ServiceInfo.ServiceID       = 1;
          RecInf->ServiceInfo.PMTPID          = 64;
          RecInf->ServiceInfo.VideoPID        = VideoPID;
          RecInf->ServiceInfo.PCRPID          = VideoPID;
          RecInf->ServiceInfo.AudioPID        = HumaxHeader.Allgemein.AudioPID;
          RecInf->ServiceInfo.VideoStreamType = STREAM_VIDEO_MPEG2;
          RecInf->ServiceInfo.AudioStreamType = STREAM_AUDIO_MPEG2;
          RecInf->RecHeaderInfo.StartTime     = DATE(HumaxHeader.Allgemein.Datum, HumaxHeader.Allgemein.Zeit / 60, HumaxHeader.Allgemein.Zeit % 60);
          RecInf->RecHeaderInfo.DurationMin   = (word)(HumaxHeader.Allgemein.Dauer / 60);
          RecInf->RecHeaderInfo.DurationSec   = (word)(HumaxHeader.Allgemein.Dauer % 60);
          ContinuityPIDs[0] = VideoPID;
          printf("    PMTPID=%hu, SID=%hu, PCRPID=%hu, Stream=0x%hhx, VPID=%hu, TtxPID=%hu\n", RecInf->ServiceInfo.PMTPID, RecInf->ServiceInfo.ServiceID, RecInf->ServiceInfo.PCRPID, RecInf->ServiceInfo.VideoStreamType, VideoPID, TeletextPID);

          DisplayTime = TF2UnixTime(RecInf->RecHeaderInfo.StartTime, 0, FALSE);
          printf("    Start Time: %s\n", TimeStr(&DisplayTime));

          if(p) *p = '\0';
          strncpy(FirstSvcName, HumaxHeader.Allgemein.Dateiname, sizeof(FirstSvcName));
          FirstSvcName[sizeof(FirstSvcName)-1] = '\0';
        }
        else if (i == 2)  // Header 2: Original-Dateiname
        {
          printf("    Orig Rec Name: %s\n", HumaxHeader.Allgemein.Dateiname);
          if(p) *p = '\0';
          if (strcmp(HumaxHeader.Allgemein.Dateiname, FirstSvcName) != 0)
          {
            strncpy(RecInf->ServiceInfo.ServiceName, HumaxHeader.Allgemein.Dateiname, sizeof(RecInf->ServiceInfo.ServiceName));
            RecInf->ServiceInfo.ServiceName[sizeof(RecInf->ServiceInfo.ServiceName)-1] = '\0';
          }
        }
        if (HumaxHeader.ZusInfoID == HumaxBookmarksID)  // Header 3: Bookmarks
        {
          tHumaxBlock_Bookmarks* HumaxBookmarks = (tHumaxBlock_Bookmarks*)HumaxHeader.ZusInfos;
          RecInf->BookmarkInfo.NrBookmarks = HumaxBookmarks->Anzahl;
          for (j = 0; j < HumaxBookmarks->Anzahl; j++)
            RecInf->BookmarkInfo.Bookmarks[j] = (dword) ((long long)HumaxBookmarks->Items[j] * 32768 / 9024);
        }
        else if (HumaxHeader.ZusInfoID == HumaxTonSpurenID)  // Header 4: Tonspuren
        {
          tHumaxBlock_Tonspuren* HumaxTonspuren = (tHumaxBlock_Tonspuren*)HumaxHeader.ZusInfos;

          for (j = -1; j <= HumaxTonspuren->Anzahl; j++)
          {
            tTSAudioDesc *Desc    = NULL;

            Elem = (tElemStream*) &Packet->Data[Offset];
            Elem->Reserved1       = 7;
            Elem->Reserved2       = 0xf;
            Offset                += sizeof(tElemStream);

            if (j < 0)
            {
              Elem->stream_type     = STREAM_VIDEO_MPEG2;
              Elem->ESPID1          = VideoPID / 256;
              Elem->ESPID2          = VideoPID % 256;
              Elem->ESInfoLen1      = 0;
              Elem->ESInfoLen2      = 0;
            }
            else if (j < HumaxTonspuren->Anzahl)
            {
              Elem->ESPID1          = HumaxTonspuren->Items[j].PID / 256;
              Elem->ESPID2          = HumaxTonspuren->Items[j].PID % 256;
              Elem->ESInfoLen1      = 0;
              if ((j >= 1) && (strstr(HumaxTonspuren->Items[j].Name, "AC") != NULL || strstr(HumaxTonspuren->Items[j].Name, "ac") != NULL))   // (strstr(HumaxTonspuren->Items[j].Name, "2ch") == 0) && (strstr(HumaxTonspuren->Items[j].Name, "mis") == 0) && (strstr(HumaxTonspuren->Items[j].Name, "fra") == 0)
              {
                tTSAC3Desc *Desc0   = (tTSAC3Desc*) &Packet->Data[Offset];

                if (HumaxTonspuren->Items[j].PID == RecInf->ServiceInfo.AudioPID)
                {
                  RecInf->ServiceInfo.AudioStreamType = STREAM_AUDIO_MPEG4_AC3_PLUS;
                  RecInf->ServiceInfo.AudioTypeFlag = 1;
                }

                Elem->stream_type   = STREAM_AUDIO_MPEG4_AC3_PLUS;  // STREAM_AUDIO_MPEG4_AC3;
                Elem->ESInfoLen2    = sizeof(tTSAC3Desc) + sizeof(tTSAudioDesc);
//                strcpy(&Packet->Data[Offset], "\x05\x04" "AC-3" "\x0A\x04" "deu");
                Desc0->DescrTag     = DESC_AC3;
                Desc0->DescrLength  = 1;
                Desc                = (tTSAudioDesc*) &Packet->Data[Offset + sizeof(tTSAC3Desc)];
              }
              else
              {
                Desc = (tTSAudioDesc*) &Packet->Data[Offset];
                Elem->stream_type   = STREAM_AUDIO_MPEG2;
                Elem->ESInfoLen2    = sizeof(tTSAudioDesc);
//                strcpy(&Packet->Data[Offset], "\x0A\x04" "deu");
              }
              if (Desc)
              {
                Desc->DescrTag      = DESC_Audio;
                Desc->DescrLength   = 4;
                strncpy(Desc->LanguageCode, HumaxTonspuren->Items[j].Name, 3);
              }
              printf("    Audio Track %d: PID=%d, Type=0x%x (%s) \n", j, HumaxTonspuren->Items[j].PID, Elem->stream_type, HumaxTonspuren->Items[j].Name);

              if (NrContinuityPIDs < MAXCONTINUITYPIDS)
              {
                for (k = 1; k < NrContinuityPIDs; k++)
                {
                  if (ContinuityPIDs[k] == HumaxTonspuren->Items[j].PID)
                    break;
                }
                if (k >= NrContinuityPIDs)
                  ContinuityPIDs[NrContinuityPIDs++] = HumaxTonspuren->Items[j].PID;
              }
            }
            else
            {
              tTSTtxDesc *Desc = (tTSTtxDesc*) &Packet->Data[Offset];

              Elem->stream_type     = 6;
              Elem->ESPID1          = HumaxHeader.Allgemein.TeletextPID / 256;
              Elem->ESPID2          = HumaxHeader.Allgemein.TeletextPID % 256;
              Elem->ESInfoLen1      = 0;
              Elem->ESInfoLen2      = 7;

//              strcpy(&Packet->Data[Offset], "V" "\x05" "deu" "\x09");
              Desc->DescrTag        = DESC_Teletext;
              Desc->DescrLength     = 5;
              memcpy(Desc->LanguageCode, "deu", 3); 
              Desc->TtxType         = 1;
              Desc->TtxMagazine     = 1;
            }
            Offset                += Elem->ESInfoLen2;
            PMT->SectionLen2      += sizeof(tElemStream) + Elem->ESInfoLen2;
          }
        }
      }
    }
  }

  PMT->PCRPID1          = VideoPID / 256;
  PMT->PCRPID2          = VideoPID % 256;
  CRC                   = (dword*) &Packet->Data[Offset];
//  *CRC                  = rocksoft_crc((byte*)PMT, (int)CRC - (int)PMT);   // CRC: 0x0043710d  (0xb3ad75b7?)
//  *CRC                  = crc32m((byte*)PMT, (int)CRC - (int)PMT);         // CRC: 0x0043710d  (0xb3ad75b7?)
  *CRC                  = crc32m_tab((byte*)PMT, (byte*)CRC - (byte*)PMT);     // CRC: 0x0043710d  (0xb3ad75b7?)
  Offset               += 4;
  memset(&Packet->Data[Offset], 0xff, 184 - Offset);

  if(NrContinuityPIDs < MAXCONTINUITYPIDS && TeletextPID != 0xffff)  ContinuityPIDs[NrContinuityPIDs++] = TeletextPID;

//  fseeko64(fIn, FilePos, SEEK_SET);
  if (!ret)
    printf("  Failed to read the Humax header from rec.\n");
  TRACEEXIT;
  return ret;
}
