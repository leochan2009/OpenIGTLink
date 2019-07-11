/*=========================================================================

  Program:   OpenIGTLink -- Example for Tracking Data Server
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
#include "igtlMessageHeader.h"
#include "igtlUDPServerSocket.h"
#include "igtlStringMessage.h"
#include "igtlMultiThreader.h"
#include "igtlMessageRTPWrapper.h"


int   SendIpAddressData(igtl::UDPServerSocket::Pointer& socket, igtl::StringMessage::Pointer& ipAddressMSG, igtl::MessageRTPWrapper::Pointer rtpWrapper);

int main(int argc, char* argv[])
{

  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 2) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <port>"    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    exit(0);
    }

  int    port     = atoi(argv[1]);
  igtl::UDPServerSocket::Pointer serverSocket;
  serverSocket = igtl::UDPServerSocket::New();
  int r = serverSocket->CreateUDPServer();
  serverSocket->AddClient("10.32.23.255", port, 1);

  if (r < 0)
    {
    std::cerr << "Cannot create a server socket." << std::endl;
    exit(0);
    }

  igtl::StringMessage::Pointer ipAddressMSG;
  ipAddressMSG = igtl::StringMessage::New();

  igtl::SimpleMutexLock* glock = igtl::SimpleMutexLock::New();
  igtl::MessageRTPWrapper::Pointer rtpWrapper = igtl::MessageRTPWrapper::New();
  //------------------------------------------------------------
  // Loop
  for (int i = 0;i<10000;i++)
    {
    ipAddressMSG->SetDeviceName("IpAddress");
    glock->Lock();
    SendIpAddressData(serverSocket, ipAddressMSG, rtpWrapper);
    glock->Unlock();
    igtl::Sleep(500);
    }

    
  //------------------------------------------------------------
  // Close connection (The example code never reaches to this section ...)
  serverSocket->CloseSocket();

}



int SendIpAddressData(igtl::UDPServerSocket::Pointer& socket, igtl::StringMessage::Pointer& ipAddressMSG, igtl::MessageRTPWrapper::Pointer rtpWrapper)
{
  static int times = 0;
  times++;
  ipAddressMSG->SetString("10.32.23.44/" + std::to_string(times));
  ipAddressMSG->Pack();
  rtpWrapper->SetSSRC(1);
  int status = igtl::MessageRTPWrapper::PacketReady;
  igtl_uint8* messagePointer = (igtl_uint8*)ipAddressMSG->GetPackPointer();
  rtpWrapper->SetMSGHeader((igtl_uint8*)ipAddressMSG->GetPackPointer());
  int messageLength = ipAddressMSG->GetPackSize();
  int sendBytes = rtpWrapper->WrapMessageAndSend(socket, messagePointer, messageLength);

   if (sendBytes > messageLength)
  {
      std::cerr << "========== iP ADDRESS sent==========" << std::endl;
      std::cerr <<ipAddressMSG->GetString()<<" SendBytes: "<<sendBytes <<std::endl;
      std::cerr << "================================" << std::endl;
  }
  else {
       std::cerr << "========== error in sending iP ADDRESS sent==========" << std::endl;
  }
  return 0;
}





