#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define inline
#endif

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
#include "NALUDump.h"
#include "NavProcessor.h"
#include "TtxProcessor.h"
#include "RecStrip.h"


// Globale Variablen
static eNaluFillState   NaluFillState = NALU_INIT;
static bool             SliceState = TRUE;

static unsigned int     History = 0xffffffff;

//bool                    NoContinuityCheck = FALSE;
int                     LastContinuityInput = -1;
static int              LastContinuityOutput = -1;
static int              PendingContinuity = -1;
//static int              ContinuityOffset = 0;

static bool             DropAllPayload = TRUE;

static int              PesId = -1;
static int              PesOffset = 0;

static int              LastEndNulls = 1;
static bool             PendingPacket = FALSE;

// ----------------------------------------------
// *****  NALU Dump  *****
// ----------------------------------------------

inline word TsGetPID(tTSPacket *Packet)
{
  return ((Packet->PID1 *256) | Packet->PID2);
}
inline int TsPayloadOffset(tTSPacket *Packet)
{
  int o = (Packet->Adapt_Field_Exists) ? Packet->Data[0]+5 : 4;
  return (o <= TS_SIZE ? o : TS_SIZE);
}

void NALUDump_Init(void)
{
  TRACEENTER;

  NaluFillState = NALU_INIT;
  SliceState = TRUE;

  History = 0xffffffff;

  LastContinuityInput = -1;
  LastContinuityOutput = -1;
  PendingContinuity = -1;
//  ContinuityOffset = 0;

  DropAllPayload = TRUE;

  PesId = -1;
  PesOffset = 0;

  LastEndNulls = 1;
  PendingPacket = FALSE;

  TRACEEXIT;
}

static void TsExtendAdaptionField(byte *Packet, int ToLength)
{
  // Hint: ExtendAdaptionField(p, TsPayloadOffset(p) - 4) is a null operation
  tTSPacket *TSPacket;
  int Offset;
  int NewPayload;

  TRACEENTER;
  TSPacket = (tTSPacket*) Packet;
  Offset = TsPayloadOffset(TSPacket); // First byte after existing adaption field

  if (ToLength <= 0)
  {
    // Remove adaption field
    TSPacket->Adapt_Field_Exists = 0;
    TRACEEXIT;
    return;
  }

  // Set adaption field present
  TSPacket->Adapt_Field_Exists = 1;

  // Set new length of adaption field:
  TSPacket->Data[0] = min(ToLength-1, TS_SIZE-4-1);

  if (TSPacket->Data[0] == TS_SIZE-4-1)
  {
    // No more payload, remove payload flag
    TSPacket->Payload_Exists = 0;
  }

  NewPayload = TSPacket->Data[0] + 5; // First byte after new adaption field

  // Fill new adaption field
  if (Offset == 4 && Offset < NewPayload)
    Offset++; // skip adaptation_field_length
  if (Offset == 5 && Offset < NewPayload)
    Packet[Offset++] = 0; // various flags set to 0
  while (Offset < NewPayload)
    Packet[Offset++] = 0xff; // stuffing byte

  TRACEEXIT;
}

static void ProcessPayload_HD(unsigned char *Payload, int size, bool PayloadStart, sPayloadInfo *Info)
{
  bool DropByte;
  int LastKeepByte = -1, i;

  TRACEENTER;
  Info->DropPayloadStartBytes = 0;
  Info->DropPayloadEndBytes = 0;
  Info->ZerosOnly = TRUE;

  if (PayloadStart)
  {
    History = 0xffffffff;
    PesId = -1;
    NaluFillState = NALU_NONE;
  }

  for (i = 0; i < size; i++)
  {
    if (Payload[i] != 0) Info->ZerosOnly = FALSE;
    DropByte = FALSE;

    History = (History << 8) | Payload[i];
    PesOffset++;

    if (History >= 0x000001B9 && History <= 0x000001FF)  // im Original: >= 0x00000180
    {
      // Start of PES packet
      PesId = History & 0xff;
      PesOffset = 0;
      NaluFillState = NALU_NONE;

      LastKeepByte = i;
      continue;
    }
    else if (PesId >= 0xe0 && PesId <= 0xef)  // video stream
    {
      if (History >= 0x00000100 && History <= 0x0000017F)  // NALU start code
      {
        int NaluId = History & 0xff;
        NaluFillState = ((NaluId & 0x1f) == 0x0c) ? NALU_FILL : NALU_NONE;
        if ((NaluFillState == NALU_FILL) && (i == 3))
        {
//          DropByte = TRUE;        // CW
          LastKeepByte = -1;      // CW
          DropAllPayload = TRUE;  // CW
        }
        else
          LastKeepByte = i;
        continue;
      }
    }
    if ((PesId >= 0xe0 && PesId <= 0xef) || (PesId == -1))  // video or no stream
    {
      if ((NaluFillState >= NALU_NONE) && (Payload[i] == 0xff))
      {
        NaluFillState = NALU_FILLWITHOUTSTART;
//        DropByte = TRUE;
        if (i == 0 && size > 100)
          DropAllPayload = TRUE;
      }
    }

    if (PesId >= 0xe0 && PesId <= 0xef  // video stream
     && (PesOffset == 1 || PesOffset == 2))
    {
      Payload[i] = 0;  // Zero out PES length field
      History = History | 0xff;
    }

    if (NaluFillState <= NALU_FILLWITHOUTSTART)  // Within NALU fill data (NALU_FILL und NALU_INIT)
    {
      // We expect a series of 0xff bytes terminated by a single 0x80 byte.

      if (Payload[i] == 0xFF)
      {
        DropByte = TRUE;
      }
      else if (Payload[i] == 0x80)
      {
        if (NaluFillState <= NALU_FILL || (i - LastKeepByte) > 10)
        {
          if (NaluFillState == NALU_FILLWITHOUTSTART)
            printf("cNaluDumper: Filler NALU without startcode deleted (length=%d)\n", i-LastKeepByte);
          NaluFillState = NALU_TERM;  // Last byte of NALU fill, next byte sets NaluFillEnd=true
          DropByte = TRUE;
        }
        else
          NaluFillState = NALU_NONE;
      }
      else  // Invalid NALU fill
      {
        if (NaluFillState == NALU_FILL)
        {
          printf("cNaluDumper: Unexpected NALU fill data: %02x\n", Payload[i]);

          if (LastKeepByte == -1)
          {
            // Nalu fill from beginning of packet until last byte
            // packet start needs to be dropped
            Info->DropPayloadStartBytes = i;
          }
        }
        NaluFillState = NALU_NONE;
      }
    }
    else if (NaluFillState == NALU_TERM) // Within NALU fill data
    {
      // We are after the terminating 0x80 byte
      if (LastKeepByte == -1)
      {
        // Nalu fill from beginning of packet until last byte
        // packet start needs to be dropped
        Info->DropPayloadStartBytes = i;
      }
      NaluFillState = NALU_NONE;  // NALU_END;
    }

    if (!DropByte)
      LastKeepByte = i; // Last useful byte
  }

  if (NaluFillState == NALU_FILLWITHOUTSTART)
  {
    if (i - LastKeepByte >= 100)
      printf("cNaluDumper: Filler NALU without start- and endcode deleted (length=%d)\n", i-LastKeepByte-1);
    else
      LastKeepByte = size - 1;
  }
  Info->DropAllPayloadBytes = (LastKeepByte == -1);
  Info->DropPayloadEndBytes = size-1-LastKeepByte;
  TRACEEXIT;
}

static void ProcessPayload_SD(unsigned char *Payload, int size, bool PayloadStart, sPayloadInfo *Info)
{
  byte StartCodeID = 0;
  int i;

  TRACEENTER;
  Info->ZerosOnly = TRUE;

  if (PayloadStart)
  {
    History = 0xffffffff;
    PesId = -1;
    SliceState = FALSE;
  }

  for (i = 0; i < size; i++)
  {
    if (Payload[i] != 0) Info->ZerosOnly = FALSE;

    History = (History << 8) | Payload[i];
    PesOffset++;

    if ((History & 0xFFFFFF00) == 0x00000100)
    {
      StartCodeID = Payload[i];
      if (StartCodeID >= 0xB9)
      {
        // Start of PES packet
        PesId = StartCodeID;
        PesOffset = 0;
      }
      if (PesId >= 0xe0 && PesId <= 0xef)  // video stream
        SliceState = (StartCodeID >= 0x01 && StartCodeID <= 0xAF);
    }

    if (PesId >= 0xe0 && PesId <= 0xef  // video stream
     && PesOffset >= 1 && PesOffset <= 2)
    {
      Payload[i] = 0; // Zero out PES length field
      History = History | 0xff;
    }
  }
  TRACEEXIT;
}

int ProcessTSPacket(unsigned char *Packet, long long FilePosition)
{
  tTSPacket *TSPacket = (tTSPacket*) Packet;
  int ContinuityOffset = 0;
  int ContinuityInput;
  int Result = 0;

  TRACEENTER;

  // Check continuity:
  ContinuityInput = TSPacket->ContinuityCount;
  if (LastContinuityInput >= 0)
  {
    int NewContinuityInput = TSPacket->Payload_Exists ? (LastContinuityInput + 1) % 16 : LastContinuityInput;
    ContinuityOffset = (ContinuityInput - NewContinuityInput) % 16;
    if (ContinuityOffset != 0)
    {
//      if (!NoContinuityCheck)
      {
        fprintf(stderr, "cNaluDumper: TS continuity mismatch (PID=%hu, pos=%lld, expect=%hhu, found=%hhu, Offset=%d)\n", VideoPID, FilePosition, NewContinuityInput, ContinuityInput, ContinuityOffset);
        AddContinuityError(VideoPID, FilePosition, NewContinuityInput, ContinuityInput);
        NaluFillState = NALU_INIT;  // experimentell! -> sollte dann auch nach Cut gesetzt werden?
        DropAllPayload = TRUE;
        SetFirstPacketAfterBreak();
//        SetTeletextBreak(FALSE);
      }
    }
//    NoContinuityCheck = FALSE;
//    if (Offset > ContinuityOffset)
//      ContinuityOffset = Offset; // max if packets get dropped, otherwise always the current one.
  }
  LastContinuityInput = ContinuityInput;

  if (TSPacket->Payload_Exists)
  {
    sPayloadInfo Info;
    bool DropThisPayload = FALSE;
    int Offset = TsPayloadOffset(TSPacket);

    if (isHDVideo)
    {
      ProcessPayload_HD(&Packet[Offset], TS_SIZE - Offset, TSPacket->Payload_Unit_Start, &Info);

      if (DropAllPayload && !Info.DropAllPayloadBytes)
      {
        // Return from drop packet mode to normal mode
        DropAllPayload = FALSE;

        // Does the packet start with some remaining NALU fill data?
        if (Info.DropPayloadStartBytes > 0)
        {
          // Add these bytes as stuffing to the adaption field.

          // Sample payload layout:
          // FF FF FF FF FF 80 00 00 01 xx xx xx xx
          //     ^DropPayloadStartBytes

          TsExtendAdaptionField(Packet, Offset - 4 + Info.DropPayloadStartBytes);
        }
      }

      DropThisPayload = DropAllPayload;
      if (!DropAllPayload && Info.DropPayloadEndBytes > 0)  // Payload ends with 0xff NALU Fill
      {
        // Last packet of useful data

        // Do early termination of NALU fill data
        if (NaluFillState == NALU_FILL)
          Packet[TS_SIZE-1] = 0x80;

        // Drop all packets AFTER this one
        DropAllPayload = TRUE;

        // Since we already wrote the 0x80, we have to make sure that
        // as soon as we stop dropping packets, any beginning NALU fill of next
        // packet gets dumped. (see DropPayloadStartBytes above)
      }
    }
    else
      ProcessPayload_SD(Packet + Offset, TS_SIZE - Offset, TSPacket->Payload_Unit_Start, &Info);

    if (DropThisPayload)
    {
      if (TSPacket->Adapt_Field_Exists)
        // Drop payload data, but keep adaption field data
        TsExtendAdaptionField(Packet, TS_SIZE-4);
      else
      {
        TRACEEXIT;
        return 1;  // Drop packet
      }
    }

    if (Info.ZerosOnly && !TSPacket->Adapt_Field_Exists && (isHDVideo || SliceState) /*&& LastEndNulls>0*/)
    {
//printf("Potential zero-byte-stuffing at position %lld", FilePosition);
      if (LastEndNulls >= 3 || PendingPacket)  // wenn 3 Nullen am Ende -> dann darf FolgePaket ohne anfangen
      {
//printf(" --> confirmed by 3 Nulls!\n");
        TRACEEXIT;
        return 2;  // Drop packet
      }
      else
      {
//printf(" --> UNSURE, pending packet!\n");
        PendingPacket = TRUE;
        Result = 3;  // Pending packet
      }
    }

    if (Result == 0)
    {
      if (PendingPacket)
      {
        if (Packet[Offset]==0 && (Packet[Offset+1]==0 || LastEndNulls>=2) && (Packet[Offset+2]==0 || LastEndNulls>=1))
        {
//printf("PENDING PACKET --> confirmed by next packet!\n");
        }
        else
        {
//printf("PENDING PACKET --> NOT confirmed, will be re-inserted!\n");
          Result = -1;  // Pending Packet doch noch einfügen
          LastContinuityOutput = PendingContinuity;
        }
        PendingPacket = FALSE;
      }

      // wird nur gesetzt für das zuletzt erhaltene Paket
      LastEndNulls = 3;
      if (Packet[TS_SIZE-3] != 0)  LastEndNulls = 2;
      if (Packet[TS_SIZE-2] != 0)  LastEndNulls = 1;
      if (Packet[TS_SIZE-1] != 0)  LastEndNulls = 0;
    }
  }
  else
  {
    // Adaptation Field ohne PCR verwerfen (experimentell!!)
    if (TSPacket->Adapt_Field_Exists && !GetPCR(Packet, NULL) /*&& (!TSPacket->Payload_Exists || TSPacket->Data[0] == 183)*/)
      return 4;
  }

  // Fix Continuity Counter and reproduce incoming offsets:
  {
    int NewContinuityOutput = ContinuityInput;
    if (LastContinuityOutput >= 0)
      NewContinuityOutput = (TSPacket->Payload_Exists) ? (LastContinuityOutput + 1) % 16 : LastContinuityOutput;
    NewContinuityOutput = (NewContinuityOutput + ContinuityOffset) % 16;
    TSPacket->ContinuityCount = NewContinuityOutput;
    if (Result == 3)
      PendingContinuity = NewContinuityOutput;
    else
      LastContinuityOutput = NewContinuityOutput;
  }

  TRACEEXIT;
  return Result;  // Keep packet
}
