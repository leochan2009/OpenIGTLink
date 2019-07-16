/*=========================================================================

  Program:   OpenIGTLink -- Example for Tracker Client Program
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <iostream>
#include <math.h>
#include <cstdlib>
#include <cstring>

#include "igtlOSUtil.h"
#include "igtlStringMessage.h"
#include "igtlUDPClientSocket.h"
#include "igtlMessageRTPWrapper.h"
#include "igtlClientSocket.h"

int ReceiveIpAddress(igtl::StringMessage::Pointer& message);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 2) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <hostname> <port> <fps>"    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    exit(0);
    }

  int    port     = atoi(argv[1]);

  //------------------------------------------------------------
  // Establish Connection

  igtl::UDPClientSocket::Pointer socket;
  socket = igtl::UDPClientSocket::New();
  int r = socket->JoinNetwork("192.168.0.255", port);

  if (r < 0)
    {
    std::cerr << "Cannot connect to the server." << std::endl;
    exit(0);
    }

  //------------------------------------------------------------
  // Ask the server to start pushing tracking data
  unsigned char* bufferPKT = new unsigned char[RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH];
  igtl::MessageRTPWrapper::Pointer rtpWrapper = igtl::MessageRTPWrapper::New();
  igtl::SimpleMutexLock* glock = igtl::SimpleMutexLock::New();
  int loop = 0;
  const char data[100] = {'s'};
  char * tcpSendData= new char[100];
  tcpSendData[0] = data[0];
  for (loop = 0; loop<10000; loop++)
  {
    int totMsgLen = socket->ReadSocket(bufferPKT, RTP_PAYLOAD_LENGTH+RTP_HEADER_LENGTH);
    std::string output((char*)bufferPKT, totMsgLen);
    if(totMsgLen>0)
    {
        std::cerr<<output.c_str()<<std::endl;
        igtl::ClientSocket::Pointer socket = igtl::ClientSocket::New();
        int r = socket->ConnectToServer("192.168.0.101", 18944);
        if(r>=0)
        {
            for (;;) {
                socket->Send(tcpSendData,100);
                igtl::Sleep(500);
            }
        }

    }
    continue;
    rtpWrapper->PushDataIntoPacketBuffer(bufferPKT, totMsgLen);
    rtpWrapper->UnWrapPacketWithTypeAndName("STRING", "IpAddress");
    glock->Lock();
    unsigned int messageNum = rtpWrapper->unWrappedMessages.size();
    glock->Unlock();
    if(messageNum)// to do: glock this session
      {
      igtl::StringMessage::Pointer ipAddressMsg = igtl::StringMessage::New();
      glock->Lock();
      std::map<igtl_uint32, igtl::UnWrappedMessage*>::iterator it = rtpWrapper->unWrappedMessages.begin();
      igtlUint8 * message = new igtlUint8[it->second->messageDataLength];
      int MSGLength = it->second->messageDataLength;
      memcpy(message, it->second->messagePackPointer, it->second->messageDataLength);
      delete it->second;
      it->second = NULL;
      rtpWrapper->unWrappedMessages.erase(it);
      glock->Unlock();
      igtl::MessageHeader::Pointer header = igtl::MessageHeader::New();
      header->InitPack();
      memcpy(header->GetPackPointer(), message, IGTL_HEADER_SIZE);
      header->Unpack();
      ipAddressMsg->SetMessageHeader(header);
      ipAddressMsg->AllocateBuffer();
      if (MSGLength == ipAddressMsg->GetPackSize())
        {
        memcpy(ipAddressMsg->GetPackPointer(), message, MSGLength);
        ReceiveIpAddress(ipAddressMsg);
        }
      }
    }
}


int ReceiveIpAddress(igtl::StringMessage::Pointer& message)
{


  //------------------------------------------------------------
  // Allocate TrackingData Message Class

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = message->Unpack(0);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
      std::cerr << "========== iP ADDRESS ==========" << std::endl;
      std::cerr << message->GetString()<<std::endl;
      std::cerr << "================================" << std::endl;
    return 1;
    }
  return 0;
}


