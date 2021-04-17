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
#include "CutProcessor.h"
#include "RecStrip.h"
#include "NavProcessor.h"

typedef struct
{
  dword                 Block;
  dword                 Timems; //Time in ms
  float                 Percent;
  int                   Selected;
  char                 *pCaption;
} tSegmentMarker1;


// Globale Variablen
int                     OutCutVersion = 3;
static const bool       WriteCutFile = TRUE;
bool                    WriteCutInf = FALSE;


// ----------------------------------------------
// *****  READ AND WRITE CUT FILE  *****
// ----------------------------------------------

static void MSecToTimeString(dword Timems, char *const OutTimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
{
  dword                 Hour, Min, Sec, Millisec;

  TRACEENTER;
  if(OutTimeString)
  {
    Hour = (Timems / 3600000);
    Min  = (Timems / 60000) % 60;
    Sec  = (Timems / 1000) % 60;
    Millisec = Timems % 1000;
    snprintf(OutTimeString, 15, "%u:%02u:%02u,%03u", Hour, Min, Sec, Millisec);
  }
  TRACEEXIT;
}

static dword TimeStringToMSec(char *const TimeString)
{
  dword                 Hour=0, Min=0, Sec=0, Millisec=0, ret=0;

  TRACEENTER;
  if(TimeString)
  {
    if (sscanf(TimeString, "%u:%u:%u%*1[.,]%u", &Hour, &Min, &Sec, &Millisec) == 4)
      ret = 1000*(60*(60*Hour + Min) + Sec) + Millisec;
  }
  TRACEEXIT;
  return ret;
}

void ResetSegmentMarkers(void)
{
  int i;
  TRACEENTER;

  for (i = 0; i < NrSegmentMarker; i++)
    if (SegmentMarker[i].pCaption)
      free(SegmentMarker[i].pCaption);
  memset(SegmentMarker, 0, NRSEGMENTMARKER * sizeof(tSegmentMarker2));
  NrSegmentMarker = 0;

  TRACEEXIT;
}

void AddDefaultSegmentMarker(void)
{
  TRACEENTER;

  ResetSegmentMarkers();
//  SegmentMarker[0].Selected = TRUE;
  SegmentMarker[1].Position = RecFileSize;
  SegmentMarker[1].Timems   = InfDuration * 1000;
  SegmentMarker[1].Percent  = 100.0;
  NrSegmentMarker = 2;

  TRACEEXIT;
}

void GetFileNameFromRec(const char *RecFileName, const char *NewExt, char *const OutCutFileName)
{
  char *p = NULL;

  TRACEENTER;
  if (RecFileName && OutCutFileName)
  {
    snprintf(OutCutFileName, FBLIB_DIR_SIZE, "%s", RecFileName);
    if ((p = strrchr(OutCutFileName, '.')) == NULL)
      p = &OutCutFileName[strlen(OutCutFileName)];
    strcpy(p, NewExt);
  }
  TRACEEXIT;
}

static bool CutFileDecodeBin(FILE *fCut, unsigned long long *OutSavedSize)
{
  byte                  Version;
  int                   SavedNrSegments = 0;
  bool                  ret = FALSE;

  TRACEENTER;

  ResetSegmentMarkers();
  ActiveSegment = 0;
  if (OutSavedSize) *OutSavedSize = 0;

  if (fCut)
  {
    // Check correct version of cut-file
    Version = fgetc(fCut);
    switch (Version)
    {
      case 1:
      {
        tCutHeader1 CutHeader;
        rewind(fCut);
        ret = (fread(&CutHeader, sizeof(CutHeader), 1, fCut) == 1);
        if (ret)
        {
          if (OutSavedSize) *OutSavedSize = CutHeader.RecFileSize;
          SavedNrSegments = CutHeader.NrSegmentMarker;
          ActiveSegment = CutHeader.ActiveSegment;
        }
        break;
      }
      case 2:
      {
        tCutHeader2 CutHeader;
        rewind(fCut);
        ret = (fread(&CutHeader, sizeof(CutHeader), 1, fCut) == 1);
        if (ret)
        {
          if (OutSavedSize) *OutSavedSize = CutHeader.RecFileSize;
          SavedNrSegments = CutHeader.NrSegmentMarker;
          ActiveSegment = CutHeader.ActiveSegment;
        }
        break;
      }
      default:
        printf("CutFileDecodeBin: .cut version mismatch!\n");
    }

    if (ret)
    {
      tSegmentMarker1   curSeg;

      SavedNrSegments = min(SavedNrSegments, NRSEGMENTMARKER);
      while (fread(&curSeg, sizeof(tSegmentMarker1)-sizeof(char*), 1, fCut))
      {
        SegmentMarker[NrSegmentMarker].Position = curSeg.Block * 9024LL;
        SegmentMarker[NrSegmentMarker].Timems   = curSeg.Timems;
        SegmentMarker[NrSegmentMarker].Percent  = curSeg.Percent;
        SegmentMarker[NrSegmentMarker].Selected = curSeg.Selected;
        SegmentMarker[NrSegmentMarker].pCaption = NULL;
        NrSegmentMarker++;
      }
      if (NrSegmentMarker < SavedNrSegments)
        printf("CutFileDecodeBin: Unexpected end of file!\n");
    }
  }
  TRACEEXIT;
  return ret;
}

static bool CutFileDecodeTxt(FILE *fCut, unsigned long long *OutSavedSize)
{
  char                  Buffer[4096];
  long long             SavedSize = -1;
  int                   Version = 3;
  int                   SavedNrSegments = -1;
  bool                  HeaderMode=FALSE, SegmentsMode=FALSE;
  char                  TimeStamp[16];
  char                 *c, Selected;
  int                   p, ReadBytes;
  bool                  ret = FALSE;

  TRACEENTER;

  ResetSegmentMarkers();
  ActiveSegment = 0;
  if (OutSavedSize) *OutSavedSize = 0;

  if (fCut)
  {
    // Check the first line
    if (fgets(Buffer, sizeof(Buffer), fCut))
    {
      if ((strncmp(Buffer, "[MCCut3]", 8)==0) || ((strncmp(Buffer, "[MCCut4]", 8)==0) && ((Version = 4))))
      {
        HeaderMode = TRUE;
        ret = TRUE;
      }
    }

    while (ret && (fgets(Buffer, sizeof(Buffer), fCut)))
    {
      //Interpret the following characters as remarks: //
      c = strstr(Buffer, "//");
      if(c) *c = '\0';

      // Remove line breaks in the end
      p = (int)strlen(Buffer);
      while (p && (Buffer[p-1] == '\r' || Buffer[p-1] == '\n' || Buffer[p-1] == ';'))
        Buffer[--p] = '\0';

      // Kommentare und Sektionen
      switch (Buffer[0])
      {
        case '\0':
          continue;

        case '%':
        case ';':
        case '#':
        case '/':
          continue;

        // Neue Sektion gefunden
        case '[':
        {
          HeaderMode = FALSE;
          // Header überprüfen
          if ((SavedSize < 0) || (SavedNrSegments < 0))
          {
            ret = FALSE;
            break;
          }
          if (strcmp(Buffer, "[Segments]") == 0)
            SegmentsMode = TRUE;
          continue;
        }
      }

      // Header einlesen
      if (HeaderMode)
      {
        char            Name[50];
        long long       Value;

        if (sscanf(Buffer, "%49[^= ] = %lld", Name, &Value) == 2)
        {
          if (strcmp(Name, "RecFileSize") == 0)
          {
            SavedSize = Value;
            if (OutSavedSize) *OutSavedSize = SavedSize;
          }
          else if (strcmp(Name, "NrSegmentMarker") == 0)
            SavedNrSegments = (int)Value;
          else if (strcmp(Name, "ActiveSegment") == 0)
            ActiveSegment = (int)Value;
        }
      }

      // Segmente einlesen
      else if (SegmentsMode)
      {
        //[Segments]
        //#Nr. ; Sel ; StartPosition ; StartTime ; Percent
        if (sscanf(Buffer, "%*i ; %c ; %lld ; %15[^;\r\n] ; %f%%%n", &Selected, &SegmentMarker[NrSegmentMarker].Position, TimeStamp, &SegmentMarker[NrSegmentMarker].Percent, &ReadBytes) >= 3)
        {
          if (Version <= 3)
            SegmentMarker[NrSegmentMarker].Position = (SegmentMarker[NrSegmentMarker].Position * 9024);
          SegmentMarker[NrSegmentMarker].Selected = (Selected == '*');
          SegmentMarker[NrSegmentMarker].Timems   = (TimeStringToMSec(TimeStamp));
          SegmentMarker[NrSegmentMarker].pCaption = NULL;
          while (Buffer[ReadBytes] && (Buffer[ReadBytes] == ' ' || Buffer[ReadBytes] == ';'))  ReadBytes++;
          if (Buffer[ReadBytes])
          {
            SegmentMarker[NrSegmentMarker].pCaption = (char*)malloc(strlen(&Buffer[ReadBytes]) + 1);
            strcpy(SegmentMarker[NrSegmentMarker].pCaption, &Buffer[ReadBytes]);
          }
          NrSegmentMarker++;
        }
      }
    }

    if (ret)
    {
      OutCutVersion = Version;
      if (NrSegmentMarker != SavedNrSegments)
        printf("CutFileDecodeTxt: Invalid number of segments read (%d of %d)!\n", NrSegmentMarker, SavedNrSegments);
    }
    else
      printf("CutFileDecodeTxt: Invalid cut file format!\n");
  }
  TRACEEXIT;
  return ret;
}

static bool CutDecodeFromBM(dword Bookmarks[])
{
  int                   End = 0, Start, i;
  bool                  ret = FALSE;

  TRACEENTER;

  ResetSegmentMarkers();
  ActiveSegment = 0;
  if (Bookmarks[NRBOOKMARKS - 2] == 0x8E0A4247)       // ID im vorletzen Bookmark-Dword (-> neues SRP-Format und CRP-Format auf SRP)
    End = NRBOOKMARKS - 2;
  else if (Bookmarks[NRBOOKMARKS - 1] == 0x8E0A4247)  // ID im letzten Bookmark-Dword (-> CRP- und altes SRP-Format)
    End = NRBOOKMARKS - 1;

  if(End)
  {
    int NrTimeStamps;
    tTimeStamp2 *TimeStamps = NavLoad(RecFileIn, &NrTimeStamps, PACKETSIZE);

    ret = TRUE;
    NrSegmentMarker = Bookmarks[End - 1];
    if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;

    Start = End - NrSegmentMarker - 5;
    for (i = 0; i < NrSegmentMarker; i++)
    {
      SegmentMarker[i].Position = Bookmarks[Start + i] * 9024LL;
      SegmentMarker[i].Selected = ((Bookmarks[End-5+(i/32)] & (1 << (i%32))) != 0);
      SegmentMarker[i].Timems = (TimeStamps) ? NavGetPosTimeStamp(TimeStamps, NrTimeStamps, SegmentMarker[i].Position) : 0;
      SegmentMarker[i].Percent = 0;
      SegmentMarker[i].pCaption = NULL;
      if ((i == 0) && (SegmentMarker[0].Position == 0) && (SegmentMarker[0].Timems < 1000))
        SegmentMarker[0].Timems = 0;
    }
    if(TimeStamps) free(TimeStamps);
  }

  TRACEEXIT;
  return ret;
}

void CutImportFromBM(const char *RecFile, dword Bookmarks[], dword NrBookmarks)
{
  int                   i;
  TRACEENTER;

  ResetSegmentMarkers();
  ActiveSegment = 0;

  if(NrBookmarks > 0)
  {
    int NrTimeStamps;
    tTimeStamp2 *TimeStamps = NavLoad(RecFile, &NrTimeStamps, PACKETSIZE);

    NrSegmentMarker = NrBookmarks + 2;
    if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;

    for (i = 1; i < NrSegmentMarker-1; i++)
    {
      SegmentMarker[i].Position = Bookmarks[i-1] * 9024LL;
      SegmentMarker[i].Selected = FALSE;
      SegmentMarker[i].Timems = (TimeStamps) ? NavGetPosTimeStamp(TimeStamps, NrTimeStamps, SegmentMarker[i].Position) : 0;
      SegmentMarker[i].Percent = 0;
      SegmentMarker[i].pCaption = NULL;
    }
    SegmentMarker[NrSegmentMarker-1].Position = TimeStamps[NrTimeStamps-1].Position;
    SegmentMarker[NrSegmentMarker-1].Timems = TimeStamps[NrTimeStamps-1].Timems;

    if(TimeStamps) free(TimeStamps);
  }
  TRACEEXIT;
}

/*void CutExportToBM(TYPE_Bookmark_Info *BookmarkInfo)
{
  int                   i;
  TRACEENTER;

  if (BookmarkInfo && NrSegmentMarker > 2)
  {
    // first, delete all present bookmarks
    while (BookmarkInfo->NrBookmarks > 0)
      BookmarkInfo->Bookmarks[--BookmarkInfo->NrBookmarks] = 0;

    // second, add a bookmark for each SegmentMarker
    for(i = 1; i <= min(NrSegmentMarker-2, 48); i++)
      BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks++] = CalcBlockSize(SegmentMarker[i].Position);
  }
  TRACEEXIT;
}*/

static bool CutImportFromTimeStamps(int Version, byte PacketSize)
{
  tTimeStamp2      *TimeStamps = NULL;
  int               NrTimeStamps, i;
  bool              ret = FALSE;

  TRACEENTER;

  // Sonderfunktion: Import von Cut-Files mit unpassender Aufnahme-Größe
  if (NrSegmentMarker > 2)
    TimeStamps = NavLoad(RecFileOut, &NrTimeStamps, PacketSize);

  if (TimeStamps != NULL)
  {
    char            curTimeStr[16];
    dword           Offsetms;
    tTimeStamp2    *CurTimeStamp;

    printf("  CutFileLoad: Importing timestamps only, recalculating block numbers...\n");
    SegmentMarker[0].Position = 0;
    SegmentMarker[0].Timems = 0;  // NavGetBlockTimeStamp(0);
//        SegmentMarker[0].Selected = FALSE;
    if (SegmentMarker[NrSegmentMarker-1].Position == (long long)RecFileSize)
      SegmentMarker[NrSegmentMarker - 1].Position = 0;

    Offsetms = 0;
    CurTimeStamp = TimeStamps;
    for (i = 1; i <= NrSegmentMarker-2; i++)
    {
      if (Version == 1)
        SegmentMarker[i].Timems = SegmentMarker[i].Timems * 1000;

      // Wenn ein Bookmark gesetzt ist, dann verschiebe die SegmentMarker so, dass der erste auf dem Bookmark steht
      if (i == 1 && BookmarkInfo && BookmarkInfo->NrBookmarks > 0)
      {
        Offsetms = NavGetPosTimeStamp(TimeStamps, NrTimeStamps, BookmarkInfo->Bookmarks[0] * 9024LL) - SegmentMarker[1].Timems;
        MSecToTimeString(SegmentMarker[i].Timems + Offsetms, curTimeStr);
        printf("  Bookmark found! - First segment marker will be moved to time %s. (Offset=%d ms)\n", curTimeStr, (int)Offsetms);
      }

      MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
      printf("  %2u.)  oldTimeStamp=%s   oldPos=%lld", i, curTimeStr, SegmentMarker[i].Position);
      if (Offsetms > 0)
      {
        SegmentMarker[i].Timems += Offsetms;
        MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
        printf("  -  movedTimeStamp=%s\n", curTimeStr);
      }

      if ((SegmentMarker[i].Timems <= CurTimeStamp->Timems) || (CurTimeStamp >= TimeStamps + NrTimeStamps-1))
      {
        DeleteSegmentMarker(i--, TRUE);
        printf("  -->  Smaller than previous TimeStamp or end of nav reached. Deleted!\n");
      }
      else
      {
        while ((CurTimeStamp->Timems < SegmentMarker[i].Timems) && (CurTimeStamp < TimeStamps + NrTimeStamps-1))
          CurTimeStamp++;
        if (CurTimeStamp->Timems > SegmentMarker[i].Timems)
          CurTimeStamp--;
        
        if (CurTimeStamp->Position < (long long)RecFileSize)
        {
          SegmentMarker[i].Position = CurTimeStamp->Position;
          SegmentMarker[i].Timems = CurTimeStamp->Timems;  // NavGetPosTimeStamp(TimeStamps, NrTimeStamps, SegmentMarker[i].Position);
//              SegmentMarker[i].Selected = FALSE;
          MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
          printf("  -->  newPos=%lld   newTimeStamp=%s\n", SegmentMarker[i].Position, curTimeStr);
        }
        else
        {
          DeleteSegmentMarker(i--, TRUE);
          printf("  -->  TotalBlocks exceeded. Deleted!\n");
        }
      }
    }
    free(TimeStamps);
    ret = TRUE;
  }
  TRACEEXIT;
  return ret;
}

bool CutProcessor_Init(void)
{
  TRACEENTER;

  // Puffer allozieren
  SegmentMarker = (tSegmentMarker2*) malloc(NRSEGMENTMARKER * sizeof(tSegmentMarker2));
  if (SegmentMarker)
  {
    memset(SegmentMarker, 0, NRSEGMENTMARKER * sizeof(tSegmentMarker2));
    TRACEEXIT;
    return TRUE;
  }
  else
  {
    free(SegmentMarker); SegmentMarker = NULL;
    printf("CutFileLoad: Failed to allocate memory!\n");
    TRACEEXIT;
    return FALSE;
  }
}

bool CutFileLoad(const char *AbsCutName)
{
  FILE                 *fCut = NULL;
  byte                  Version;
  unsigned long long    SavedSize;
  int                   i;
  bool                  ret = FALSE;

  TRACEENTER;

  if (!SegmentMarker)
  {
    TRACEEXIT;
    return FALSE;
  }

  // Schaue zuerst im Cut-File nach
  fCut = fopen(AbsCutName, "rb");
  if(fCut)
  {
    Version = fgetc(fCut);
    if (Version == '[')
    {
      fseek(fCut, 6, SEEK_SET);
      Version = fgetc(fCut) - '0';
    }
    rewind(fCut);
    printf("  CutFileLoad: Importing cut-file version %hhu\n", Version);

    switch (Version)
    {
      case 1:
      case 2:
      {
        printf("  CutFileLoad: Binary .cut versions are deprecated!\n");
        ret = CutFileDecodeBin(fCut, &SavedSize);
        break;
      }
      case 3:
      case 4:
      {
        ret = CutFileDecodeTxt(fCut, &SavedSize);
        break;
      }
    }
    fclose(fCut);
    if (!ret)
      printf("  CutFileLoad: Failed to read cut-info from .cut!\n");


    // Check, if size of rec-File has been changed
    if (ret && (RecFileSize != SavedSize))
    {
      printf("  CutFileLoad: .cut file size mismatch!\n");

      // Sonderfunktion: Import von Cut-Files mit unpassender Aufnahme-Größe
      if (!CutImportFromTimeStamps(Version, PACKETSIZE))
        ResetSegmentMarkers();
    }
  }


  // sonst schaue in der inf
  if (!ret && BookmarkInfo)
  {
    ret = CutDecodeFromBM(BookmarkInfo->Bookmarks);
    if (ret)
    {
      WriteCutInf = TRUE;
      printf("  CutFileLoad: Imported segments from Bookmark-area.\n");
    }
  }
  else if (BookmarkInfo)
  {
    if ((BookmarkInfo->Bookmarks[NRBOOKMARKS-2] == 0x8E0A4247) || (BookmarkInfo->Bookmarks[NRBOOKMARKS-1] == 0x8E0A4247))
      WriteCutInf = TRUE;
  }


  if (ret && NrSegmentMarker > 0)
  {
    // erstes Segment auf 0 setzen?
    if (SegmentMarker[0].Position != 0)
    {
      SegmentMarker[0].Position = 0;
      SegmentMarker[0].Timems = 0;
    }
    
    // Wenn letzter Segment-Marker ungleich TotalBlock ist -> anpassen
    if (SegmentMarker[NrSegmentMarker - 1].Position != (long long)RecFileSize)
    {
      SegmentMarker[NrSegmentMarker - 1].Position = RecFileSize;
      if (!SegmentMarker[NrSegmentMarker - 1].Timems)
        SegmentMarker[NrSegmentMarker - 1].Timems = InfDuration * 1000;
    }

    // Prozent-Angaben neu berechnen (müssen künftig nicht mehr in der .cut gespeichert werden)
    for (i = NrSegmentMarker-1; i >= 0; i--)
    {
      if ((i < NrSegmentMarker-1) && (SegmentMarker[i].Position >= (long long)RecFileSize))
      {
        printf("  SegmentMarker %d (%lld): TotalBlocks exceeded. -> Deleting!\n", i, SegmentMarker[i].Position);
        DeleteSegmentMarker(i, TRUE);
      }
      else
        SegmentMarker[i].Percent = (float)(((float)SegmentMarker[i].Position / RecFileSize) * 100.0);
    }

    // Markierungen und ActiveSegment prüfen, ggf. korrigieren
    SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;         // the very last marker (no segment)
  }

  // Wenn zu wenig Segmente -> auf Standard zurücksetzen
  if (NrSegmentMarker < 2)
  {
    if(ret) printf("  CutFileLoad: Less than two timestamps imported -> resetting!\n"); 
    ResetSegmentMarkers();
//    free(SegmentMarker);
//    SegmentMarker = NULL;
    ret = FALSE;
  }

  TRACEEXIT;
  return ret;
}

static bool CutEncodeToBM(dword Bookmarks[], int NrBookmarks)
{
  int                   End, Start, i;
  bool                  ret = TRUE;

  TRACEENTER;
  if (!Bookmarks)
  {
    TRACEEXIT;
    return FALSE;
  }
  memset(&Bookmarks[NrBookmarks], 0, (NRBOOKMARKS - min(NrBookmarks, NRBOOKMARKS)) * sizeof(dword));

  if (SegmentMarker && (NrSegmentMarker > 2))
  {
    End   = (SystemType == ST_TMSC) ? NRBOOKMARKS-1 : NRBOOKMARKS-2;  // neu: auf dem SRP das vorletzte Dword nehmen -> CRP-kompatibel
    Start = End - NrSegmentMarker - 5;
    if (Start >= NrBookmarks)
    {
      Bookmarks[End]   = 0x8E0A4247;  // Magic
      Bookmarks[End-1] = NrSegmentMarker;
      for (i = 0; i < NrSegmentMarker; i++)
      {
        Bookmarks[Start+i] = (dword) (SegmentMarker[i].Position / 9024);
        Bookmarks[End-5+(i/32)] = (Bookmarks[End-5+(i/32)] & ~(1 << (i%32))) | (SegmentMarker[i].Selected ? 1 << (i%32) : 0);
      }
    }
    else
    {
      printf("CutEncodeToBM: Error! Not enough space to store segment markers. (NrSegmentMarker=%d, NrBookmarks=%d)\n", NrSegmentMarker, NrBookmarks);
      ret = FALSE;
    }
  }

  TRACEEXIT;
  return ret;
}

bool CutFileSave(const char* AbsCutName)
{
  FILE                 *fCut = NULL;
  char                  TimeStamp[16];
  unsigned long long    RecFileSize;
  int                   i;
  bool                  ret = TRUE;

  TRACEENTER;

  if (SegmentMarker)
  {
    if (WriteCutFile && AbsCutName && *AbsCutName && (NrSegmentMarker > 2 || SegmentMarker[0].pCaption))
    {
      // neues CutFile speichern
      if (!HDD_GetFileSize(RecFileOut, &RecFileSize))
        printf("CutFileSave: Could not detect size of recording!\n"); 

      fCut = fopen(AbsCutName, "wb");
      if(fCut)
      {
        ret = (fprintf(fCut, "[MCCut%c]\r\n", OutCutVersion + '0') > 0) && ret;
        ret = (fprintf(fCut, "RecFileSize=%llu\r\n", RecFileSize) > 0) && ret;
        ret = (fprintf(fCut, "NrSegmentMarker=%d\r\n", NrSegmentMarker) > 0) && ret;
        ret = (fprintf(fCut, "ActiveSegment=%d\r\n\r\n", ActiveSegment) > 0) && ret;  // sicher!?
        ret = (fprintf(fCut, "[Segments]\r\n") > 0) && ret;
        ret = (fprintf(fCut, "#Nr ; Sel ; %s ;     StartTime ; Percent ; Caption\r\n", ((OutCutVersion >= 4) ? "StartPosition" : "StartBlock")) > 0) && ret;
        for (i = 0; i < NrSegmentMarker; i++)
        {
          SegmentMarker[i].Percent = (float)(((float)SegmentMarker[i].Position / RecFileSize) * 100.0);
          MSecToTimeString(SegmentMarker[i].Timems, TimeStamp);
          if (OutCutVersion >= 4)
            ret = (fprintf(fCut, "%3d ;  %c  ; %13lld ;%14s ;  %5.1f%% ; %s\r\n", i, (SegmentMarker[i].Selected ? '*' : '-'), SegmentMarker[i].Position, TimeStamp, SegmentMarker[i].Percent, (SegmentMarker[i].pCaption ? SegmentMarker[i].pCaption : "")) > 0) && ret;
          else
            ret = (fprintf(fCut, "%3d ;  %c  ; %10u ;%14s ;  %5.1f%% ; %s\r\n", i, (SegmentMarker[i].Selected ? '*' : '-'), (dword)(SegmentMarker[i].Position/9024), TimeStamp, SegmentMarker[i].Percent, (SegmentMarker[i].pCaption ? SegmentMarker[i].pCaption : "")) > 0) && ret;
        }
        ret = (fclose(fCut) == 0) && ret;
//        HDD_SetFileDateTime(&AbsCutName[1], "", 0);
      }
      else
      {
        printf("CutFileSave: failed to open .cut!\n");
        ret = FALSE;
      }
    }
    else
      remove(AbsCutName);

    if (WriteCutInf && BookmarkInfo)
      CutEncodeToBM(BookmarkInfo->Bookmarks, BookmarkInfo->NrBookmarks);
  }
  TRACEEXIT;
  return ret;
}

void CutProcessor_Free(void)
{
  TRACEEXIT;
  if (SegmentMarker)
  {
    ResetSegmentMarkers();
    free(SegmentMarker);  SegmentMarker = NULL;
  }
  TRACEEXIT;
}
