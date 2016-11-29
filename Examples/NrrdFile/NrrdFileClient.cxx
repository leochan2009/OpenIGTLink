/*=========================================================================
 
 Program:   Open IGT Link -- Example for Data Receiving Client Program
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
#include <iomanip>
#include <math.h>
#include <cstdlib>
#include <cstring>
#include <string>

#include "igtlOSUtil.h"
#include "igtlImageMessage.h"
#include "igtlClientSocket.h"
#include "igtlutil/igtl_image.h"

int ReceiveImage(igtl::Socket * socket, igtl::MessageHeader::Pointer& header, char*  file);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments
  
  if (argc != 4) // check number of arguments
  {
    // If not correct, print usage
    std::cerr << "    <hostname> : IP or host name"                    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    std::cerr << "    <dir>     :  directory to save the images"   << std::endl;
    exit(0);
  }
  
  char*  hostname = argv[1];
  int    port     = atoi(argv[2]);
  char*  dir  = argv[3];
  
  //------------------------------------------------------------
  // Establish Connection
  
  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();
  int r = socket->ConnectToServer(hostname, port);
  
  if (r != 0)
  {
    std::cerr << "Cannot connect to the server." << std::endl;
    exit(0);
  }
  
  //------------------------------------------------------------
  // Create a message buffer to receive header
  igtl::MessageHeader::Pointer headerMsg;
  headerMsg = igtl::MessageHeader::New();
  
  //------------------------------------------------------------
  // Allocate a time stamp
  igtl::TimeStamp::Pointer ts;
  ts = igtl::TimeStamp::New();
  //------------------------------------------------------------
  // loop
  for (int i = 0; i < 5; i ++)
  {
    
    // Initialize receive buffer
    headerMsg->InitPack();
    
    // Receive generic header from the socket
    int r = socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
    if (r == 0)
    {
      socket->CloseSocket();
      exit(0);
    }
    if (r != headerMsg->GetPackSize())
    {
      continue;
    }
    
    // Deserialize the header
    headerMsg->Unpack();
    
    // Get time stamp
    igtlUint32 sec;
    igtlUint32 nanosec;
    
    headerMsg->GetTimeStamp(ts);
    ts->GetTimeStamp(&sec, &nanosec);
    
    std::cerr << "Time stamp: "
    << sec << "." << std::setw(9) << std::setfill('0')
    << nanosec << std::endl;
    
    if (strcmp(headerMsg->GetDeviceType(), "IMAGE") == 0)
    {
      char filename[128];
      sprintf(filename, "%s/igtlTestImage%d.nrrd", dir, i+1);
      ReceiveImage(socket, headerMsg, filename);
    }
    else
    {
      std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
      socket->Skip(headerMsg->GetBodySizeToRead(), 0);
    }
  }
  
  //------------------------------------------------------------
  // Close connection (The example code never reaches this section ...)
  
  socket->CloseSocket();
  
}

int ReceiveImage(igtl::Socket * socket, igtl::MessageHeader::Pointer& header, char*  file)
{
  std::cerr << "Receiving IMAGE data type." << std::endl;
  
  // Create a message buffer to receive transform data
  igtl::ImageMessage::Pointer imgMsg;
  imgMsg = igtl::ImageMessage::New();
  imgMsg->SetMessageHeader(header);
  imgMsg->AllocatePack();
  
  // Receive transform data from the socket
  socket->Receive(imgMsg->GetPackBodyPointer(), imgMsg->GetPackBodySize());
  
  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = imgMsg->Unpack(1);
  
  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
  {
    // Retrive the image data
    int   size[3];          // image dimension
    float spacing[3];       // spacing (mm/pixel)
    int   scalarType;       // scalar type
    igtl::Matrix4x4 matrix; // matrix
    int endianType;
    
    
    scalarType = imgMsg->GetScalarType();
    imgMsg->GetDimensions(size);
    imgMsg->GetSpacing(spacing);
    imgMsg->GetMatrix(matrix);
    endianType = imgMsg->GetEndian();
    
    std::cerr << "Device Name           : " << imgMsg->GetDeviceName() << std::endl;
    std::cerr << "Scalar Type           : " << scalarType << std::endl;
    std::cerr << "Dimensions            : ("
    << size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
    std::cerr << "Spacing               : ("
    << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << ")" << std::endl;
    FILE *fp = fopen(file, "wb");
    std::string lineBreak;
    lineBreak = std::string("\n");
    
    std::string buffer("NRRD0004");
    buffer.append(lineBreak);
    buffer.append("# Complete NRRD file format specification at:");
    buffer.append(lineBreak);
    buffer.append("# http://teem.sourceforge.net/nrrd/format.html");
    buffer.append(lineBreak);
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    buffer = std::string("");
    if (imgMsg->GetScalarType() == igtl::ImageMessage::TYPE_UINT16)
    {
      buffer = std::string("type: short");
      buffer.append(lineBreak);
    }
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    buffer = std::string("dimension: 3");
    buffer.append(lineBreak);
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    buffer = std::string("");
    if (imgMsg->GetCoordinateSystem() == igtl::ImageMessage::COORDINATE_RAS)
    {
      buffer = std::string("space: right-anterior-superior");
      buffer.append(lineBreak);
    }
    else if(imgMsg->GetCoordinateSystem() == igtl::ImageMessage::COORDINATE_LPS)
    {
      buffer = std::string("space: left-posterior-superior");
      buffer.append(lineBreak);
    }
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    
    buffer = std::string("sizes: ");
    buffer.append(std::to_string(size[0]));
    buffer.append(" ");
    buffer.append(std::to_string(size[1]));
    buffer.append(" ");
    buffer.append(std::to_string(size[2]));
    buffer.append(lineBreak);
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    buffer = std::string("space directions: (");
    buffer.append(std::to_string(matrix[0][0]*spacing[0]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[0][1]*spacing[0]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[0][2]*spacing[0]));
    buffer.append(") (");
    buffer.append(std::to_string(matrix[1][0]*spacing[1]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[1][1]*spacing[1]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[1][2]*spacing[1]));
    buffer.append(") (");
    buffer.append(std::to_string(matrix[2][0]*spacing[2]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[2][1]*spacing[2]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[2][2]*spacing[2]));
    buffer.append(")");
    buffer.append(lineBreak);
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    
    buffer = std::string("kinds: domain domain domain");
    buffer.append(lineBreak);
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    
    buffer = std::string("");
    if (imgMsg->GetEndian() == igtl::ImageMessage::ENDIAN_BIG)
    {
      buffer = std::string("endian: big");
      buffer.append(lineBreak);
    }
    else if(imgMsg->GetCoordinateSystem() == igtl::ImageMessage::ENDIAN_LITTLE)
    {
      buffer = std::string("endian: little");
      buffer.append(lineBreak);
    }
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    
    buffer = std::string("encoding: raw");
    buffer.append(lineBreak);
    fwrite (buffer.c_str(), 1, buffer.length(), fp);
    
    
    buffer = std::string("space origin: (");
    buffer.append(std::to_string(matrix[0][3]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[1][3]));
    buffer.append(",");
    buffer.append(std::to_string(matrix[2][3]));
    buffer.append(")");
    buffer.append(lineBreak);
    fwrite(buffer.c_str(), 1, buffer.length(), fp);
    fwrite("\r", 1, 1, fp);
    int fsize = imgMsg->GetImageSize();
    int b = fwrite((unsigned char*)imgMsg->GetScalarPointer(), 1, fsize,fp);
    fclose(fp);
    fp = NULL;
    
    if (b == fsize)
    {
      return 1;
    }
  }
  return 0;
  
}