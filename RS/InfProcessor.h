#ifndef __INFPROCESSORH__
#define __INFPROCESSORH__

#include "RecHeader.h"
#include "RecStrip.h"

extern byte                *InfBuffer;
extern TYPE_RecHeader_Info *RecHeaderInfo;
extern tPVRTime             OrigStartTime;  //, CurrentStartTime;
extern byte                 OrigStartSec;   //, CurrentStartSec;

bool InfProcessor_Init(void);
bool LoadInfFromRec(char *AbsRecFileName);
bool LoadInfFile(char *AbsInfName, bool FirstTime);
void SetInfEventText(const char *pCaption);
bool SetInfCryptFlag(const char *AbsInfFile);
bool GetInfStripFlags(const char *AbsInfFile, bool *const OutHasBeenStripped, bool *const OutToBeStripped);
bool SetInfStripFlags(const char *AbsInfFile, bool SetHasBeenScanned, bool ResetToBeStripped, bool SetStartTime);
bool SaveInfFile(const char *AbsDestInf, const char *AbsSourceInf);
void InfProcessor_Free(void);

#endif
