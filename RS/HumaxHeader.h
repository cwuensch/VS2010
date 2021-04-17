#ifndef __HUMAXHEADERH__
#define __HUMAXHEADERH__

#ifdef _MSC_VER
  #define __attribute__(a)
#endif

/*
 Das Format
 ----------
 - Humax-Header (u.a. Dateiname) befinden sich am Ende jedes 32768 Byte (0x8000) Blocks
   und sind jeweils 1184 Bytes lang.
     
 - Der erste Header beginnt also bei 0x7B60, der zweite bei 0xFB60, usw.
 - Es gibt 3 Sorten von Headern:
   1. Header-Block: Allgemeiner Header THumaxHeader (enthält z.B. die PIDs, Datum und Zeit der Aufnahme usw.)
   3. Header-Block: der Bookmark-Header THumaxHeader_Bookmarks
   4. Header-Block: der Tonspur-Header THumaxHeader_Tonspuren
   alle anderen Header-Blöcke: können mit dem allgemeinen Header beschrieben werden

 - Ich habe diese 3 Header-Typen aufgeteilt in folgende Blöcke:
   (a) THumaxBlock_allg           -> allgemeiner Block (PIDs, Datum, Zeit, Dateiname, usw.)
                                     der Teil ist in allen Headern gleich
                                     Beachte: bei Änderung des Dateinamens oder des Schutzes wird nur der 1. Header aktualisiert! In den anderen bleibt die alte Info stehen.
   (b) ZusInfoID As Integer       -> gibt an, um welche Art von Header es sich handelt
   (c) ZusInfos(1 To 404) As Byte -> das ist der zusätzliche Info-Block, der sich je nach Header-Typ unterscheidet
                                     da stehen dann z.B. die Bookmarks oder Tonspurinfos drin
   (d) THumaxBlock_Ende           -> in diesem Teil stehen keine relevanten Informationen mehr
*/

#include "type.h"

#define HumaxHeaderIntervall    0x8000           // nach diesem Intervall wird der Header wiederholt
#define HumaxHeaderLaenge       0x4A0            // die Länge eines jeden Headers
#define HumaxHeaderAnfang       0xFD04417F       // das erste DWORD eines jeden Headers
#define HumaxHeaderAnfang2      0x78123456       // die DWORDs 2-9 eines jeden Headers

typedef enum
{
  HumaxBookmarksID = 0x5514,
  HumaxTonSpurenID = 0x4823
} tHumaxZusInfoIDs;

#pragma pack(push, 1)
typedef struct
{
  word                  PID;
  char                  Name[6];                 // meist kürzer, nullterminiert
}__attribute__((packed)) tHumaxTonSpur;

typedef struct
{
  dword Anfang;              // dient zur Identifikation des Headers (?)
  dword Anfang2[8];          // 8 Mal Wiederholung von "V4.x"
  word VideoPID;
  word AudioPID;
  word TeletextPID;
  byte Leer1[10];            // 10 Bytes leer
  dword Konstante;           // scheint immer 0x40201 zu sein
  byte Leer2[8];             // 8 Bytes meist(?) leer
  dword Unbekannt1;
  dword Datum;               // Anzahl der Tage seit 17.11.1858 -> Unix = (MJD-40587)*86400
  dword Zeit;                // Anzahl der Minuten seit 00:00
  dword Dauer;               // Aufnahmedauer in Sekunden
  byte Schreibschutz;        // 1 (geschützt) - 0 (nicht geschützt)
  byte Unbekannt2[15];
  char Dateiname[32];        // evtl. kürzer, nullterminiert
}__attribute__((packed)) tHumaxBlock_allg;

typedef struct
{
  byte Unbekannt3[620];
  byte Ende[30];             // vielleicht zur Markierung des Endes (-> eher nicht!)
} tHumaxBlock_Ende;


typedef struct
{
  word Anzahl;               // (vermutlich ist die Anzahl ein Long, aber zur Sicherheit...)
  word Leer;
  dword Items[100];
} tHumaxBlock_Bookmarks;

typedef struct
{
  word Anzahl;
  word Leer;
  tHumaxTonSpur Items[50];
}__attribute__((packed)) tHumaxBlock_Tonspuren;


typedef struct
{
  tHumaxBlock_allg Allgemein;  // allgemeiner Block (Dateiname und Schreibschutz nur im 1. Header aktuell!)
  word ZusInfoID;              // ID des ZusatzInfo-Blocks
  byte ZusInfos[404];          // z.B. 3. Header: Bookmarks, 4. Header: Tonspuren
  tHumaxBlock_Ende Ende;
}__attribute__((packed)) tHumaxHeader;
#pragma pack(pop)


typedef struct
{
  byte TableID;
  byte SectionLen1:4;    // first 2 bits are 0
  byte Reserved1:2;      // = 0x03 (all 1)
  byte Private:1;        // = 0
  byte SectionSyntax:1;  // = 1
  byte SectionLen2;
  byte TS_ID1;
  byte TS_ID2;
  byte CurNextInd:1;
  byte VersionNr:5;
  byte Reserved2:2;      // = 0x03 (all 1)
  byte SectionNr;
  byte LastSection;

//  for i = 0 to N  {
    word ProgramNr1:8;
    word ProgramNr2:8;
    word PMTPID1:5;  // oder NetworkPID, falls ProgramNr==0
    word Reserved111:3;
    word PMTPID2:8;  // oder NetworkPID, falls ProgramNr==0
//  }
  dword CRC32;
} tTSPAT;


typedef struct
{
  byte DescrTag;
  byte DescrLength;
//  char Name[4];          // without terminating 0
  byte Reserved:4;
  byte asvc_flag:1;
  byte mainid_flag:1;
  byte bsid_flag:1;
  byte component_type_flag:1;
} tTSAC3Desc;            // Ist das richtig??

typedef struct
{
  byte DescrTag;
  byte DescrLength;
  char LanguageCode[3]; // without terminating 0
  byte AudioType;
} tTSAudioDesc;

typedef struct
{
  byte DescrTag;
  byte DescrLength;
  char LanguageCode[3];  // without terminating 0
  byte TtxMagazine:1;    // = 1
  byte Unknown2:2;       // = 0
  byte TtxType:2;        // 1 = initial Teletext page
  byte Unknown:3;        // unknown
  byte FirstPage;
} tTSTtxDesc;

/*typedef struct
{
  byte stream_type;
  byte ESPID1:5;
  byte Reserved1:3;      // = 0x03 (all 1)
  byte ESPID2;
  byte ESInfoLen1:4;
  byte Reserved2:4;      // = 0x07 (all 1)
  byte ESInfoLen2;
} tElemStream;

typedef struct
{
  byte TableID;
  byte SectionLen1:4;    // first 2 bits are 0
  byte Reserved1:2;      // = 0x03 (all 1)
  byte Private:1;        // = 0
  byte SectionSyntax:1;  // = 1
  byte SectionLen2;
  byte ProgramNr1;
  byte ProgramNr2;
  byte CurNextInd:1;
  byte VersionNr:5;
  byte Reserved2:2;      // = 0x03 (all 1)
  byte SectionNr;
  byte LastSection;

  word PCRPID1:5;
  word Reserved3:3;      // = 0x07 (all 1)
  word PCRPID2:8;

  word ProgInfoLen1:4;   // first 2 bits are 0
  word Reserved4:4;      // = 0x0F (all 1)
  word ProgInfoLen2:8;
  // ProgInfo (of length ProgInfoLen1*256 + ProgInfoLen2)
  // Elementary Streams (of remaining SectionLen)
} tTSPMT; */


dword crc32m_tab(const unsigned char *buf, size_t len);
bool SaveHumaxHeader(char *const VidFileName, char *const OutFileName);
bool LoadHumaxHeader(FILE *fIn, byte *const PATPMTBuf, TYPE_RecHeader_TMSS *RecInf);

#endif
