/*=========================================================================

  Program:   Open IGT Link -- Example for Tracking Data Server
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

#include "igtlOSUtil.h"
#include "igtlMessageHeader.h"
#include "igtlImageMessage.h"
#include "igtlUDPServerSocket.h"
#include "igtlTrackingDataMessage.h"
#include "igtlMultiThreader.h"
#include "igtlMessageRTPWrapper.h"


//void WrapMessage(igtl::UDPServerSocket::Pointer &serverSocket, igtl::MutexLock::Pointer &glock, igtl::MessageRTPWrapper::Pointer &rtpWrapper);
int   SendTrackingData(igtl::UDPServerSocket::Pointer &socket, igtl::MessageRTPWrapper::Pointer &rtpWrapper);
void  GetRandomTestMatrix(igtl::Matrix4x4& matrix, float phi, float theta);


int main(int argc, char* argv[])
{

  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 3) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << "<clientIP> <port>"    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    exit(0);
    }
  char*  hostname = argv[1];
  int    port     = atoi(argv[2]);
  igtl::UDPServerSocket::Pointer serverSocket;
  serverSocket = igtl::UDPServerSocket::New();
  int r = serverSocket->CreateUDPServer(port);
  //serverSocket->AddGroup("226.0.0.1", 18944, 1);
  serverSocket->AddClient(hostname, 18944, 0);
  if (r < 0)
    {
    std::cerr << "Cannot create a server socket." << std::endl;
    exit(0);
    }

  igtl::MultiThreader::Pointer threader = igtl::MultiThreader::New();
  igtl::MessageRTPWrapper::Pointer rtpWrapper = igtl::MessageRTPWrapper::New();
  //------------------------------------------------------------
  // loop
  while (1)
  {
    SendTrackingData(serverSocket, rtpWrapper);
  }
  
  //------------------------------------------------------------
  // Close connection (The example code never reaches to this section ...)
  serverSocket->CloseSocket();

}


int SendTrackingData(igtl::UDPServerSocket::Pointer &socket, igtl::MessageRTPWrapper::Pointer &rtpWrapper)
{

  static float phi0   = 0.0;
  static float theta0 = 0.0;
  static float phi1   = 0.0;
  static float theta1 = 0.0;
  static float phi2   = 0.0;
  static float theta2 = 0.0;
  static igtlUint32 messageID = 0;
  
  igtl::TrackingDataMessage::Pointer trackingMsg;
  trackingMsg = igtl::TrackingDataMessage::New();
  trackingMsg->SetHeaderVersion(IGTL_HEADER_VERSION_2);
  trackingMsg->SetDeviceName("Tracker");
  
  igtl::TrackingDataElement::Pointer trackElement0;
  trackElement0 = igtl::TrackingDataElement::New();
  trackElement0->SetName("Channel 0");
  trackElement0->SetType(igtl::TrackingDataElement::TYPE_TRACKER);
  
  igtl::TrackingDataElement::Pointer trackElement1;
  trackElement1 = igtl::TrackingDataElement::New();
  trackElement1->SetName("Channel 1");
  trackElement1->SetType(igtl::TrackingDataElement::TYPE_6D);
  
  igtl::TrackingDataElement::Pointer trackElement2;
  trackElement2 = igtl::TrackingDataElement::New();
  trackElement2->SetName("Channel 2");
  trackElement2->SetType(igtl::TrackingDataElement::TYPE_5D);
  
  trackingMsg->AddTrackingDataElement(trackElement0);
  trackingMsg->AddTrackingDataElement(trackElement1);
  trackingMsg->AddTrackingDataElement(trackElement2);

  igtl::Matrix4x4 matrix;
  igtl::TrackingDataElement::Pointer ptr;

  // Channel 0
  trackingMsg->GetTrackingDataElement(0, ptr);
  GetRandomTestMatrix(matrix, phi0, theta0);
  ptr->SetMatrix(matrix);
  
  // Channel 1
  trackingMsg->GetTrackingDataElement(1, ptr);
  GetRandomTestMatrix(matrix, phi1, theta1);
  ptr->SetMatrix(matrix);
  
  // Channel 2
  trackingMsg->GetTrackingDataElement(2, ptr);
  GetRandomTestMatrix(matrix, phi2, theta2);
  ptr->SetMatrix(matrix);
  
  trackingMsg->SetMessageID(messageID);
  messageID ++;
  trackingMsg->Pack();
  rtpWrapper->SetSSRC(1);
  int status = igtl::MessageRTPWrapper::PaketReady;
  status = rtpWrapper->WrapMessageAndSend(socket, (unsigned char*)trackingMsg->GetPackPointer(), trackingMsg->GetPackSize());
  //------------------------------------------------------------
  
  igtl::Sleep(100);
  std::cerr<<"Send"<<std::endl;
  phi0 += 0.1;
  phi1 += 0.2;
  phi2 += 0.3;
  theta0 += 0.2;
  theta1 += 0.1;
  theta2 += 0.05;
  //trackingMsg->ClearTrackingDataElements();
  return 0;
}



//------------------------------------------------------------
// Function to generate random matrix.
void GetRandomTestMatrix(igtl::Matrix4x4& matrix, float phi, float theta)
{
  float position[3];
  float orientation[4];

  // random position
  position[0] = 50.0 * cos(phi);
  position[1] = 50.0 * sin(phi);
  position[2] = 50.0 * cos(phi);
  phi = phi + 0.2;

  // random orientation
  orientation[0]=0.0;
  orientation[1]=0.6666666666*cos(theta);
  orientation[2]=0.577350269189626;
  orientation[3]=0.6666666666*sin(theta);
  theta = theta + 0.1;

  //igtl::Matrix4x4 matrix;
  igtl::QuaternionToMatrix(orientation, matrix);

  matrix[0][3] = position[0];
  matrix[1][3] = position[1];
  matrix[2][3] = position[2];
  
  //igtl::PrintMatrix(matrix);
}






