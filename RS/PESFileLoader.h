#ifndef __PESFILELOADERH__
#define __PESFILELOADERH__

#define PCRTOPTSOFFSET_SD 20000000

typedef struct
{
  byte                  StartCode[3];
  byte                  StreamID;
  byte                  PacketLength1;
  byte                  PacketLength2;

  byte                  isOriginal:1;
  byte                  Copyright:1;
  byte                  DataAlignment:1;         // 1 if Video Start Code imediately follows header
  byte                  Priority:1;
  byte                  ScramblingCtrl:2;        // 00 = not scrambled
  byte                  OptionalHeaderMarker:2;  // 10 (=2) if Optional Header present

  byte                  ExtensionFlag:1;
  byte                  CRCflag:1;
  byte                  AdditionalCopyInfo:1;
  byte                  DSMTrickMode:1;
  byte                  ESRateFlag:1;
  byte                  ESCRpresent:1;
  byte                  DTSpresent:1;            // DTS can only be present if also PTS is present
  byte                  PTSpresent:1;

  byte                  PESHeaderLen;
  byte                  HeaderData[0];
} tPESHeader;

typedef struct
{
  FILE                 *fSrc;
  byte                 *Buffer;
  int                   BufferSize;
  int                   PesId;
  bool                  isVideo;
  bool                  SliceState;
  dword                 curPacketLength;
  dword                 curPayloadStart;
  dword                 curPacketDTS;
  int                   NextStartCodeFound;
  bool                  DTSOverflow;
  bool                  FileAtEnd;
  bool                  ErrorFlag;
#ifdef _DEBUG
  int                   maxPESLen;
#endif
} tPESStream;


extern tPESStream       PESVideo;
extern byte            *EPGBuffer;
extern int              EPGLen;

bool PESStream_Open(tPESStream *PESStream, FILE *fSource, int BufferSize);
void PESStream_Close(tPESStream *PESStream);
byte* PESStream_GetNextPacket(tPESStream *PESStream);


void GeneratePatPmt(byte *const PATPMTBuf, word ServiceID, word PMTPID, word VideoPID, word AudioPID, word TtxPID, tVideoStreamFmt VideoType, tAudioStreamFmt AudioType);

bool SimpleMuxer_Open(FILE *fIn, char const* PESAudName, char const* PESTtxName, char const* EITName);
void SimpleMuxer_DoEITOutput(void);
bool SimpleMuxer_NextTSPacket(tTSPacket *pack);
void SimpleMuxer_Close(void);

#endif
