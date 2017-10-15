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

#define typeLength 12
#define nameLength 20

struct ReadSocketAndPush
{
  igtl::MessageRTPWrapper::Pointer wrapper;
  igtl::UDPClientSocket::Pointer clientSocket;
  char deviceType[typeLength];
  char deviceName[nameLength];
};


void* ThreadFunctionReadSocketAndUnwrap(void* ptr)
{
  // Get thread information
  igtl::MultiThreader::ThreadInfo* info =
  static_cast<igtl::MultiThreader::ThreadInfo*>(ptr);
  
  ReadSocketAndPush parentObj = *(static_cast<ReadSocketAndPush*>(info->UserData));
  std::string deviceType(parentObj.deviceType);
  std::string deviceName(parentObj.deviceName);
  unsigned char UDPPacket[RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH];
  while(1)
    {
    int totMsgLen = parentObj.clientSocket->ReadSocket(UDPPacket, RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH);
    if (totMsgLen>0)
      {
      parentObj.wrapper->ReceiveDataIntoPacketBuffer(UDPPacket, totMsgLen);
      parentObj.wrapper->UnWrapPacketWithTypeAndName(deviceType.c_str(), deviceName.c_str());
      }
    }
}


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
    exit(0);
    }

  char*  hostname = argv[1];
  int    port     = atoi(argv[2]);
  //------------------------------------------------------------
  // Establish Connection

  igtl::UDPClientSocket::Pointer UDPSocket;
  UDPSocket = igtl::UDPClientSocket::New();
  UDPSocket->JoinNetwork(hostname, port, 0);
  igtl::MessageRTPWrapper::Pointer rtpWrapper = igtl::MessageRTPWrapper::New();
  ReadSocketAndPush info;
  info.wrapper = rtpWrapper;
  info.clientSocket = UDPSocket;
  strcpy(info.deviceName, "Tracker");
  strcpy(info.deviceType, "TDATA");
  igtl::MultiThreader::Pointer threader = igtl::MultiThreader::New();
  threader->SpawnThread((igtl::ThreadFunctionType)&ThreadFunctionReadSocketAndUnwrap, &info);
  while(1)
    {
    igtl::TrackingDataMessage::Pointer trackingDataMultiPKTMSG = igtl::TrackingDataMessage::New();
    if (rtpWrapper->PullUnwrappedMessageAtIndex((igtl::MessageBase::Pointer)trackingDataMultiPKTMSG,0))
      {
      ReceiveTrackingData(trackingDataMultiPKTMSG);
      }
    igtl::Sleep(5);
    }
}


int ReceiveTrackingData(igtl::TrackingDataMessage::Pointer& msgData)
{
  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = msgData->Unpack(0);

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


