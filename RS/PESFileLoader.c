#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "type.h"
#include "RecStrip.h"
#include "PESFileLoader.h"
#include "RebuildInf.h"
#include "NavProcessor.h"
#include "HumaxHeader.h"


tPESStream              PESVideo;
static tPESStream       PESAudio, PESTeletxt;
byte                   *EPGBuffer;
int                     EPGLen = 0;
static const byte       PIDs[4] = {100, 101, 102, 0x12};
static byte             ContCtr[4] = {1, 1, 1, 1};
static bool             DoEITOutput = TRUE;

// Simple PES Extractor
/*{
  tPSBuffer PSBuf;
  FILE *in = fopen("C:/Users/Test/Desktop/PMTFehler/Star Trek Beyond - 2019-02-18 01-07_DD.rec", "rb");
  FILE *out = fopen("C:/Users/Test/Desktop/PMTFehler/Star Trek Beyond - 2019-02-18 01-07_DD_ttx.pes", "wb");
  bool LastBuffer = 0;

  PSBuffer_Init(&PSBuf, 0x21, 128000, FALSE);

  while (fread(Buffer, 192, 1, in))
  {
    if (Buffer[4] == 'G')
    {
        PSBuffer_ProcessTSPacket(&PSBuf, (tTSPacket*)(&Buffer[4]));
        if(PSBuf.ValidBuffer != LastBuffer)
        {
          if(PSBuf.ValidBuffer == 1)
          {
            int pes_packet_length = 6 + ((PSBuf.Buffer1[4] << 8) | PSBuf.Buffer1[5]);
            fwrite(PSBuf.Buffer1, pes_packet_length, 1, out);
          }
          else if(PSBuf.ValidBuffer == 2)
          {
            int pes_packet_length = 6 + ((PSBuf.Buffer2[4] << 8) | PSBuf.Buffer2[5]);
            fwrite(PSBuf.Buffer2, pes_packet_length, 1, out);
          }
          LastBuffer = PSBuf.ValidBuffer;
        }
    }
  }
  fclose(out);
  fclose(in);
  PSBuffer_Reset(&PSBuf);
  exit(17);
} */


// ---------------------------------------------------------------------------------
// PES File Loader (PES-paketbasierter Puffer)
// ---------------------------------------------------------------------------------

bool PESStream_Open(tPESStream *PESStream, FILE* fSource, int BufferSize)
{
  TRACEENTER;

  memset(PESStream, 0, sizeof(tPESStream));
  
  if ((PESStream->Buffer = (byte*) malloc(BufferSize)))
  {
    PESStream->BufferSize = BufferSize;
    memset(PESStream->Buffer, 0, BufferSize);
    PESStream->Buffer[0] = 0;
    PESStream->Buffer[1] = 0;
    PESStream->Buffer[2] = 1;

    if (fSource)
      PESStream->fSrc = fSource;
    else
    {
      PESStream->FileAtEnd = TRUE;
      PESStream->DTSOverflow = TRUE;
      PESStream->curPacketLength = 0;
      PESStream->curPacketDTS = 0xffffffff;
    }
    TRACEEXIT;
    return TRUE;
  }
  else
  {
    printf("  PESFileLoader: Cannot allocate %d bytes of memory.", PESStream->BufferSize);
    TRACEEXIT;
    return FALSE;
  }
}

void PESStream_Close(tPESStream *PESStream)
{
  TRACEENTER;
  if(PESStream && PESStream->fSrc)
  {
//    if(PESStream->fSrc)
//      fclose(PESStream->fSrc);
    if(PESStream->Buffer)
      free(PESStream->Buffer);
    memset(PESStream, 0, sizeof(tPESStream));
  }
  TRACEEXIT;
}

static int PESStream_FindPacketStart(tPESStream *PESStream, dword StartAtPos)
{
  dword                 History = 0xffffffff;
  int                   SkipFirst = 0, i;

  TRACEENTER;

  for (i = StartAtPos; i+2 < PESStream->BufferSize; i++)
  {
    if (fread(&PESStream->Buffer[i], 1, 1, PESStream->fSrc))
    {  
      History = (History << 8) | PESStream->Buffer[i];

      if ((History & 0xFFFFFF00) == 0x00000100)
      {
        if (PESStream->Buffer[i] >= 0xB9)
        {
          // Start of PES packet
          if (PESStream->curPacketLength <= StartAtPos)
            PESStream->curPacketLength = i - 3;

          // Read 2 more bytes (length of next packet)
          if (!fread(&PESStream->Buffer[i+1], 2, 1, PESStream->fSrc))
          {
            PESStream->Buffer[i+1] = 0; PESStream->Buffer[i+2] = 0;
          }

          // Ignore first audio packet, if invalid length
          if (!PESStream->NextStartCodeFound && !PESStream->isVideo && SkipFirst < 3)
          {
            if (PESStream->Buffer[i+1] >= 32)   // (PESStream->Buffer[i+1] * 256 + PESStream->Buffer[i+2] > 8000)
            {
              printf("  SimpleMuxer: First audio / teletext packet skipped (invalid length %hu).\n", PESStream->Buffer[i+1] * 256 + PESStream->Buffer[i+2]);
              SkipFirst++;
              continue;
            }
          }
          TRACEEXIT;
          return i;
        }
        else if (PESStream->isVideo)
          PESStream->SliceState = ((PESStream->Buffer[i] >= 0x01) && (PESStream->Buffer[i] <= 0xAF));
      }
      else if ((History == 0) && MedionStrip && PESStream->isVideo && PESStream->SliceState)  // 4 Nullen am Stück gefunden (Zero byte padding)
      {
        i--;  // letzte 0 nicht in Buffer behalten
        NrDroppedZeroStuffing++;
      }
    }
    else
      break;
  }
  PESStream->curPacketLength = i;

  if (i+2 >= PESStream->BufferSize)
  {
    printf("  PESFileLoader: Insufficient buffer size!\n");
    PESStream->ErrorFlag = TRUE;
  }

  TRACEEXIT;
  return 0;
}

// Annahme: p zeigt auf Start eines Pakets, nächster Startcode (3 Bytes) bereits gelesen
byte* PESStream_GetNextPacket(tPESStream *PESStream)
{
  tPESHeader           *curPacket;
  TRACEENTER;

  // Nur beim ersten Paket: Anfang des Pakets finden, dann Länge dieses Pakets ermitteln
  if (!PESStream->NextStartCodeFound)
  {
    if (PESStream->curPacketLength || !(PESStream->NextStartCodeFound = PESStream_FindPacketStart(PESStream, 3)))
    {
      PESStream->FileAtEnd = TRUE;
      PESStream->DTSOverflow = TRUE;
      PESStream->curPacketLength = 0;
      PESStream->curPacketDTS = 0xffffffff;
      TRACEEXIT;
      return FALSE;
    }
  }

  // Verschiebe StartCode und Länge des zuletzt gelesenen Pakets an Anfang des Buffers
  memmove(&PESStream->Buffer[3], &PESStream->Buffer[PESStream->NextStartCodeFound], 3);
  curPacket = (tPESHeader*) PESStream->Buffer;

  // Extract attributes (1)
  PESStream->PesId = curPacket->StreamID;
  PESStream->isVideo = (PESStream->PesId >= 0xe0 && PESStream->PesId <= 0xef);

  // Ermittle Länge
  PESStream->curPacketLength = (curPacket->PacketLength1 * 256) + curPacket->PacketLength2;
  if (PESStream->curPacketLength > 0)
    PESStream->curPacketLength = (dword)fread(&PESStream->Buffer[6], 1, PESStream->curPacketLength, PESStream->fSrc);
  PESStream->curPacketLength += 6;

  PESStream->NextStartCodeFound = PESStream_FindPacketStart(PESStream, PESStream->curPacketLength);
#ifdef _DEBUG
  if (PESStream->curPacketLength > (dword)PESStream->maxPESLen)
    PESStream->maxPESLen = PESStream->curPacketLength;
#endif

  // Extract attributes (2)
  PESStream->curPayloadStart = (curPacket->OptionalHeaderMarker == 2) ? 9 + curPacket->PESHeaderLen : 6;
  PESStream->SliceState = FALSE;

  // Extract DTS
  if (curPacket->PTSpresent)
  {
    dword lastPacketDTS = PESStream->curPacketDTS;
    PESStream->curPacketDTS = 0;
    GetPTS2(&PESStream->Buffer[3], NULL, &PESStream->curPacketDTS);
    PESStream->DTSOverflow = (PESStream->curPacketDTS < lastPacketDTS);
  }
  else
    PESStream->curPacketDTS = 0;

  // Remove zero byte stuffing at end of packet
  if (MedionStrip && PESStream->isVideo)
  {
    int HeaderLen = 6;
    if (curPacket->OptionalHeaderMarker == 2)
      HeaderLen += (3 + curPacket->PESHeaderLen);
            
    curPacket->PacketLength1 = 0;
    curPacket->PacketLength2 = 0;

    {
      int newLen;
      byte *p = &PESStream->Buffer[PESStream->curPacketLength-1];
      while(*p == 0) p--;
      
      newLen = (int)(p - PESStream->Buffer) + 1;
      if (newLen > HeaderLen)
      {
        if(PESStream->Buffer[PESStream->curPacketLength-1] == 0) newLen++;  // mindestens eine Null am Ende erhalten, wenn vorher eine da war (ist TS-Doctor Bug!!!)
        NrDroppedZeroStuffing += (PESStream->curPacketLength - newLen);
        PESStream->curPacketLength = newLen;
      }
      else
      {
        // Es sind nur Fülldaten im PES-Paket enthalten -> ganzes Paket überspringen
        NrDroppedZeroStuffing += (PESStream->curPacketLength);
printf("DEBUG: Assertion. Empty PES packet (after stripping) found!");
        TRACEEXIT;
        return PESStream_GetNextPacket(PESStream);
      }
    }
  }

  TRACEEXIT;
  return PESStream->Buffer;
}


// ---------------------------------------------------------------------------------
// SIMPLE PES-to-TS MUXER
// ---------------------------------------------------------------------------------
static bool           FirstRun = 2;
static dword          LastVidDTS = 0;
static byte           curPid = 0;
static int            StreamNr = 0;


// Generate a PMT
void GeneratePatPmt(byte *const PATPMTBuf, word ServiceID, word PMTPID, word VideoPID, word AudioPID, word TtxPID, tVideoStreamFmt VideoType, tAudioStreamFmt AudioType)
{
  tTSPacket          *Packet = NULL;
  tTSPAT             *PAT = NULL;
  tTSPMT             *PMT = NULL;
  dword              *CRC = NULL;
  int                 i, Offset = 0;

  TRACEENTER;
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
  PAT->ProgramNr1       = ServiceID / 256;
  PAT->ProgramNr2       = (ServiceID & 0xff);
  PAT->PMTPID1          = PMTPID / 256;
  PAT->PMTPID2          = (PMTPID & 0xff);
  PAT->Reserved111      = 7;
  PAT->CRC32            = crc32m_tab((byte*)PAT, sizeof(tTSPAT)-4);      // CRC: 0x786989a2
  
  Offset = 1 + /*Packet->Data[0] +*/ sizeof(tTSPAT);
  memset(&Packet->Data[Offset], 0xff, 184 - Offset);

  Packet = (tTSPacket*) &PATPMTBuf[196];
  PMT = (tTSPMT*) &Packet->Data[1 /*+ Packet->Data[0]*/];

  Packet->SyncByte      = 'G';
  Packet->PID1          = PMTPID / 256;
  Packet->PID2          = (PMTPID & 0xff);
  Packet->Payload_Unit_Start = 1;
  Packet->Payload_Exists = 1;

  PMT->TableID          = TABLE_PMT;
  PMT->SectionLen1      = 0;
  PMT->SectionLen2      = sizeof(tTSPMT) - 3 + 4;
  PMT->Reserved1        = 3;
  PMT->Private          = 0;
  PMT->SectionSyntax    = 1;
  PMT->ProgramNr1       = ServiceID / 256;
  PMT->ProgramNr2       = (ServiceID & 0xff);
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

  for (i = 0; i < 3; i++)
  {
    tElemStream *Elem = (tElemStream*) &Packet->Data[Offset];
    Elem->ESInfoLen1      = 0;
    Elem->Reserved1       = 7;
    Elem->Reserved2       = 0xf;
    Offset                += sizeof(tElemStream);

    if (i == 0)
    {
      Elem->stream_type   = VideoType;
      Elem->ESPID1        = VideoPID / 256;
      Elem->ESPID2        = (VideoPID & 0xff);
      Elem->ESInfoLen2    = 0;
    }
    else if (i == 1 && AudioPID != (word)-1)
    {
      tTSAudioDesc *Desc  = (tTSAudioDesc*) &Packet->Data[Offset];

      Elem->stream_type   = AudioType;
      Elem->ESPID1        = AudioPID / 256;
      Elem->ESPID2        = (AudioPID & 0xff);
      Elem->ESInfoLen2    = sizeof(tTSAudioDesc);
      Desc = (tTSAudioDesc*) &Packet->Data[Offset];
      Desc->DescrTag      = DESC_Audio;
      Desc->DescrLength   = 4;
      memcpy(Desc->LanguageCode, "deu", 3);
    }
    else if (TtxPID != (word)-1)
    {
      tTSTtxDesc *Desc = (tTSTtxDesc*) &Packet->Data[Offset];

      Elem->stream_type     = 6;
      Elem->ESPID1          = TtxPID / 256;
      Elem->ESPID2          = (TtxPID & 0xff);
      Elem->ESInfoLen2      = 7;
      Desc->DescrTag        = DESC_Teletext;
      Desc->DescrLength     = 5;
      memcpy(Desc->LanguageCode, "deu", 3); 
      Desc->TtxType         = 1;
      Desc->TtxMagazine     = 1;
    }

    Offset                += Elem->ESInfoLen2;
    PMT->SectionLen2      += sizeof(tElemStream) + Elem->ESInfoLen2;
  }

  PMT->PCRPID1            = VideoPID / 256;
  PMT->PCRPID2            = (VideoPID & 0xff);
  CRC                     = (dword*) &Packet->Data[Offset];
  *CRC                    = crc32m_tab((byte*)PMT, (byte*)CRC - (byte*)PMT);     // CRC: 0x0043710d  (0xb3ad75b7?)
  Offset                 += 4;
  memset(&Packet->Data[Offset], 0xff, 184 - Offset);
  TRACEEXIT;
}


// Simple Muxer
bool SimpleMuxer_Open(FILE *fIn, char const* PESAudName, char const* PESTtxName, char const* EITName)  // fIn ist ein PES Video Stream
{
  FILE *aud = NULL, *ttx = NULL;  //, *eit = NULL;
  TRACEENTER;

  FirstRun = 2;
  LastVidDTS = 0;
  curPid = 0;
  StreamNr = 0;
  DoEITOutput = TRUE;

  if ((aud = fopen(PESAudName, "rb")))
    setvbuf(aud, NULL, _IOFBF, BUFSIZE);
  else
    printf("  SimpleMuxer: Cannot open file %s.\n", PESAudName);

  if ((ttx = fopen(PESTtxName, "rb")))
    setvbuf(ttx, NULL, _IOFBF, BUFSIZE);
  else
    printf("  SimpleMuxer: Cannot open file %s.\n", PESTtxName);

/*  EPGLen = 0;
  if ((eit = fopen(EITName, "rb")))
  {
    int EITMagic = 0;
    if (fread(&EITMagic, 4, 1, eit) && (EITMagic == 0x12345678))
      if (fread(&EPGLen, 4, 1, eit))
      {
        if (EPGLen && ((EPGBuffer = (byte*)malloc(EPGLen + 1))))
        {
          EPGBuffer[0] = 0;  // Pointer field (=0) vor der TableID (nur im ersten TS-Paket der Tabelle, gibt den Offset an, an der die Tabelle startet, z.B. wenn noch Reste der vorherigen am Paketanfang stehen)
          if ((EPGLen = (int)fread(&EPGBuffer[1], 1, EPGLen, eit)))
            EPGLen += 1;
        }
        else
          printf("  SimpleMuxer: Cannot allocate EIT buffer.\n");
      }
    fclose(eit);
  } */
  if (EITName && !EPGLen)
    printf("  SimpleMuxer: Cannot open file %s.\n", EITName);

  if (PESStream_Open(&PESVideo, fIn, VIDEOBUFSIZE) && PESStream_Open(&PESAudio, aud, 131027) && PESStream_Open(&PESTeletxt, ttx, 32768))
  {
    if (!PESVideo.FileAtEnd)   { PESStream_GetNextPacket(&PESVideo); VideoPID = 100; }
    if (!PESAudio.FileAtEnd)   { PESStream_GetNextPacket(&PESAudio); }
    if (!PESTeletxt.FileAtEnd) { PESStream_GetNextPacket(&PESTeletxt); TeletextPID = 102; }

    TRACEEXIT;
    return (!PESVideo.ErrorFlag && !PESAudio.ErrorFlag && !PESTeletxt.ErrorFlag);
  }

  TRACEEXIT;
  return FALSE;
}

void SimpleMuxer_DoEITOutput(void)
{
  TRACEENTER;
  DoEITOutput = TRUE;
  TRACEEXIT;
}

bool SimpleMuxer_NextTSPacket(tTSPacket *pack)
{
  static byte          *p;
  static int            len;
  bool                  ret = FALSE;

  TRACEENTER;

  pack->SyncByte = 'G';

  // ersten PCR als eigenes Paket einfügen? (ProjectX -> NEIN!)
/*  if (FirstRun == 2)
  {
    pack->PID1 = PIDs[0] / 256;
    pack->PID2 = (PIDs[0] & 0xff);
    pack->Payload_Exists = FALSE;
    pack->Adapt_Field_Exists = TRUE;
    pack->Data[0] = 183;
    memset(&pack->Data[8], 0xff, 176);
    SetPCR((byte*)pack, (long long)(PESVideo.curPacketDTS-4500) * 2 * 300);
    FirstRun = 1;

    TRACEEXIT;
    return TRUE;
  } */

  if (FirstRun)
  {
    // Skip video packets until Sequence header code (0xB3)
    while (!PESVideo.FileAtEnd)
      if (PESVideo.Buffer[PESVideo.curPayloadStart]==0 && PESVideo.Buffer[PESVideo.curPayloadStart+1]==0 && PESVideo.Buffer[PESVideo.curPayloadStart+2]==1 && PESVideo.Buffer[PESVideo.curPayloadStart+3]==0xB3)
        break;
      else
        PESStream_GetNextPacket(&PESVideo);

    // Skip audio and teletext packets with DTS > 2 sec before first video packet
    if (!PESVideo.FileAtEnd && PESVideo.curPacketDTS >= 90000)
    {
      dword AudioSkipped = 0, TtxSkipped = 0;
      while (!PESAudio.FileAtEnd && (PESAudio.curPacketDTS + 90000 < PESVideo.curPacketDTS))
        { PESStream_GetNextPacket(&PESAudio); AudioSkipped++; }
      while (!PESTeletxt.FileAtEnd && (PESTeletxt.curPacketDTS + 90000 < PESVideo.curPacketDTS))
        { PESStream_GetNextPacket(&PESTeletxt); TtxSkipped++; }

      if (AudioSkipped || TtxSkipped)
        printf("  SimpleMuxer: %u audio packets, %u teletext packets skipped at beginning.\n", AudioSkipped, TtxSkipped);
    }
    FirstRun = 0;
  }

  pack->Payload_Exists = TRUE;

  if (!PESVideo.FileAtEnd || !PESAudio.FileAtEnd || !PESTeletxt.FileAtEnd)
  {
    pack->Adapt_Field_Exists = FALSE;
    if (curPid)
      pack->Payload_Unit_Start = FALSE;
    else
    {
      if ((PESVideo.DTSOverflow /*|| PESVideo.FileAtEnd*/) && (PESAudio.DTSOverflow /*|| PESAudio.FileAtEnd*/) && (PESTeletxt.DTSOverflow /*|| PESTeletxt.FileAtEnd*/))
        PESVideo.DTSOverflow = PESAudio.DTSOverflow = PESTeletxt.DTSOverflow = FALSE;

      if (DoEITOutput && EPGLen)
      {
        p = EPGBuffer;
        len = EPGLen;
        StreamNr = 3;
        DoEITOutput = FALSE;
      }
      else if (!PESVideo.FileAtEnd && (/*FirstRun ||*/ ((PESVideo.curPacketDTS <= PESAudio.curPacketDTS || PESAudio.DTSOverflow) && (PESVideo.curPacketDTS <= PESTeletxt.curPacketDTS || PESTeletxt.DTSOverflow) && !PESVideo.DTSOverflow)))
      {
        // Start with video packet ----^
//        FirstRun = 0;
        p = PESVideo.Buffer;
        len = PESVideo.curPacketLength;
        StreamNr = 0;
        if (LastVidDTS)
        {
          long long newPCR = (long long)(LastVidDTS) * 600 - PCRTOPTSOFFSET_SD;
          pack->Adapt_Field_Exists = TRUE;
          pack->Data[0] = 7;  // Adaptation Field Length
          if(newPCR < 0) newPCR += 2576980377600LL;  // falls Überlauf von LastVidDTS (= 2^32 * 600)
          SetPCR((byte*)pack, newPCR);
        }
        LastVidDTS = PESVideo.curPacketDTS;
      }
      else if (!PESAudio.FileAtEnd && (PESAudio.curPacketDTS <= PESTeletxt.curPacketDTS || PESTeletxt.DTSOverflow) && !PESAudio.DTSOverflow)
      {
        p = PESAudio.Buffer;
        len = PESAudio.curPacketLength;
        StreamNr = 1;
      }
      else if (!PESTeletxt.FileAtEnd && !PESTeletxt.DTSOverflow)
      {
        p = PESTeletxt.Buffer;
        len = PESTeletxt.curPacketLength;
        StreamNr = 2;
      }
      pack->Payload_Unit_Start = TRUE;
      curPid = PIDs[StreamNr];
    }
        
    if (curPid && len)
    {
      int PayloadBytes, PayloadStart;
      pack->PID1 = curPid / 256;
      pack->PID2 = (curPid & 0xff);
      pack->ContinuityCount = (ContCtr[StreamNr]++) % 16;

      PayloadStart = (pack->Adapt_Field_Exists) ? (1 + pack->Data[0]) : 0;
      PayloadBytes = 184 - PayloadStart;

      if (len < PayloadBytes)
      {
        if (curPid == 0x12)
          // bei Tabellen kein Stuffing im Adaptation Field, sondern 0xFF Fill-Bytes am Ende
          memset(&pack->Data[PayloadStart+len], 0xff, PayloadBytes - len);
        else
          // Zero byte padding hinzufügen
//          memset(&pack->Data[PayloadStart+len], 0x00, PayloadBytes - len);
        {
          // Alternative: Stuffing Bytes im Adaptation Field
          int FillBytes, FillStart;

          if (pack->Adapt_Field_Exists)
          {
            FillBytes = PayloadBytes - len;
            FillStart = 1 + pack->Data[0];
            pack->Data[0] += FillBytes;
            PayloadStart = 1 + pack->Data[0];  //  <==>  PayloadStart += FillBytes
          }
          else
          {
            pack->Adapt_Field_Exists = TRUE;
            FillBytes = PayloadBytes - len - 1;
            FillStart = 1;
            pack->Data[0] = FillBytes;
            PayloadStart = 1 + pack->Data[0];  //  <==>  PayloadStart = (1 + FillBytes)
          }

          if ((FillStart == 1) && (FillBytes > 0))
          {
            pack->Data[1] = 0;
            FillStart++;
            FillBytes--;
          }
          memset(&pack->Data[FillStart], 0xff, FillBytes);
        }
        PayloadBytes = len;
      }
      memcpy(&pack->Data[PayloadStart], p, PayloadBytes);
      p += PayloadBytes;
      len -= PayloadBytes;
      ret = TRUE;
    }
    if (curPid && (len == 0))
    {
      switch (StreamNr)
      {
        case 0: PESStream_GetNextPacket(&PESVideo); break;
        case 1: PESStream_GetNextPacket(&PESAudio); break;
        case 2: PESStream_GetNextPacket(&PESTeletxt); break;
      }
      curPid = 0;
    }
  }

  if (ret && (PESVideo.ErrorFlag || PESAudio.ErrorFlag || PESTeletxt.ErrorFlag))
    ret = -1;

  TRACEEXIT;
  return ret;
}

void SimpleMuxer_Close(void)
{
  TRACEENTER;

  if(PESAudio.fSrc) fclose(PESAudio.fSrc);
  if(PESTeletxt.fSrc) fclose(PESTeletxt.fSrc);
  PESStream_Close(&PESVideo);
  PESStream_Close(&PESAudio);
  PESStream_Close(&PESTeletxt);
  free(EPGBuffer);
  EPGBuffer = NULL;
  EPGLen = 0;

  TRACEEXIT;
}


/* ERSTER ANSATZ
  fwrite(&PATPMTBuf[4], 188, 1, out);
  fwrite(&PATPMTBuf[4 + 192], 188, 1, out);

  memset(Buffer, 0, 188);
  pack->SyncByte = 'G';
  pack->PID1 = 0;

  for (i = 0; i < 500000; i++)
  {
    pack->Adapt_Field_Exists = FALSE;
    pack->Payload_Exists = TRUE;
    if (i % 28 == 0)
    {
      if (fread(&Buffer[4], 184, 1, aud))
      {
        pack->PID2 = 101;
        pack->ContinuityCount = (lastAudCount++) % 16;
        fwrite(Buffer, 188, 1, out);
      }
    }
    if (fread(&Buffer[4], 184, 1, fIn))
    {
      pack->PID2 = 100;
      pack->ContinuityCount = (lastVidCount++) % 16;
      fwrite(Buffer, 188, 1, out);
    }
    if (i % 196 == 0)
    {
      memset(Buffer, 0, 188); 
      pack->SyncByte = 'G';
      pack->PID2 = 102;
      pack->ContinuityCount = 0; //(lastPMTCount++) % 16;
      pack->Adapt_Field_Exists = TRUE;
      pack->Payload_Exists = FALSE;
      pack->Data[0] = 183;
      SetPCR((byte*)pack, (pcr+=(40*27000)));
      fwrite(Buffer, 188, 1, out);
    }
  }
  fclose(out);
  fclose(aud); */
