#ifndef __CUTPROCESSORH__
#define __CUTPROCESSORH__

#ifdef _MSC_VER
  #define __attribute__(a)
#endif

#include "RecHeader.h"

#pragma pack(push, 1)
typedef struct
{
  byte                  Version;
  unsigned long long    RecFileSize;
  dword                 NrSegmentMarker;
  dword                 ActiveSegment;
}__attribute__((packed)) tCutHeader1;

typedef struct
{
  word                  Version;
  unsigned long long    RecFileSize;
  word                  NrSegmentMarker;
  word                  ActiveSegment;
  word                  Padding;
}__attribute__((packed)) tCutHeader2;
#pragma pack(pop)

extern int              OutCutVersion;
extern bool             WriteCutInf;

void ResetSegmentMarkers(void);
void AddDefaultSegmentMarker(void);
void GetFileNameFromRec(const char *RecFileName, const char *NewExt, char *const OutCutFileName);
void CutImportFromBM(const char* RecFile, dword Bookmarks[], dword NrBookmarks);
//void CutExportToBM(TYPE_Bookmark_Info *BookmarkInfo);
//bool CutImportFromTimeStamps(int Version, byte PacketSize);
bool CutProcessor_Init(void);
bool CutFileLoad(const char *AbsCutName);
bool CutFileSave(const char* AbsCutName);
void CutProcessor_Free(void);

#endif
