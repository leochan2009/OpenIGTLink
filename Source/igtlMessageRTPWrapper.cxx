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
    this->extendedHeaderSize = sizeof(igtl_extended_header);
    this->glock = igtl::SimpleMutexLock::New();
    this->incommingPakets =  PaketBuffer::PaketBuffer();
    this->reorderBuffer = new igtl::ReorderBuffer();
    this->reorderBufferMap = std::map<igtl_uint32, igtl::ReorderBuffer*>();
  }
  
  MessageRTPWrapper::~MessageRTPWrapper()
  {
    glock->Lock();
    std::map<igtl_uint32, igtl::ReorderBuffer*>::iterator itr = this->reorderBufferMap.begin();
    while (itr != this->reorderBufferMap.end())
    {
      // found it - delete it
      itr->second->~ReorderBuffer();
      this->reorderBufferMap.erase(itr);
      itr++;
    }
    std::map<igtl_uint32, igtl::UnWrappedMessage*>::iterator itr2 = this->unWrappedMessages.begin();
    while (itr2 != this->unWrappedMessages.end())
    {
      // found it - delete it
      itr2->second->~UnWrappedMessage();
      this->unWrappedMessages.erase(itr2);
      itr2++;
    }
    glock->Unlock();
    glock = NULL;
  }
  
  
  void MessageRTPWrapper::SetMSGHeader(igtl_uint8* header)
  {
    memcpy(this->MSGHeader, header, IGTL_HEADER_SIZE+this->extendedHeaderSize);
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
  
  void MessageRTPWrapper::SetSeqNum(igtl_uint16 num)
  {
    SeqNum = num;
    if(igtl_is_little_endian())
    {
      SeqNum = BYTE_SWAP_INT16(num);
    }
  }
  
  int MessageRTPWrapper::PushDataIntoPaketBuffer(igtlUint8* UDPPaket, igtlUint16 paketLen)
  {
    this->glock->Lock();
    this->incommingPakets.pPaketLengthInByte.push_back(paketLen);
    this->incommingPakets.pBsBuf.insert(this->incommingPakets.pBsBuf.end(), UDPPaket, UDPPaket+paketLen);
    this->incommingPakets.totalLength += paketLen;
    if (this->incommingPakets.totalLength  == this->incommingPakets.pBsBuf.size())
    {
      this->glock->Unlock();
      return 1;
      
    }
    else
    {
      this->glock->Unlock();
      return 0;
    }
    
  }
  
  
  
  int MessageRTPWrapper::UnWrapPaketWithTypeAndName(const char *deviceType, const char * deviceName)
  {
    int extendedHeaderLength = sizeof(igtl_extended_header);
    igtl::MessageFactory::Pointer MSGFactory = igtl::MessageFactory::New();
    if (this->incommingPakets.pPaketLengthInByte.size())
    {
      this->glock->Lock();
      igtlUint16 totMsgLen = this->incommingPakets.pPaketLengthInByte[0];
      igtlUint8 * UDPPaket = new igtlUint8[totMsgLen];
      memcpy(UDPPaket, this->incommingPakets.pBsBuf.data(), totMsgLen);
      this->incommingPakets.pPaketLengthInByte.erase(this->incommingPakets.pPaketLengthInByte.begin());
      this->incommingPakets.pBsBuf.erase(this->incommingPakets.pBsBuf.begin(), this->incommingPakets.pBsBuf.begin()+totMsgLen);
      this->incommingPakets.totalLength -= totMsgLen;
      this->glock->Unlock();
      // Set up the RTP header:
      igtl_uint32  rtpHdr, timeIncrement;
      rtpHdr = *((igtl_uint32*)UDPPaket);
      //bool rtpMarkerBit = (rtpHdr&0x00800000) != 0;
      timeIncrement = *(igtl_uint32*)(UDPPaket+4);
      igtl_uint32 SSRC = *(igtl_uint32*)(UDPPaket+8);
      if(igtl_is_little_endian())
      {
        rtpHdr = BYTE_SWAP_INT32(rtpHdr);
        timeIncrement = BYTE_SWAP_INT32(timeIncrement);
        SSRC = BYTE_SWAP_INT32(SSRC);
      }
      int curPackedMSGLocation = RTP_HEADER_LENGTH;
      status = WaitingForAnotherPaket;
      while(curPackedMSGLocation<totMsgLen)
      {
        igtl::MessageHeader::Pointer header = igtl::MessageHeader::New();
        header->AllocatePack();
        memcpy(header->GetPackPointer(), UDPPaket + curPackedMSGLocation, IGTL_HEADER_SIZE);
        igtl_uint16 fragmentNumber;
        memcpy(&fragmentNumber, (void*)(UDPPaket + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength-2),2);
        igtl_uint32 messageID;
        memcpy(&messageID, (void*)(UDPPaket + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength-6),4);
        if(igtl_is_little_endian())
        {
          fragmentNumber = BYTE_SWAP_INT16(fragmentNumber);
          messageID = BYTE_SWAP_INT32(messageID);
        }
        //this->reorderBufferMap.erase(it);
        std::map<igtl_uint32, igtl::ReorderBuffer*>::iterator it = this->reorderBufferMap.find(messageID);
        if (it == this->reorderBufferMap.end())
        {
          this->reorderBufferMap.insert(std::pair<igtl_uint32,igtl::ReorderBuffer*>(messageID,new igtl::ReorderBuffer()));
          it = this->reorderBufferMap.find(messageID);
          
        }
        this->reorderBuffer = it->second;
        header->Unpack();
        if(fragmentNumber==0X0000) // fragment doesn't exist
        {
          
          if (strcmp(header->GetDeviceType(),deviceType)==0 && strcmp(header->GetDeviceName(),deviceName)==0)
          {
            memcpy(this->reorderBuffer->firstFragBuffer, UDPPaket + curPackedMSGLocation, header->GetBodySizeToRead()+IGTL_HEADER_SIZE);
            this->reorderBuffer->filledPaketNum = 1;
            reorderBuffer->receivedFirstFrag = true;
            reorderBuffer->receivedLastFrag = true;
            reorderBuffer->firstPaketLen = header->GetBodySizeToRead()+IGTL_HEADER_SIZE;
            igtl::UnWrappedMessage* message = new igtl::UnWrappedMessage();
            message->messageDataLength = reorderBuffer->firstPaketLen;
            memcpy(message->messagePackPointer, reorderBuffer->firstFragBuffer, reorderBuffer->firstPaketLen);
            glock->Lock();
            unWrappedMessages.insert(std::pair<igtl_uint32, igtl::UnWrappedMessage*>(it->first,message));
            glock->Unlock();
            this->reorderBufferMap.erase(it);
            delete it->second;
            status = MessageReady;
          }
          curPackedMSGLocation += header->GetBodySizeToRead()+IGTL_HEADER_SIZE;
        }
        else
        {
          if (strcmp(header->GetDeviceType(),deviceType)==0 && strcmp(header->GetDeviceName(),deviceName)==0)
          {
            int bodyMsgLength = (RTP_PAYLOAD_LENGTH-IGTL_HEADER_SIZE-extendedHeaderLength);//this is the length of the body within a full fragment paket
            if(fragmentNumber==0X8000)// To do, fix the issue when later fragment arrives earlier than the beginning fragment
            {
              *(UDPPaket + curPackedMSGLocation + IGTL_HEADER_SIZE+extendedHeaderLength-2) = 0X0000; // set the fragment no. to 0000
              memcpy(reorderBuffer->firstFragBuffer, UDPPaket + curPackedMSGLocation, totMsgLen-curPackedMSGLocation);
              reorderBuffer->firstPaketLen = totMsgLen-curPackedMSGLocation;
              curPackedMSGLocation = totMsgLen;
              reorderBuffer->receivedFirstFrag = true;
              status = WaitingForAnotherPaket;
            }
            else if(fragmentNumber>=0XE000)// this is the last fragment
            {
              reorderBuffer->totFragNumber = fragmentNumber - 0XE000 + 1;
              memcpy(reorderBuffer->lastFragBuffer, UDPPaket + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength, totMsgLen-(RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength));
              reorderBuffer->receivedLastFrag = true;
              reorderBuffer->lastPaketLen = totMsgLen-(RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength);
              status = WaitingForAnotherPaket;
            }
            else if(fragmentNumber>0X8000 && fragmentNumber<0XE000)
            {
              int curFragNumber = fragmentNumber - 0X8000;
              memcpy(reorderBuffer->buffer+(curFragNumber-1)*bodyMsgLength, UDPPaket + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength, totMsgLen-(RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength));
              status = WaitingForAnotherPaket;
            }
            reorderBuffer->filledPaketNum++;
            if(reorderBuffer->receivedFirstFrag == true && reorderBuffer->receivedLastFrag == true && reorderBuffer->filledPaketNum == reorderBuffer->totFragNumber)
            {
              igtl::UnWrappedMessage* message = new igtl::UnWrappedMessage();
              message->messageDataLength = reorderBuffer->lastPaketLen+reorderBuffer->firstPaketLen+ (reorderBuffer->totFragNumber-2)*RTP_PAYLOAD_LENGTH;
              memcpy(message->messagePackPointer, reorderBuffer->firstFragBuffer, reorderBuffer->firstPaketLen);
              memcpy(message->messagePackPointer+reorderBuffer->firstPaketLen, reorderBuffer->buffer, (RTP_PAYLOAD_LENGTH-IGTL_HEADER_SIZE-extendedHeaderLength)*(reorderBuffer->totFragNumber-2));
              memcpy(message->messagePackPointer+reorderBuffer->firstPaketLen+(RTP_PAYLOAD_LENGTH-IGTL_HEADER_SIZE-extendedHeaderLength)*(reorderBuffer->totFragNumber-2), reorderBuffer->lastFragBuffer, reorderBuffer->lastPaketLen);
              glock->Lock();
              unWrappedMessages.insert(std::pair<igtl_uint32, igtl::UnWrappedMessage*>(it->first,message));
              glock->Unlock();
              this->reorderBufferMap.erase(it);
              delete it->second;
              status = MessageReady;
            }
          }
          else
          {
            curPackedMSGLocation = totMsgLen;
          }
          break;
        }
      }
      delete[] UDPPaket;
    }
    return 1;
  }
  
  int MessageRTPWrapper::WrapMessageAndSend(igtl::UDPServerSocket::Pointer &socket, igtl_uint8* messagePackPointer, int msgtotalLen)
  {
    igtl_uint8* messageContentPointer = messagePackPointer+IGTL_HEADER_SIZE+sizeof(igtl_extended_header);
    this->SetMSGHeader((igtl_uint8*)messagePackPointer);
    int MSGContentLength = msgtotalLen- IGTL_HEADER_SIZE-sizeof(igtl_extended_header); // this is the m_content size + meta data size
    int leftMsgLen = MSGContentLength;
    igtl_uint8* leftmessageContent = messageContentPointer;
    do
    {
      status = this->WrapMessage(leftmessageContent, leftMsgLen);
      
      if (status == igtl::MessageRTPWrapper::WaitingForFragment || status == igtl::MessageRTPWrapper::PaketReady)
      {
        this->glock->Lock();
        socket->WriteSocket(this->GetPackPointer(), this->GetPackedMSGLocation());
        this->glock->Unlock();
      }
      leftmessageContent = messageContentPointer + this->GetCurMSGLocation();
      leftMsgLen = MSGContentLength - this->GetCurMSGLocation();
    }while(leftMsgLen>0 && status!=igtl::MessageRTPWrapper::PaketReady); // to do when bodyMsgLen
    return 1;
  }
  
  
  // SSRC should be set only once before wraping the message in a transmission session.
  // CSRC should be set when messages from different devices are packed into the same paket.
  // this special header is here for the compatiblity with standard RTP protocal.
  // we have our own defined openigtlink header, which is 58 byte size.
  // append the openigtlink header as the Extension header after CSRC.
  int MessageRTPWrapper::WrapMessage(igtl_uint8* messageContent, int bodyMsgLen)
  {
    // Set up the RTP header:
    igtl_uint32 rtpHdr = 0x80000000; // RTP version 2;
    rtpHdr |= (RTPPayLoadType<<16);
    rtpHdr |= SeqNum; // sequence number, increment the sequence number after sending the data
#if defined(WIN32) || defined(_WIN32)

                      //LARGE_INTEGER tick;
                      //
                      //::QueryPerformanceCounter( &tick );
                      //
                      //TimeStampType value = 
                      //    static_cast< TimeStampType >( (__int64)tick.QuadPart ) / 
                      //    this->m_WinFrequency;
                      //
                      //value += this->m_WinOrigin;
                      //
                      //double second = floor(value);

    clock_t c1 = clock();
    clock_t winClockOrigin = time(NULL);
    clock_t second =  + (c1 - winClockOrigin) / CLOCKS_PER_SEC;
    clock_t microsecond = (c1 - winClockOrigin) % CLOCKS_PER_SEC * (1e6 / CLOCKS_PER_SEC);
    clock_t timeIncrement = appSpecificFreq*second;
    timeIncrement += igtl_uint32(appSpecificFreq*(microsecond / 1.0e6) + 0.5);
#else
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    igtl_uint32 timeIncrement = (appSpecificFreq*timeNow.tv_sec); //need to check the frequency of different application
    timeIncrement += igtl_uint32(appSpecificFreq*(timeNow.tv_usec/1.0e6)+ 0.5);
#endif
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
      if (bodyMsgLen <= (AvailabeBytesNum-IGTL_HEADER_SIZE-this->extendedHeaderSize))
      {
        igtl_uint16 temp = 0X0000;
        memmove(this->MSGHeader+IGTL_HEADER_SIZE+this->extendedHeaderSize-2, (void*)&temp,2);// no fragmented message here
        memmove(packedMsg+curPackedMSGLocation, this->MSGHeader, IGTL_HEADER_SIZE+this->extendedHeaderSize);
        curPackedMSGLocation += (IGTL_HEADER_SIZE + this->extendedHeaderSize);
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageContent), bodyMsgLen);
        AvailabeBytesNum -= (bodyMsgLen+this->extendedHeaderSize+IGTL_HEADER_SIZE);
        curPackedMSGLocation += bodyMsgLen;
        curMSGLocation += bodyMsgLen;
        if (AvailabeBytesNum > MinimumPaketSpace)// when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        {
          status = PaketReady;//WaitingForAnotherMSG; // now we send the paket immediately, however, it is possible to send after the packt is full. just uncomment the WaitingForAnotherMSG
        }
        else
        {
          status = PaketReady;
          SeqNum++;
        }
      }
      else if(bodyMsgLen > (AvailabeBytesNum-IGTL_HEADER_SIZE-this->extendedHeaderSize))
      {
        igtl_uint16 temp = 0X8000;
        if(igtl_is_little_endian())
        {
          temp = BYTE_SWAP_INT16(temp);
        }
        memmove(this->MSGHeader+IGTL_HEADER_SIZE+this->extendedHeaderSize-2, (void*)&temp,2);// no fragmented message here
        // fragmented message exists, first bit indicate the existance, the second bit indicates if the current section is the ending fragment, the other 14 bits indicates the fragements' sequence number.
        memmove(packedMsg+curPackedMSGLocation, this->MSGHeader, IGTL_HEADER_SIZE+this->extendedHeaderSize);
        curPackedMSGLocation += (IGTL_HEADER_SIZE + this->extendedHeaderSize);
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageContent), AvailabeBytesNum-this->extendedHeaderSize-IGTL_HEADER_SIZE);
        status = WaitingForFragment;
        this->curPackedMSGLocation = RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH;
        this->curMSGLocation = AvailabeBytesNum-IGTL_HEADER_SIZE-this->extendedHeaderSize;
        AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
        SeqNum++;
        framentNumber++;
      }
    }
    else
    {
      memmove(packedMsg, (void *)(&rtpHdr), 4);
      memmove(packedMsg+4, (void *)(&this->fragmentTimeIncrement), 4);
      memmove(packedMsg+8, (void *)(&SSRC), 4); // SSRC needs to set by different devices, collision should be avoided.
      curPackedMSGLocation = RTP_HEADER_LENGTH;
      if (bodyMsgLen <= (AvailabeBytesNum-IGTL_HEADER_SIZE-this->extendedHeaderSize))
      {
        igtl_uint16 temp = 0XE000+framentNumber; //set the seconde bit to be 1, indicates the end of fragmented msg.;
        if(igtl_is_little_endian())
        {
          temp = BYTE_SWAP_INT16(temp);
        }
        memmove(this->MSGHeader+IGTL_HEADER_SIZE+this->extendedHeaderSize-2, (void*)&temp,2);
        // fragmented message exists, first bit indicate the existance, the second bit indicates if the current section is the ending fragment, the other 14 bits indicates the fragements' sequence number.
        memmove(packedMsg+curPackedMSGLocation, this->MSGHeader, IGTL_HEADER_SIZE+this->extendedHeaderSize);
        curPackedMSGLocation += (IGTL_HEADER_SIZE + this->extendedHeaderSize);
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageContent), bodyMsgLen);
        this->curPackedMSGLocation += bodyMsgLen;
        // when it is packing the fragment, we want to sent the data ASAP, otherwize, we will wait for another message
        status = PaketReady;
      }
      else if(bodyMsgLen > (AvailabeBytesNum-IGTL_HEADER_SIZE-this->extendedHeaderSize))
      {
        igtl_uint16 temp = 0X8000+framentNumber; //set the seconde bit to be 1, indicates the end of fragmented msg.;
        if(igtl_is_little_endian())
        {
          temp = BYTE_SWAP_INT16(temp);
        }
        memmove(this->MSGHeader+IGTL_HEADER_SIZE+this->extendedHeaderSize-2, (void*)&temp,2);//set the second bit to be 0, indicates more fragmented msg are comming.
        memmove(packedMsg+curPackedMSGLocation, MSGHeader, IGTL_HEADER_SIZE+this->extendedHeaderSize);
        curPackedMSGLocation += (IGTL_HEADER_SIZE + this->extendedHeaderSize);
        memmove(packedMsg + curPackedMSGLocation, (void *)(messageContent), AvailabeBytesNum-this->extendedHeaderSize-IGTL_HEADER_SIZE);
        status = WaitingForFragment;
        this->curMSGLocation += (AvailabeBytesNum-this->extendedHeaderSize-IGTL_HEADER_SIZE);
        curPackedMSGLocation += (AvailabeBytesNum-this->extendedHeaderSize-IGTL_HEADER_SIZE);
      }
      AvailabeBytesNum = RTP_PAYLOAD_LENGTH;
      SeqNum++;
      framentNumber++;
    }
    return status;
  }
  
  
  igtl::MessageBase::Pointer MessageRTPWrapper::UnWrapMessage(igtl_uint8* messageContent, int bodyMsgLen)
  {
    return NULL;
  }

} // namespace igtl