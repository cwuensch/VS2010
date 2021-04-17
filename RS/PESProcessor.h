#ifndef __PESPROCESSORH__
#define __PESPROCESSORH__

typedef struct
{
  word                  PID;
  bool                  TablePacket;
  int                   ValidBuffer;    //0: no data yet, Buffer1 gets filled
                                        //1: Buffer1 is valid, Buffer2 gets filled
                                        //2: Buffer2 is valid, Buffer1 gets filled
  int                   BufferSize;
  int                   BufferPtr;
  int                   ValidBufLen;
  byte                 *pBuffer;
  byte                  LastCCCounter;
  byte                 *Buffer1, *Buffer2;
  bool                  ErrorFlag;
#ifdef _DEBUG
  int                   maxPESLen;
#endif
} tPSBuffer;


void PSBuffer_Reset(tPSBuffer *PSBuffer);
void PSBuffer_Init(tPSBuffer *PSBuffer, word PID, int BufferSize, bool TablePacket);
void PSBuffer_ProcessTSPacket(tPSBuffer *PSBuffer, tTSPacket *Packet);
void PSBuffer_DropCurBuffer(tPSBuffer *PSBuffer);

#endif
