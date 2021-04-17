#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "type.h"
#include "RecStrip.h"
#include "PESProcessor.h"


//------ TS to PS converter
void PSBuffer_Reset(tPSBuffer *PSBuffer)
{
  TRACEENTER;
  if(PSBuffer)
  {
    if(PSBuffer->Buffer1)
    {
      free(PSBuffer->Buffer1);
      PSBuffer->Buffer1 = NULL;
    }

    if(PSBuffer->Buffer2)
    {
      free(PSBuffer->Buffer2);
      PSBuffer->Buffer2 = NULL;
    }

    memset(PSBuffer, 0, sizeof(tPSBuffer));
  }
  TRACEEXIT;
}

void PSBuffer_Init(tPSBuffer *PSBuffer, word PID, int BufferSize, bool TablePacket)
{
  TRACEENTER;

  memset(PSBuffer, 0, sizeof(tPSBuffer));
  PSBuffer->PID = PID;
  PSBuffer->TablePacket = TablePacket;
  PSBuffer->BufferSize = BufferSize;

  PSBuffer->Buffer1 = (byte*) malloc(BufferSize);
  PSBuffer->Buffer2 = (byte*) malloc(BufferSize);
  PSBuffer->pBuffer = PSBuffer->Buffer1;
  PSBuffer->LastCCCounter = 255;

  TRACEEXIT;
}

void PSBuffer_ProcessTSPacket(tPSBuffer *PSBuffer, tTSPacket *Packet)
{
  int                   RemainingBytes = 0, Start = 0;
  bool                  PESStart = Packet->Payload_Unit_Start;
  
  TRACEENTER;

  //Stimmt die PID?
  if((Packet->SyncByte == 'G') && ((Packet->PID1 *256) + Packet->PID2 == PSBuffer->PID) && Packet->Payload_Exists)
  {
    //Continuity Counter ok? Falls nicht Buffer komplett verwerfen
    if((PSBuffer->LastCCCounter != 255) && (Packet->ContinuityCount != ((PSBuffer->LastCCCounter + 1) % 16)))
    {
      //Unerwarteter Continuity Counter, Daten verwerfen
      if(PSBuffer->LastCCCounter != 255)
      {
        printf("  PESProcessor: CC error while parsing PID %hu\n", PSBuffer->PID);
        PSBuffer_DropCurBuffer(PSBuffer);
      }
    }
        
    //Adaptation field gibt es nur bei PES Paketen
    if(Packet->Adapt_Field_Exists)
    {
      if(Packet->Data[0] > 183)
      {
        printf("  PESProcessor: Illegal Adaptation field length (%hu) on PID %hu\n", Packet->Data[0], PSBuffer->PID);
        PSBuffer->ErrorFlag = TRUE;
        Start = 184;
      }
      else
        Start += (1 + Packet->Data[0]);  // CW (Längen-Byte zählt ja auch noch mit!)

      // Liegt ein DiscontinueFlag vor?
      if ((Packet->Data[0] > 0) && ((Packet->Data[1] & 0x80) > 0))
      {
        printf("  PESProcessor: Discontinuity flag while parsing PID %hu\n", PSBuffer->PID);
        PSBuffer_DropCurBuffer(PSBuffer);
      }
    }

    //Startet ein neues PES-Paket?
    if(Packet->Payload_Unit_Start)
    {
      // PES-Packet oder Table?
      if (PSBuffer->TablePacket)
      {
        if (Start < 184)
        {
          RemainingBytes = Packet->Data[Start];
          Start++;
        }
      }
      else
      {
        for (RemainingBytes = 0; RemainingBytes < 184-Start-2; RemainingBytes++)
          if (Packet->Data[Start+RemainingBytes]==0 && Packet->Data[Start+RemainingBytes+1]==0 && Packet->Data[Start+RemainingBytes+2]==1)
            break;

        if (RemainingBytes == 184-Start-2)
        {
          if (Packet->Data[Start+RemainingBytes+1] == 0)
          {
            if (Packet->Data[Start+RemainingBytes] != 0)  RemainingBytes++;
          }
          else
            RemainingBytes += 2;
        }
      }

      if (RemainingBytes < 184 - Start)
      {
        //Restliche Bytes umkopieren und neuen Buffer beginnen
        if(PSBuffer->BufferPtr != 0)
        {
          if(RemainingBytes != 0)
          {
            if(PSBuffer->BufferPtr + RemainingBytes <= PSBuffer->BufferSize)
            {
              memcpy(PSBuffer->pBuffer, &Packet->Data[Start], RemainingBytes);
              PSBuffer->pBuffer += RemainingBytes;
              PSBuffer->BufferPtr += RemainingBytes;
            }
            else
            {
              printf("  PESProcessor: PS buffer overflow while parsing PID %hu\n", PSBuffer->PID);
              PSBuffer->ErrorFlag = TRUE;
            }
          }

#ifdef _DEBUG
          if(PSBuffer->BufferPtr > PSBuffer->maxPESLen)
            PSBuffer->maxPESLen = PSBuffer->BufferPtr;
#endif

          //Puffer mit den abfragbaren Daten markieren
          PSBuffer->ValidBuffer = (PSBuffer->ValidBuffer % 2) + 1;  // 0 und 2 -> 1, 1 -> 2
          PSBuffer->ValidBufLen = PSBuffer->BufferPtr;

          //Neuen Puffer aktivieren
          switch(PSBuffer->ValidBuffer)
          {
            case 0:
            case 2: PSBuffer->pBuffer = PSBuffer->Buffer1; break;
            case 1: PSBuffer->pBuffer = PSBuffer->Buffer2; break;
          }
          PSBuffer->BufferPtr = 0;
        }
      }
      else
      {
        printf("  PESProcessor: TS packet is marked as Payload Start, but does not contain a start code (PID %hu)\n", PSBuffer->PID);
        PESStart = FALSE;
        RemainingBytes = 0;
      }
    }

    if (PESStart)
    {
      //Erste Daten kopieren
      memset(PSBuffer->pBuffer, 0, PSBuffer->BufferSize);
      memcpy(PSBuffer->pBuffer, &Packet->Data[Start+RemainingBytes], 184-Start-RemainingBytes);
      PSBuffer->pBuffer += (184-Start-RemainingBytes);
      PSBuffer->BufferPtr += (184-Start-RemainingBytes);
    }
    else
    {
      //Weiterkopieren
      if(PSBuffer->BufferPtr != 0)
      {
        if(PSBuffer->BufferPtr + 184-Start <= PSBuffer->BufferSize)
        {
          memcpy(PSBuffer->pBuffer, &Packet->Data[Start], 184-Start);
          PSBuffer->pBuffer += (184-Start);
          PSBuffer->BufferPtr += (184-Start);
        }
        else
        {
          printf("  PESProcessor: PS buffer overflow while parsing PID %hu\n", PSBuffer->PID);
          PSBuffer->ErrorFlag = TRUE;
        }
      }
    }

    PSBuffer->LastCCCounter = Packet->ContinuityCount;
  }
  TRACEEXIT;
}

void PSBuffer_DropCurBuffer(tPSBuffer *PSBuffer)
{
  TRACEENTER;
  switch(PSBuffer->ValidBuffer)
  {
    case 0:
    case 1: PSBuffer->pBuffer = PSBuffer->Buffer1; break;
    case 2: PSBuffer->pBuffer = PSBuffer->Buffer2; break;
  }
  memset(PSBuffer->pBuffer, 0, PSBuffer->BufferSize);
  PSBuffer->BufferPtr = 0;
  PSBuffer->LastCCCounter = 255;
  TRACEEXIT;
}
