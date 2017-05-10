/*=========================================================================
 
 Program:   H265Encoder
 Language:  C++
 
 Copyright (c) Insight Software Consortium. All rights reserved.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.
 
 =========================================================================*/
#ifndef __igtlH265Decoder_h
#define __igtlH265Decoder_h

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <vector>
#include <limits.h>
#include <string.h>
#include <fstream>
#include "igtl_types.h"
#include "igtlVideoMessage.h"
#include "igtlCodecCommonClasses.h"
#include "Lib/TLibDecoder/AnnexBread.h"
#include "Lib/TLibDecoder/NALread.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibDecoder/TDecTop.h"

namespace H265DecoderNameSpace
{
  #include "TLibCommon/TypeDef.h"
}


  class InputByteStreamNoFile
  {
    public:
    /**
     * process a binary stream. the code is modified based on InputByteStream
     *
    **/
    InputByteStreamNoFile()
    {
      m_NumFutureBytes = 0;
      m_FutureBytes = 0;
      inputStream = new igtl_uint8[0];
      streamSize = 0;
      pos = 0;
    }
    
    InputByteStreamNoFile(igtl_uint8* bitStream, int size)
    : m_NumFutureBytes(0)
    , m_FutureBytes(0)
    {
      inputStream = new igtl_uint8[size];
      memcpy(inputStream, bitStream, size);
      streamSize = size;
      pos = 0;
    }
    
    int GetPos(){return pos;};
    
    void SetByteStream(igtl_uint8* bitStream, int size)
    {
      this->reset();
      inputStream = new igtl_uint8[size];
      memcpy(inputStream, bitStream, size);
      streamSize = size;
    }
    
    void reset()
    {
      m_NumFutureBytes = 0;
      m_FutureBytes = 0;
      delete inputStream;
      inputStream = NULL;
      pos = 0;
    }
    
    /**
     * returns true if an EOF will be encountered within the next
     * n bytes.
     */
    bool eofBeforeNBytes(UInt n)
    {
      assert(n <= 4);
      if (m_NumFutureBytes >= n)
      {
        return false;
      }
      
      n -= m_NumFutureBytes;
      for (UInt i = 0; i < n; i++)
      {
        m_FutureBytes = (m_FutureBytes << 8) | inputStream[pos];
        m_NumFutureBytes++;
        pos++;
        if(pos==streamSize)
        {
          return true;
        }
      }
      return false;
    }
    
    uint32_t peekBytes(UInt n)
    {
      eofBeforeNBytes(n);
      return m_FutureBytes >> 8*(m_NumFutureBytes - n);
    }
    uint8_t readByte()
    {
      if (!m_NumFutureBytes)
      {
        uint8_t byte = inputStream[pos];
        pos++;
        return byte;
      }
      m_NumFutureBytes--;
      uint8_t wanted_byte = m_FutureBytes >> 8*m_NumFutureBytes;
      m_FutureBytes &= ~(0xff << 8*m_NumFutureBytes);
      return wanted_byte;
    }
    
    uint32_t readBytes(UInt n)
    {
      uint32_t val = 0;
      for (UInt i = 0; i < n; i++)
      {
        val = (val << 8) | readByte();
      }
      return val;
    }
  private:
    UInt m_NumFutureBytes; /* number of valid bytes in m_FutureBytes */
    uint32_t m_FutureBytes; /* bytes that have been peeked */
    igtl_uint8 * inputStream;
    int streamSize;
    int pos;
  };

  class H265Decoder:public GenericDecoder
  {
  public: 
    H265Decoder();
    ~H265Decoder();
    
    virtual int DecodeBitStreamIntoFrame(unsigned char* bitStream,igtl_uint8* outputFrame, igtl_uint32 iDimensions[2], igtl_uint64 &iStreamSize);
    
    virtual int DecodeVideoMSGIntoSingleFrame(igtl::VideoMessage* videoMessage, SourcePicture* pDecodedPic);
    
  private:
    
    virtual void ComposeByteSteam(igtl_uint8** inputData, int dimension[2], int iStride[2], igtl_uint8 *outputFrame);
    
    int ReconstructDecodedPic(TComPic* picTop, igtl_uint8* outputFrame);
    
    TDecTop* pDecoder;
    
    InputNALUnit nalu;
    
    AnnexBStats stats;
    
    TComList<TComPic*>* pcListPic;
    
    InputByteStreamNoFile* bytestream;
    
  };

#endif