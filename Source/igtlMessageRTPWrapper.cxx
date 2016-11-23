/*=========================================================================

  Program:   The OpenIGTLink Library
  Language:  C++
  Web page:  http://openigtlink.org/

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "igtlMessageRTPWrapper.h"
#include <string.h>

namespace igtl {
  
  MessageRTPWrapper::MessageRTPWrapper():Object(){
    this->SeqNum = 0;
    this->AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
    this->numberOfDataFrag = 1;
    this->numberOfDataFragToSent = 1;
    this->packedMsg = NULL;
    this->appSpecificFreq = 100;
    this->status = PaketReady;
    this->curFragLocation = 0;
  }
  
  MessageRTPWrapper::~MessageRTPWrapper()
  {
  }
  
  void MessageRTPWrapper::SetSSRC(igtl_uint32 identifier)
  {
    SSRC = identifier;
    if(igtl_is_little_endian())
    {
      SSRC = BYTE_SWAP_INT32(SSRC);
    }
  }
  
  int MessageRTPWrapper::WrapMessage(igtl_uint8* messageHead, int totMsgLen)
  {
    // Set up the RTP header:
    igtl_uint32 rtpHdr = 0x80000000; // RTP version 2;
    rtpHdr |= (RTPPayLoadType<<16);
    rtpHdr |= SeqNum; // sequence number, increment the sequence number after sending the data
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    igtl_uint32 timeIncrement = (appSpecificFreq*timeNow.tv_sec); //need to check the frequency of different application
    timeIncrement += igtl_uint32(appSpecificFreq*(timeNow.tv_usec/1.0e6)+ 0.5);
    //igtl_uint32 CSRC = 0x00000000; not used currently
    if(igtl_is_little_endian())
    {
      rtpHdr = BYTE_SWAP_INT32(rtpHdr);
      timeIncrement = BYTE_SWAP_INT32(timeIncrement);
    }
    if (status == PaketReady)
    {
      delete packedMsg;
      packedMsg = new unsigned char[RTP_PAYLOAD_LENGTH + RTP_HEADER_LENGTH];
      AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
      curFragLocation = 0;
    }
    if (status != WaitingForFragment)
    {
      this->fragmentTimeIncrement = timeIncrement;
      if (status != WaitingForAnotherMSG) // only the add header at the paket begin
      {
        memmove(packedMsg, (void *)(&rtpHdr), 4);
        memmove(packedMsg, (void *)(&timeIncrement), 4);
        memmove(packedMsg, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
      }
      if (totMsgLen <= AvailabeBytesNum)
      {
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), totMsgLen);
        AvailabeBytesNum -= totMsgLen;
        if (AvailabeBytesNum > MinimumPaketSpace)// when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        {
          status = WaitingForAnotherMSG;
        }
        else
        {
          status = PaketReady;
          SeqNum++;
        }
      }
      else if(totMsgLen > AvailabeBytesNum)
      {
        totMsgLen -= AvailabeBytesNum;
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), AvailabeBytesNum);
        status = WaitingForFragment;
        this->curFragLocation += AvailabeBytesNum;
        AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
      }
    }
    else
    {
      memmove(packedMsg, (void *)(&rtpHdr), 4);
      memmove(packedMsg, (void *)(&this->fragmentTimeIncrement), 4);
      memmove(packedMsg, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
      if (totMsgLen <= AvailabeBytesNum)
      {
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), totMsgLen);
        // when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        status = PaketReady;
      }
      else if(totMsgLen > AvailabeBytesNum)
      {
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), AvailabeBytesNum);
        status = WaitingForFragment;
        this->curFragLocation += AvailabeBytesNum;
      }
      AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
    }
    return status;
  }
  
  
  int MessageRTPWrapper::UnWrapMessage(igtl_uint8* messageHead, int totMsgLen)
  {
    if (totMsgLen<12) return -1;
    
    // Set up the RTP header:
    igtl_uint32  rtpHdr, timeIncrement;
    rtpHdr = *((igtl_uint32*)messageHead);
    //bool rtpMarkerBit = (rtpHdr&0x00800000) != 0;
    timeIncrement = *(igtl_uint32*)(messageHead+4);
    SSRC = *(igtl_uint32*)(messageHead+8);
    if(igtl_is_little_endian())
    {
      rtpHdr = BYTE_SWAP_INT32(rtpHdr);
      timeIncrement = BYTE_SWAP_INT32(timeIncrement);
      SSRC = BYTE_SWAP_INT32(SSRC);
    }
    if (status == MessageReady)
    {
      delete packedMsg;
      packedMsg = new unsigned char[RTP_PAYLOAD_LENGTH + RTP_HEADER_LENGTH];
      AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
      curFragLocation = 0;
    }
    if (status != WaitingForAnotherPaket)
    {
      this->fragmentTimeIncrement = timeIncrement;
      if (status != WaitingForAnotherMSG) // only the add header at the paket begin
      {
        memmove(packedMsg, (void *)(&rtpHdr), 4);
        memmove(packedMsg, (void *)(&timeIncrement), 4);
        memmove(packedMsg, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
      }
      if (totMsgLen <= AvailabeBytesNum)
      {
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), totMsgLen);
        AvailabeBytesNum -= totMsgLen;
        if (AvailabeBytesNum > MinimumPaketSpace)// when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        {
          status = ToUnpackAnotherMSG;
        }
        else
        {
          status = PaketReady;
          SeqNum++;
        }
      }
      else if(totMsgLen > AvailabeBytesNum)
      {
        totMsgLen -= AvailabeBytesNum;
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), AvailabeBytesNum);
        status = WaitingForFragment;
        this->curFragLocation += AvailabeBytesNum;
        AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
      }
    }
    else
    {
      memmove(packedMsg, (void *)(&rtpHdr), 4);
      memmove(packedMsg, (void *)(&this->fragmentTimeIncrement), 4);
      memmove(packedMsg, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
      if (totMsgLen <= AvailabeBytesNum)
      {
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), totMsgLen);
        // when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        status = PaketReady;
      }
      else if(totMsgLen > AvailabeBytesNum)
      {
        memmove(packedMsg + RTP_HEADER_LENGTH, (void *)(messageHead), AvailabeBytesNum);
        status = WaitingForFragment;
        this->curFragLocation += AvailabeBytesNum;
      }
      AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
    }
    return status;
  }

} // namespace igtl