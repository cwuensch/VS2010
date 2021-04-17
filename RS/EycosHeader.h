#ifndef __EYCOSHEADERH__
#define __EYCOSHEADERH__

#ifdef _MSC_VER
  #define __attribute__(a)
#endif

/*
Format des Eycos S80.12HD
=========================
(der Ende des Jahres neu erscheinende S90.12HD soll dasselbe Format verwenden
bei dem vor einigen Monaten erschienenen S70.12HD wurde das Format geringfügig
geändert, weiteres kann ich dazu aber mangels Testgerät nicht sagen)

Die Eycos-Aufnahme besteht aus einer TRP-Datei (=TS, aber ohne PMT/PAT),
einer IFO-Datei, deren Format ich hier beschreibe (viele Informationen,
wie z.B. der Transponder, können beim Erzeugen der Datei auch mit 0 beschrieben werden),
und einer IDX-Datei, die den Index enthält. Hierfür lässt sich auch eine andere
vom Eycos erzeugte Index-Datei oder eine des Vantage verwenden. Wenn die Index-Datei
aber nicht zum Film passt, funktioniert die Fortschrittsanzeige, das Spulen
und das Springen nicht korrekt.

Video-PID: 0x08
Audio-PID: 0xD0
Teletext (o.ä.): 0x18
nochmal Video (oder PCR): 0x20

Müll bis: 0x85
nochmal Video-PID: 0x88

0x92: Sat-Name
0xA2: Sender-Name
0xB6: nochmal Sat
0xC6: Transponder (Frequenz)
0xCC: Symbolrate
0xD4, 0xDA: die beiden letzten als String
0xE0: Polarisierung ('H' oder 'V')
0xE2: FEC (als Text)
0xEC: unbekannt
0x100: unbekannt
0x114: unbekannt
0x11C: unbekannt (Konstante 'U#')
0x12A: nochmal Sat (Text)
0x13B: nochmal Sender (Text)
0x14E: nochmal Sat (Text)

0x154: Marken-Anzahl
ab 0x158: Marken (jeweils ein Long)

0x1E4: Tonspur-Info
*/

#include "type.h"

#pragma pack(push, 1)
typedef struct
{
  dword                 Type;  // 0x0b = Video, 0x04 / 0x05 / 0x0a = Audio, 0x09 = PCR?
  dword                 PID;
} tEycosPid;

typedef struct
{
  word                  PID;
  char                  Name[4];                 // nullterminiert
}__attribute__((packed)) tEycosAudio;

typedef struct
{
  int                   NrPids;        // unbekannt?
  tEycosPid             Pids[16];      // 1. Video, 2. + 3. Audio, 4. PCR?
  word                  ConstantU;     // "U#"
  word                  ServiceID;
  word                  VideoPid;
  word                  PMTPid;        // ??
  word                  AudioPid;
  word                  NullPid1;
  word                  NullPid2;
  char                  SatelliteName[16];
  char                  SenderName[20];
  char                  SatelliteName2[16];
  word                  Frequency;
  dword                 Unknown2;
  word                  SymbolRate;
  word                  ConstantT;      // "T#"
  word                  FEC;            // ??  0x0d = 3/4
  word                  Polarization;   // ??  1 = horizontal?
  char                  ServiceStr[26]; // wie lang?
  byte                  Unknown3[46];
  word                  ConstantU2;     // "U#"
  byte                  Unknown4[12];
  char                  SatelliteName3[16];
  char                  SenderName2[20];
  char                  Group[6];       // ?
  int                   NrBookmarks;    // bei (geschnittenen?) Aufnahmen oft falsch gesetzt!
  dword                 Bookmarks[32];
  byte                  Unknown5[12];
  word                  ConstantT2;     // "T#"
  word                  AudioNames[32];
}__attribute__((packed)) tEycosHeader;

typedef struct
{
  char                  DescBlock[250];  // auch Sub-Blöcke sind null-terminiert
  char                  Unknown[6];
}__attribute__((packed)) tEycosDescBlock;

typedef struct
{
  byte                  Unknown1[6];
  word                  ServiceID;
  byte                  Unknown2[12];
  word                  EvtStartYear;
  byte                  EvtStartMonth;
  byte                  EvtStartDay;
  byte                  EvtStartHour;
  byte                  EvtStartMin;
  word                  Unused1;
  word                  EvtEndYear;
  byte                  EvtEndMonth;
  byte                  EvtEndDay;
  byte                  EvtEndHour;
  byte                  EvtEndMin;
  byte                  Unknown3[190];
  char                  Title[64];
  char                  ShortDesc[512];
  tEycosDescBlock       LongDesc[8];
}__attribute__((packed)) tEycosEvent;

typedef struct
{
  dword                 Timems;    // in ca. Hunderter-Schritten
  dword                 Unused1;
  long long             PacketNr;  // in 188 Byte Packets
} tEycosIdxEntry;
#pragma pack(pop)

char* EycosGetPart(char *const OutEycosPart, const char* AbsTrpName, int NrPart);
int   EycosGetNrParts(const char* AbsTrpName);
bool  LoadEycosHeader(char *AbsTrpFileName, byte *const PATPMTBuf, TYPE_RecHeader_TMSS *RecInf);

#endif
