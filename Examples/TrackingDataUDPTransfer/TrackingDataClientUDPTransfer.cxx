/*=========================================================================

  Program:   Open IGT Link -- Example for Tracker Client Program
  Module:    $RCSfile: $
  Language:  C++
  Date:      $Date: $
  Version:   $Revision: $

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <iostream>
#include <math.h>
#include <cstdlib>
#include <cstring>

#include <string.h>

#include "igtlOSUtil.h"
#include "igtlTrackingDataMessage.h"
#include "igtlMessageRTPWrapper.h"
#include "igtlUDPClientSocket.h"

class ReorderBuffer
{
public:
  ReorderBuffer(){firstPaketPos=0;filledPaketNum=0;receivedLastFrag=false;receivedFirstFrag==false;};
  ~ReorderBuffer(){};
  unsigned char buffer[RTP_PAYLOAD_LENGTH*64];  // we use 6 bits for fragment number.
  uint32_t firstPaketPos;
  uint32_t filledPaketNum;
  bool receivedLastFrag;
  bool receivedFirstFrag;
};

int ReceiveTrackingData(igtl::TrackingDataMessage::Pointer& msgData);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 4) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <hostname> <port> <fps>"    << std::endl;
    std::cerr << "    <hostname> : IP or host name"                    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    std::cerr << "    <fps>      : Frequency (fps) to send coordinate" << std::endl;
    exit(0);
    }

  char*  hostname = argv[1];
  int    port     = atoi(argv[2]);
  double fps      = atof(argv[3]);
  int    interval = (int) (1000.0 / fps);

  //------------------------------------------------------------
  // Establish Connection

  igtl::UDPClientSocket::Pointer socket;
  socket = igtl::UDPClientSocket::New();
  socket->SetIPAddress("127.0.0.1");
  socket->SetPortNumber(port);
  socket->CreateUDPClient(port);
  unsigned char bufferPKT[RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH];
  igtl::MessageRTPWrapper::Pointer rtpWrapper = igtl::MessageRTPWrapper::New();
  igtl::TrackingDataMessage::Pointer trackingMultiPKTMSG = igtl::TrackingDataMessage::New();
  trackingMultiPKTMSG->SetHeaderVersion(IGTL_HEADER_VERSION_2);
  //std::vector<ReorderBuffer> reorderBufferVec(10, ReorderBuffer();
  ReorderBuffer reorderBuffer = ReorderBuffer();
  int extendedHeaderLength = sizeof(igtl_extended_header);
  int loop = 0;
  for (loop = 0; loop<100; loop++)
  {
    int totMsgLen = socket->ReadSocket(bufferPKT, RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH);
    if (totMsgLen>12)
    {
      // Set up the RTP header:
      igtl_uint32  rtpHdr, timeIncrement;
      rtpHdr = *((igtl_uint32*)bufferPKT);
      //bool rtpMarkerBit = (rtpHdr&0x00800000) != 0;
      timeIncrement = *(igtl_uint32*)(bufferPKT+4);
      igtl_uint32 SSRC = *(igtl_uint32*)(bufferPKT+8);
      if(igtl_is_little_endian())
      {
        rtpHdr = BYTE_SWAP_INT32(rtpHdr);
        timeIncrement = BYTE_SWAP_INT32(timeIncrement);
        SSRC = BYTE_SWAP_INT32(SSRC);
      }
      int curPackedMSGLocation = RTP_HEADER_LENGTH;
      while(curPackedMSGLocation<totMsgLen)
      {
        igtl::MessageHeader::Pointer header = igtl::MessageHeader::New();
        header->AllocatePack();
        memcpy(header->GetPackPointer(), bufferPKT + curPackedMSGLocation, IGTL_HEADER_SIZE);
        igtl_uint16 fragmentNumber = *(bufferPKT + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength-2);
        if(igtl_is_little_endian())
        {
          fragmentNumber = BYTE_SWAP_INT16(fragmentNumber);
        }
        header->Unpack();
        if(fragmentNumber==0X0000) // fragment doesn't exist
        {
          
          if (strcmp(header->GetDeviceType(),"TDATA")==0)
          {
            igtl::TrackingDataMessage::Pointer trackingMSG = igtl::TrackingDataMessage::New();
            trackingMSG->SetMessageHeader(header);
            trackingMSG->AllocatePack();
            memcpy(trackingMSG->GetPackBodyPointer(), bufferPKT + curPackedMSGLocation+IGTL_HEADER_SIZE, header->GetBodySizeToRead());
            ReceiveTrackingData(trackingMSG);
          }
          curPackedMSGLocation += header->GetBodySizeToRead()+IGTL_HEADER_SIZE;
        }
        else
        {
          if (strcmp(header->GetDeviceType(),"TDATA")==0)
          {
            int bodyMsgLength = (RTP_PAYLOAD_LENGTH-IGTL_HEADER_SIZE-extendedHeaderLength);//this is the length of the body within a full fragment paket
            int totFragNumber = -1;
            if(fragmentNumber==0X8000)// To do, fix the issue when later fragment arrives earlier than the beginning fragment
            {
              trackingMultiPKTMSG->SetMessageHeader(header);
              trackingMultiPKTMSG->AllocatePack();
              *(bufferPKT + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength-2) = 0X0000; // set the fragment no. to 0000
              memcpy(reorderBuffer.buffer, bufferPKT + curPackedMSGLocation, totMsgLen-curPackedMSGLocation);
              reorderBuffer.firstPaketPos = totMsgLen-curPackedMSGLocation;
              curPackedMSGLocation = totMsgLen;
            }
            else if(fragmentNumber>=0XE000)// this is the last fragment
            {
              totFragNumber = fragmentNumber - 0XE000 + 1;
              memcpy(reorderBuffer.buffer+reorderBuffer.firstPaketPos+(totFragNumber-1)*bodyMsgLength, bufferPKT + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength, totMsgLen-(RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength));
              reorderBuffer.receivedLastFrag = true;
            }
            else
            {
              int curFragNumber = fragmentNumber - 0X8000;
              memcpy(reorderBuffer.buffer+reorderBuffer.firstPaketPos+(curFragNumber-1)*bodyMsgLength, bufferPKT + RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength, totMsgLen-(RTP_HEADER_LENGTH+IGTL_HEADER_SIZE+extendedHeaderLength));
            }
            reorderBuffer.filledPaketNum++;
            if(reorderBuffer.receivedLastFrag == true && reorderBuffer.filledPaketNum == (totFragNumber+1))
            {
              memcpy(trackingMultiPKTMSG->GetPackBodyPointer(), reorderBuffer.buffer+IGTL_HEADER_SIZE, header->GetBodySizeToRead());
              ReceiveTrackingData(trackingMultiPKTMSG);
              reorderBuffer.filledPaketNum = 0;
              reorderBuffer.receivedLastFrag = false;
            }
          }
          break;
        }
      }
    }
    //igtl::Sleep(interval);
  }
}


int ReceiveTrackingData(igtl::TrackingDataMessage::Pointer& msgData)
{
  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = msgData->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    int nElements = msgData->GetNumberOfTrackingDataElements();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::TrackingDataElement::Pointer trackingElement;
      msgData->GetTrackingDataElement(i, trackingElement);

      igtl::Matrix4x4 matrix;
      trackingElement->GetMatrix(matrix);


      std::cerr << "========== Element #" << i << " ==========" << std::endl;
      std::cerr << " Name       : " << trackingElement->GetName() << std::endl;
      std::cerr << " Type       : " << (int) trackingElement->GetType() << std::endl;
      std::cerr << " Matrix : " << std::endl;
      igtl::PrintMatrix(matrix);
      std::cerr << "================================" << std::endl;
      }
    return 1;
    }
  return 0;
}


