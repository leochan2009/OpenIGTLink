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
#include "igtlMultiThreader.h"
#include "igtlConditionVariable.h"
#include "igtlMutexLock.h"

void* ThreadFunctionUnWrap(void* ptr);

void* ThreadFunctionReadSocket(void* ptr);

int ReceiveTrackingData(igtl::TrackingDataMessage::Pointer& msgData);


struct ReadSocketAndPush
{
  igtl::MessageRTPWrapper::Pointer wrapper;
  igtl::UDPClientSocket::Pointer clientSocket;
};

struct Wrapper
{
  igtl::MessageRTPWrapper::Pointer wrapper;
};


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
  socket->JoinNetwork("226.0.0.1", port, 1);
  igtl::ConditionVariable::Pointer conditionVar = igtl::ConditionVariable::New();
  igtl::SimpleMutexLock* glock = igtl::SimpleMutexLock::New();
  //socket->JoinNetwork("127.0.0.1", port, 0); // join the local network for a client connection
  igtl::MessageRTPWrapper::Pointer rtpWrapper = igtl::MessageRTPWrapper::New();
  //std::vector<ReorderBuffer> reorderBufferVec(10, ReorderBuffer();
  //int loop = 0;
  ReadSocketAndPush info;
  info.wrapper = rtpWrapper;
  info.clientSocket = socket;
  
  Wrapper infoWrapper;
  infoWrapper.wrapper = rtpWrapper;
  
  igtl::MultiThreader::Pointer threader = igtl::MultiThreader::New();
  
  threader->SpawnThread((igtl::ThreadFunctionType)&ThreadFunctionReadSocket, &info);
  threader->SpawnThread((igtl::ThreadFunctionType)&ThreadFunctionUnWrap, &infoWrapper);
  while(1)
  {
    if(rtpWrapper->unWrappedMessages.size())// to do: glock this session
    {
      igtl::TrackingDataMessage::Pointer trackingMultiPKTMSG = igtl::TrackingDataMessage::New();
      trackingMultiPKTMSG->SetHeaderVersion(IGTL_HEADER_VERSION_2);
      std::map<igtl_uint32, igtl::UnWrappedMessage*>::iterator it = rtpWrapper->unWrappedMessages.begin();
      igtl::MessageHeader::Pointer header = igtl::MessageHeader::New();
      header->InitPack();
      memcpy(header->GetPackPointer(), it->second->messagePackPointer, IGTL_HEADER_SIZE);
      header->Unpack();
      trackingMultiPKTMSG->SetMessageHeader(header);
      trackingMultiPKTMSG->AllocateBuffer();
      if (it->second->messageDataLength == trackingMultiPKTMSG->GetPackSize())
      {
        memcpy(trackingMultiPKTMSG->GetPackPointer(), it->second->messagePackPointer, it->second->messageDataLength);
        ReceiveTrackingData(trackingMultiPKTMSG);
      }
      rtpWrapper->unWrappedMessages.erase(it);
      delete it->second;
    }
  }
}

void* ThreadFunctionUnWrap(void* ptr)
{
  // Get thread information
  igtl::MultiThreader::ThreadInfo* info =
  static_cast<igtl::MultiThreader::ThreadInfo*>(ptr);
  const char *trackingDeviceName = "Tracker";
  const char *deviceType = "TDATA";
  Wrapper parentObj = *(static_cast<Wrapper*>(info->UserData));
  while(1)
  {
    parentObj.wrapper->UnWrapPaketWithTypeAndName(deviceType, trackingDeviceName);
    igtl::Sleep(5);
  }
}


void* ThreadFunctionReadSocket(void* ptr)
{
  // Get thread information
  igtl::MultiThreader::ThreadInfo* info =
  static_cast<igtl::MultiThreader::ThreadInfo*>(ptr);
  
  ReadSocketAndPush parentObj = *(static_cast<ReadSocketAndPush*>(info->UserData));
  unsigned char UDPPaket[RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH];
  const char *trackingDeviceName = "Tracker";
  const char *deviceType = "TDATA";
  while(1)
  {
    int totMsgLen = parentObj.clientSocket->ReadSocket(UDPPaket, RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH);
    if (totMsgLen>0)
    {
      for(int i=78;i<84;i++)
        std::cerr<<*(UDPPaket+i)<<std::endl;
      parentObj.wrapper->PushDataIntoPaketBuffer(UDPPaket, totMsgLen);
    }
  }
}

int ReceiveTrackingData(igtl::TrackingDataMessage::Pointer& trackingMSG)
{
  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = trackingMSG->Unpack(0); // to do crc check fails, fix the error

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    int nElements = trackingMSG->GetNumberOfTrackingDataElements();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::TrackingDataElement::Pointer trackingElement;
      trackingMSG->GetTrackingDataElement(i, trackingElement);

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


