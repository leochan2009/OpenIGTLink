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
#include <cstdio>
#include <string.h>

#include "igtlOSUtil.h"
#include "igtlImageMessage.h"
#include "igtlServerSocket.h"



int GetTestImage(igtl::ImageMessage::Pointer& imgMsg, const char* file);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 4) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <port> <fps> <imgfile>"    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    std::cerr << "    <fps>      : Frequency (fps) to send coordinate" << std::endl;
    std::cerr << "    <imgfile>   : file, where \"MRHead-subvolume.nrrd\" are placed." << std::endl;
    std::cerr << "                 (usually, in the Examples/Imager/img directory.)" << std::endl;
    exit(0);
    }

  int    port     = atoi(argv[1]);
  double fps      = atof(argv[2]);
  int    interval = (int) (1000.0 / fps);
  char*  file  = argv[3];


  //------------------------------------------------------------
  // Prepare server socket
  igtl::ServerSocket::Pointer serverSocket;
  serverSocket = igtl::ServerSocket::New();
  int r = serverSocket->CreateServer(port);

  if (r < 0)
    {
    std::cerr << "Cannot create a server socket." << std::endl;
    exit(0);
    }


  igtl::Socket::Pointer socket;
  
  while (1)
    {
    //------------------------------------------------------------
    // Waiting for Connection
    socket = serverSocket->WaitForConnection(1000);
    
    if (socket.IsNotNull()) // if client connected
      {
      //------------------------------------------------------------
      // loop
      for (int i = 0; i < 5; i ++)
        {
        
        //------------------------------------------------------------
        // Create a new IMAGE type message
        igtl::ImageMessage::Pointer imgMsg = igtl::ImageMessage::New();
        //------------------------------------------------------------
        // Set image data (See GetTestImage() bellow for the details)
        if (GetTestImage(imgMsg, file))
        {
        
          //------------------------------------------------------------
          // Pack (serialize) and send
          imgMsg->Pack();
          socket->Send(imgMsg->GetPackPointer(), imgMsg->GetPackSize());
        }
        igtl::Sleep(interval); // wait
        }
      }
    }
  
  //------------------------------------------------------------
  // Close connection (The example code never reachs to this section ...)
  
  socket->CloseSocket();

}

int GetNrrdHeaderLength(const char* file) // the data could contain '\n' so the length is not correct
{
  FILE *fp = fopen(file, "rb");
  std::string data("  ");
  int headerLength = 0;
  int temp = 1;
  while(fread(&data[0],2,1,fp)){
    fseek(fp, -1, SEEK_CUR);
    temp ++;
    if (strcmp(data.c_str(),"\n\n")==0)
    {
      headerLength = temp;
      break;
    }
  }
  fclose(fp);
  fp = NULL;
  return headerLength;
}

int GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength)
{
  int beginIndex = -1;
  int endIndex = -1;
  int index = 0;
  for(index = 0; index < headerLenght; index ++ )
  {
    if (index < headerLenght -tagLength)
    {
      std::string stringTemp(&(headerString[index]), &(headerString[index + tagLength]));
      if(strcmp(stringTemp.c_str(),tag)==0)
      {
        beginIndex = index+tagLength+2;
      }
    }
    std::string stringTemp2(&(headerString[index]), &(headerString[index + 1]));
    if(beginIndex>=0 && (strcmp(stringTemp2.c_str(), "\n") == 0))
    {
      endIndex = index;
      break;
    }
  }
  tagValueString = std::string(&(headerString[beginIndex]), &(headerString[endIndex]));
  return 1;
}

std::string GetValueByDelimiter(std::string &inputString, std::string delimiter, int index)
{
  size_t pos = 0;
  std::string token;
  int tokenIndex = 0;
  while ((pos = inputString.find(delimiter)) != std::string::npos) {
    token = inputString.substr(0, pos);
    //std::cout << token << std::endl;
    inputString.erase(0, pos + delimiter.length());
    tokenIndex++;
    if (tokenIndex == index)
    {
      return token;
    }
  }
  return NULL;
}

//------------------------------------------------------------
// Function to read test image data
int GetTestImage(igtl::ImageMessage::Pointer& imgMsg, const char* file)
{
  //------------------------------------------------------------
  // size parameters
  int   size[]     = {0, 0, 1};       // image dimension
  float spacing[]  = {1.0, 1.0, 1.0};     // spacing (mm/pixel)
  int   scalarType = igtl::ImageMessage::TYPE_UINT8;// scalar type
  //------------------------------------------------------------
  // Get randome orientation matrix and set it.
  igtl::Matrix4x4 matrix;
  
  
  int headerLength = GetNrrdHeaderLength(file);
  FILE *fp = fopen(file, "rb");
  if (fp == NULL)
  {
    std::cerr << "File opeining error: " << file << std::endl;
    return 0;
  }
  char * headerString = new char[headerLength];
  fread(headerString, headerLength,1,fp);
  std::string tagValueString("");
  int tagValueLength;
  if(GetTagValue(headerString, headerLength, "type", 4, tagValueString, tagValueLength))
  {
    if (strcmp(tagValueString.c_str(), "short")==0)
    {
      scalarType = igtl::ImageMessage::TYPE_UINT16;
    }
  }
  if(GetTagValue(headerString, headerLength, "space", 5, tagValueString, tagValueLength))
  {
    if(strcmp(tagValueString.c_str(), "left-posterior-superior")==0 || strcmp(tagValueString.c_str(), "LPS")==0)
    {
      imgMsg->SetCoordinateSystem(igtl::ImageMessage::COORDINATE_LPS);
    }
    if(strcmp(tagValueString.c_str(), "right-anterior-superior")==0 || strcmp(tagValueString.c_str(), "RAS")==0)
    {
      imgMsg->SetCoordinateSystem(igtl::ImageMessage::COORDINATE_RAS);
    }
  }
  
  if(GetTagValue(headerString, headerLength, "sizes", 5, tagValueString, tagValueLength))
  {
    std::string stringTemp(tagValueString);
    std::string valueStr = GetValueByDelimiter(stringTemp," ", 1); // there is a space at the beginning
    size[0] = atoi(valueStr.c_str());
    valueStr = GetValueByDelimiter(stringTemp," ", 1);
    size[1] = atoi(valueStr.c_str());
    size[2] = atoi(stringTemp.c_str());
  }
  
  if(GetTagValue(headerString, headerLength, "space directions", 16, tagValueString, tagValueLength))
  {
    std::string stringTemp(tagValueString);
    std::string firstSpaceDirect = GetValueByDelimiter(stringTemp," ", 1);
    std::string temp = GetValueByDelimiter(firstSpaceDirect,"(", 1);// get rid of the "("
    float x00 = atof(GetValueByDelimiter(firstSpaceDirect,",",1).c_str());
    float x10 = atof(GetValueByDelimiter(firstSpaceDirect,",",1).c_str());
    float x20 = atof(GetValueByDelimiter(firstSpaceDirect,")",1).c_str());
    std::string secondSpaceDirect = GetValueByDelimiter(stringTemp," ", 1);
    temp = GetValueByDelimiter(secondSpaceDirect,"(", 1);// get rid of the "("
    float x01 = atof(GetValueByDelimiter(secondSpaceDirect,",",1).c_str());
    float x11 = atof(GetValueByDelimiter(secondSpaceDirect,",",1).c_str());
    float x21 = atof(GetValueByDelimiter(secondSpaceDirect,")",1).c_str());
    std::string thirdSpaceDirect = GetValueByDelimiter(stringTemp,")", 1);
    temp = GetValueByDelimiter(thirdSpaceDirect,"(", 1);// get rid of the "("
    float x02 = atof(GetValueByDelimiter(thirdSpaceDirect,",",1).c_str());
    float x12 = atof(GetValueByDelimiter(thirdSpaceDirect,",",1).c_str());
    float x22 = atof(thirdSpaceDirect.c_str());
    spacing[0] = sqrt(x00*x00+x10*x10+x20*x20);
    spacing[1] = sqrt(x01*x01+x11*x11+x21*x21);
    spacing[2] = sqrt(x02*x02+x12*x12+x22*x22);
    matrix[0][0] = x00/spacing[0];
    matrix[0][1] = x01/spacing[0];
    matrix[0][2] = x02/spacing[0];
    matrix[1][0] = x10/spacing[1];
    matrix[1][1] = x11/spacing[1];
    matrix[1][2] = x12/spacing[1];
    matrix[2][0] = x20/spacing[2];
    matrix[2][1] = x21/spacing[2];
    matrix[2][2] = x22/spacing[2];
    matrix[3][3] = 1.0;
  }
  
  if(GetTagValue(headerString, headerLength, "endian", 6, tagValueString, tagValueLength))
  {
    if(strcmp(tagValueString.c_str(), "little")==0)
    {
      imgMsg->SetEndian(igtl::ImageMessage::ENDIAN_LITTLE);
    }
    if(strcmp(tagValueString.c_str(), "big")==0)
    {
      imgMsg->SetEndian(igtl::ImageMessage::ENDIAN_BIG);
    }
  }
  if(GetTagValue(headerString, headerLength, "space origin", 12, tagValueString, tagValueLength))
  {
    std::string stringTemp(tagValueString);
    std::string temp = GetValueByDelimiter(stringTemp,"(", 1);// get rid of the "("
    std::string t0 = GetValueByDelimiter(stringTemp,",",1);
    std::string t1 = GetValueByDelimiter(stringTemp,",",1);
    std::string t2 = GetValueByDelimiter(stringTemp,")",1);
    matrix[0][3] = atof(t0.c_str());
    matrix[1][3] = atof(t1.c_str());
    matrix[2][3] = atof(t2.c_str());
  }
  
  imgMsg->SetDimensions(size);
  imgMsg->SetSpacing(spacing);
  imgMsg->SetScalarType(scalarType);
  imgMsg->SetDeviceName("ImagerClient");
  imgMsg->AllocateScalars();
  imgMsg->SetMatrix(matrix);
  
  int fsize = imgMsg->GetImageSize();
  size_t b = fread(imgMsg->GetScalarPointer(), 1, fsize, fp);
  if (b == fsize)
  {
    return 1;
  }
  fclose(fp);

  return 0;
}
