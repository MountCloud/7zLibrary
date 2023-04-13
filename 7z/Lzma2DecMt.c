/* Lzma2DecMt.c -- LZMA2 Decoder Multi-thread
2019-02-02 : Igor Pavlov : Public domain */

#include "Precomp.h"

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#define PRF_STR(s) PRF(printf("\n" s "\n"))
#define PRF_STR_INT(s, d) PRF(printf("\n" s " %d\n", (unsigned)d))
#define PRF_STR_INT_2(s, d1, d2) PRF(printf("\n" s " %d %d\n", (unsigned)d1, (unsigned)d2))

#include "Alloc.h"

#include "Lzma2Dec.h"
#include "Lzma2DecMt.h"


#define LZMA2DECMT_OUT_BLOCK_MAX_DEFAULT (1 << 28)

void Lzma2DecMtProps_Init(CLzma2DecMtProps *p)
{
  p->inBufSize_ST = 1 << 20;
  p->outStep_ST = 1 << 20;

}


/* ---------- CLzma2DecMt ---------- */

typedef struct
{
  // ISzAllocPtr alloc;
  ISzAllocPtr allocMid;

  CAlignOffsetAlloc alignOffsetAlloc;
  CLzma2DecMtProps props;
  Byte prop;
  
  ISeqInStream *inStream;
  ISeqOutStream *outStream;
  ICompressProgress *progress;

  BoolInt finishMode;
  BoolInt outSize_Defined;
  UInt64 outSize;

  UInt64 outProcessed;
  UInt64 inProcessed;
  BoolInt readWasFinished;
  SRes readRes;

  Byte *inBuf;
  size_t inBufSize;
  Byte dec_created;
  CLzma2Dec dec;

  size_t inPos;
  size_t inLim;
} CLzma2DecMt;



CLzma2DecMtHandle Lzma2DecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid)
{
  CLzma2DecMt *p = (CLzma2DecMt *)ISzAlloc_Alloc(alloc, sizeof(CLzma2DecMt));
  if (!p)
    return NULL;
  
  // p->alloc = alloc;
  p->allocMid = allocMid;

  AlignOffsetAlloc_CreateVTable(&p->alignOffsetAlloc);
  p->alignOffsetAlloc.numAlignBits = 7;
  p->alignOffsetAlloc.offset = 0;
  p->alignOffsetAlloc.baseAlloc = alloc;

  p->inBuf = NULL;
  p->inBufSize = 0;
  p->dec_created = False;

  // Lzma2DecMtProps_Init(&p->props);

  return p;
}


static void Lzma2DecMt_FreeSt(CLzma2DecMt *p)
{
  if (p->dec_created)
  {
    Lzma2Dec_Free(&p->dec, &p->alignOffsetAlloc.vt);
    p->dec_created = False;
  }
  if (p->inBuf)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBuf = NULL;
  }
  p->inBufSize = 0;
}


void Lzma2DecMt_Destroy(CLzma2DecMtHandle pp)
{
  CLzma2DecMt *p = (CLzma2DecMt *)pp;

  Lzma2DecMt_FreeSt(p);

  ISzAlloc_Free(p->alignOffsetAlloc.baseAlloc, pp);
}


static SRes Lzma2Dec_Prepare_ST(CLzma2DecMt *p)
{
  if (!p->dec_created)
  {
    Lzma2Dec_Construct(&p->dec);
    p->dec_created = True;
  }

  RINOK(Lzma2Dec_Allocate(&p->dec, p->prop, &p->alignOffsetAlloc.vt));

  if (!p->inBuf || p->inBufSize != p->props.inBufSize_ST)
  {
    ISzAlloc_Free(p->allocMid, p->inBuf);
    p->inBufSize = 0;
    p->inBuf = (Byte *)ISzAlloc_Alloc(p->allocMid, p->props.inBufSize_ST);
    if (!p->inBuf)
      return SZ_ERROR_MEM;
    p->inBufSize = p->props.inBufSize_ST;
  }

  Lzma2Dec_Init(&p->dec);
  
  return SZ_OK;
}


static SRes Lzma2Dec_Decode_ST(CLzma2DecMt *p)
{
  SizeT wrPos;
  size_t inPos, inLim;
  const Byte *inData;
  UInt64 inPrev, outPrev;

  CLzma2Dec *dec;

  RINOK(Lzma2Dec_Prepare_ST(p));

  dec = &p->dec;

  inPrev = p->inProcessed;
  outPrev = p->outProcessed;

  inPos = 0;
  inLim = 0;
  inData = NULL;
  wrPos = dec->decoder.dicPos;

  for (;;)
  {
    SizeT dicPos;
    SizeT size;
    ELzmaFinishMode finishMode;
    SizeT inProcessed;
    ELzmaStatus status;
    SRes res;

    SizeT outProcessed;
    BoolInt outFinished;
    BoolInt needStop;

    if (inPos == inLim)
    {
      
      if (!p->readWasFinished)
      {
        inPos = 0;
        inLim = p->inBufSize;
        inData = p->inBuf;
        p->readRes = ISeqInStream_Read(p->inStream, (void *)inData, &inLim);
        // p->readProcessed += inLim;
        // inLim -= 5; p->readWasFinished = True; // for test
        if (inLim == 0 || p->readRes != SZ_OK)
          p->readWasFinished = True;
      }
    }

    dicPos = dec->decoder.dicPos;
    {
      SizeT next = dec->decoder.dicBufSize;
      if (next - wrPos > p->props.outStep_ST)
        next = wrPos + p->props.outStep_ST;
      size = next - dicPos;
    }

    finishMode = LZMA_FINISH_ANY;
    if (p->outSize_Defined)
    {
      const UInt64 rem = p->outSize - p->outProcessed;
      if (size >= rem)
      {
        size = (SizeT)rem;
        if (p->finishMode)
          finishMode = LZMA_FINISH_END;
      }
    }

    inProcessed = inLim - inPos;
    
    res = Lzma2Dec_DecodeToDic(dec, dicPos + size, inData + inPos, &inProcessed, finishMode, &status);

    inPos += inProcessed;
    p->inProcessed += inProcessed;
    outProcessed = dec->decoder.dicPos - dicPos;
    p->outProcessed += outProcessed;

    outFinished = (p->outSize_Defined && p->outSize <= p->outProcessed);

    needStop = (res != SZ_OK
        || (inProcessed == 0 && outProcessed == 0)
        || status == LZMA_STATUS_FINISHED_WITH_MARK
        || (!p->finishMode && outFinished));

    if (needStop || outProcessed >= size)
    {
      SRes res2;
      {
        size_t writeSize = dec->decoder.dicPos - wrPos;
        size_t written = ISeqOutStream_Write(p->outStream, dec->decoder.dic + wrPos, writeSize);
        res2 = (written == writeSize) ? SZ_OK : SZ_ERROR_WRITE;
      }

      if (dec->decoder.dicPos == dec->decoder.dicBufSize)
        dec->decoder.dicPos = 0;
      wrPos = dec->decoder.dicPos;

      RINOK(res2);

      if (needStop)
      {
        if (res != SZ_OK)
          return res;

        if (status == LZMA_STATUS_FINISHED_WITH_MARK)
        {
          if (p->finishMode)
          {
            if (p->outSize_Defined && p->outSize != p->outProcessed)
              return SZ_ERROR_DATA;
          }
          return SZ_OK;
        }

        if (!p->finishMode && outFinished)
          return SZ_OK;

        if (status == LZMA_STATUS_NEEDS_MORE_INPUT)
          return SZ_ERROR_INPUT_EOF;
        
        return SZ_ERROR_DATA;
      }
    }
    
    if (p->progress)
    {
      UInt64 inDelta = p->inProcessed - inPrev;
      UInt64 outDelta = p->outProcessed - outPrev;
      if (inDelta >= (1 << 22) || outDelta >= (1 << 22))
      {
        RINOK(ICompressProgress_Progress(p->progress, p->inProcessed, p->outProcessed));
        inPrev = p->inProcessed;
        outPrev = p->outProcessed;
      }
    }
  }
}



SRes Lzma2DecMt_Decode(CLzma2DecMtHandle pp,
    Byte prop,
    const CLzma2DecMtProps *props,
    ISeqOutStream *outStream, const UInt64 *outDataSize, int finishMode,
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStream *inStream,
    // const Byte *inData, size_t inDataSize,
    UInt64 *inProcessed,
    // UInt64 *outProcessed,
    int *isMT,
    ICompressProgress *progress)
{
  CLzma2DecMt *p = (CLzma2DecMt *)pp;

  *inProcessed = 0;

  if (prop > 40)
    return SZ_ERROR_UNSUPPORTED;

  p->prop = prop;
  p->props = *props;

  p->inStream = inStream;
  p->outStream = outStream;
  p->progress = progress;

  p->outSize = 0;
  p->outSize_Defined = False;
  if (outDataSize)
  {
    p->outSize_Defined = True;
    p->outSize = *outDataSize;
  }
  p->finishMode = finishMode;

  p->outProcessed = 0;
  p->inProcessed = 0;

  p->readWasFinished = False;

  *isMT = False;

  {
    SRes res = Lzma2Dec_Decode_ST(p);

    *inProcessed = p->inProcessed;

    // res = SZ_OK; // for test
    if (res == SZ_OK && p->readRes != SZ_OK)
      res = p->readRes;
    
    return res;
  }
}


/* ---------- Read from CLzma2DecMtHandle Interface ---------- */

SRes Lzma2DecMt_Init(CLzma2DecMtHandle pp,
    Byte prop,
    const CLzma2DecMtProps *props,
    const UInt64 *outDataSize, int finishMode,
    ISeqInStream *inStream)
{
  CLzma2DecMt *p = (CLzma2DecMt *)pp;

  if (prop > 40)
    return SZ_ERROR_UNSUPPORTED;

  p->prop = prop;
  p->props = *props;

  p->inStream = inStream;

  p->outSize = 0;
  p->outSize_Defined = False;
  if (outDataSize)
  {
    p->outSize_Defined = True;
    p->outSize = *outDataSize;
  }
  p->finishMode = finishMode;

  p->outProcessed = 0;
  p->inProcessed = 0;

  p->inPos = 0;
  p->inLim = 0;

  return Lzma2Dec_Prepare_ST(p);
}


SRes Lzma2DecMt_Read(CLzma2DecMtHandle pp,
    Byte *data, size_t *outSize,
    UInt64 *inStreamProcessed)
{
  CLzma2DecMt *p = (CLzma2DecMt *)pp;
  ELzmaFinishMode finishMode;
  SRes readRes;
  size_t size = *outSize;

  *outSize = 0;
  *inStreamProcessed = 0;

  finishMode = LZMA_FINISH_ANY;
  if (p->outSize_Defined)
  {
    const UInt64 rem = p->outSize - p->outProcessed;
    if (size >= rem)
    {
      size = (size_t)rem;
      if (p->finishMode)
        finishMode = LZMA_FINISH_END;
    }
  }

  readRes = SZ_OK;

  for (;;)
  {
    SizeT inCur;
    SizeT outCur;
    ELzmaStatus status;
    SRes res;

    if (p->inPos == p->inLim && readRes == SZ_OK)
    {
      p->inPos = 0;
      p->inLim = p->props.inBufSize_ST;
      readRes = ISeqInStream_Read(p->inStream, p->inBuf, &p->inLim);
    }

    inCur = p->inLim - p->inPos;
    outCur = size;

    res = Lzma2Dec_DecodeToBuf(&p->dec, data, &outCur,
        p->inBuf + p->inPos, &inCur, finishMode, &status);
    
    p->inPos += inCur;
    p->inProcessed += inCur;
    *inStreamProcessed += inCur;
    p->outProcessed += outCur;
    *outSize += outCur;
    size -= outCur;
    data += outCur;
    
    if (res != 0)
      return res;
    
    /*
    if (status == LZMA_STATUS_FINISHED_WITH_MARK)
      return readRes;

    if (size == 0 && status != LZMA_STATUS_NEEDS_MORE_INPUT)
    {
      if (p->finishMode && p->outSize_Defined && p->outProcessed >= p->outSize)
        return SZ_ERROR_DATA;
      return readRes;
    }
    */

    if (inCur == 0 && outCur == 0)
      return readRes;
  }
}
