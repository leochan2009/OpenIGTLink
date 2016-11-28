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
    this->curMSGLocation = 0;
    this->curPackedMSGLocation = 0;
    this->framentNumber = 0;
    this->MSGHeader= new igtl_uint8[IGTL_HEADER_SIZE];
  }
  
  MessageRTPWrapper::~MessageRTPWrapper()
  {
  }
  
  
  void MessageRTPWrapper::SetMSGHeader(igtl_uint8* header)
  {
    memcpy(this->MSGHeader, header, IGTL_HEADER_SIZE);
  }

  
  void MessageRTPWrapper::SetSSRC(igtl_uint32 identifier)
  {
    SSRC = identifier;
    if(igtl_is_little_endian())
    {
      SSRC = BYTE_SWAP_INT32(SSRC);
    }
  }
  
  void MessageRTPWrapper::SetCSRC(igtl_uint32 identifier)
  {
    CSRC = identifier;
    if(igtl_is_little_endian())
    {
      CSRC = BYTE_SWAP_INT32(CSRC);
    }
  }
  // SSRC should be set only once before wraping the message in a transmission session.
  // CSRC should be set when messages from different devices are packed into the same paket.
  // this special header is here for the compatiblity with standard RTP protocal.
  // we have our own defined openigtlink header, which is 58 byte size.
  // append the openigtlink header as the Extension header after CSRC.
  int MessageRTPWrapper::WrapMessage(igtl_uint8* messageBody, int totMsgLen)
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
      curMSGLocation = 0;
      curPackedMSGLocation = 0;
      framentNumber = 0;
    }
    if (status != WaitingForFragment)
    {
      this->fragmentTimeIncrement = timeIncrement;
      if (status != WaitingForAnotherMSG) // only add header at the paket begin
      {
        memmove(packedMsg, (void *)(&rtpHdr), 4);
        memmove(packedMsg+4, (void *)(&timeIncrement), 4);
        memmove(packedMsg+8, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
        curPackedMSGLocation += RTP_HEADER_LENGTH;
      }
      if (totMsgLen <= (AvailabeBytesNum-1-IGTL_HEADER_SIZE))
      {
        igtl_uint8 temp = 0X00;
        memmove(packedMsg+curPackedMSGLocation, (void*)&temp, 1); // no fragmented message here
        curPackedMSGLocation++;
        memmove(packedMsg+curPackedMSGLocation, this->MSGHeader, IGTL_HEADER_SIZE);
        curPackedMSGLocation += IGTL_HEADER_SIZE;
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageBody), totMsgLen);
        AvailabeBytesNum -= (totMsgLen+1+IGTL_HEADER_SIZE);
        curPackedMSGLocation += totMsgLen;
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
      else if(totMsgLen > (AvailabeBytesNum-1-IGTL_HEADER_SIZE))
      {
        igtl_uint8 temp = 0X80;
        memmove(packedMsg+curPackedMSGLocation, (void*)&temp, 1);
        // fragmented message exists, first bit indicate the existance, the second bit indicates if the current section is the ending fragment, the other 6 bits indicates the fragements' sequence number.
        curPackedMSGLocation++;
        memmove(packedMsg+curPackedMSGLocation, this->MSGHeader, IGTL_HEADER_SIZE);
        curPackedMSGLocation += IGTL_HEADER_SIZE;
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageBody), AvailabeBytesNum-1-IGTL_HEADER_SIZE);
        status = WaitingForFragment;
        this->curPackedMSGLocation = RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH;
        this->curMSGLocation = AvailabeBytesNum-1-IGTL_HEADER_SIZE;
        AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
      }
    }
    else
    {
      memmove(packedMsg, (void *)(&rtpHdr), 4);
      memmove(packedMsg+4, (void *)(&this->fragmentTimeIncrement), 4);
      memmove(packedMsg+8, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
      curPackedMSGLocation = RTP_HEADER_LENGTH;
      framentNumber++;
      if (totMsgLen <= (AvailabeBytesNum-1-IGTL_HEADER_SIZE))
      {
        igtl_uint8 temp = 0XE0+framentNumber; //set the seconde bit to be 1, indicates the end of fragmented msg.
        memmove(packedMsg+curPackedMSGLocation, (void*)(&temp), 1);
        curPackedMSGLocation++;
        memmove(packedMsg+curPackedMSGLocation, this->MSGHeader, IGTL_HEADER_SIZE);
        curPackedMSGLocation += IGTL_HEADER_SIZE;
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageBody), totMsgLen);
        this->curPackedMSGLocation += totMsgLen;
        // when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        status = PaketReady;
      }
      else if(totMsgLen > (AvailabeBytesNum-1-IGTL_HEADER_SIZE))
      {
        igtl_uint8 temp = 0X80+framentNumber;
        memmove(packedMsg+curPackedMSGLocation, (void*)(&temp), 1); //set the second bit to be 0, indicates more fragmented msg are comming.
        curPackedMSGLocation++;
        memmove(packedMsg+curPackedMSGLocation, MSGHeader, IGTL_HEADER_SIZE);
        curPackedMSGLocation += IGTL_HEADER_SIZE;
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageBody), (AvailabeBytesNum-1));
        status = WaitingForFragment;
        this->curMSGLocation += (AvailabeBytesNum-1);
      }
      AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
    }
    return status;
  }
  
  
  igtl::MessageBase::Pointer MessageRTPWrapper::UnWrapMessage(igtl_uint8* messageBody, int totMsgLen)
  {
    return NULL;
  }

} // namespace igtl