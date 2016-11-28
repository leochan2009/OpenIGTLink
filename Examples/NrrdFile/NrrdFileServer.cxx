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
    std::cerr << "Usage: " << argv[0] << " <port> <fps> <imgdir>"    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    std::cerr << "    <fps>      : Frequency (fps) to send coordinate" << std::endl;
    std::cerr << "    <img>   : file, where \"MRHead-subvolume.nrrd\" are placed." << std::endl;
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
      for (int i = 0; i < 100; i ++)
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
  bool foundSemiComma = false;
  while(fread(&data[0],2,1,fp)){
    fseek(fp, -1, SEEK_CUR);
    temp ++;
    if (strcmp(data.c_str(),": ")==0)
    {
      foundSemiComma = true;
    }
#if defined(_WIN32) || defined(__CYGWIN__)
    if (strcmp(data.c_str(),"\r\n")==0 && foundSemiComma)
    {
      headerLength = temp;
      foundSemiComma = false;
    }
    
#else
    const char datatemp = data.c_str()[0];
    if (datatemp == '\n' && foundSemiComma)
    {
      headerLength = temp;
      foundSemiComma = false;
    }
#endif
  }
  fclose(fp);
  fp = NULL;
  return headerLength;
}

int GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, char* & tagValueString,int&tagValueLength)
{
  int beginIndex = -1;
  int endIndex = -1;
  int index = 0;
  for(index = 0; index < headerLenght; index ++ )
  {
    char *stringTemp = new char[tagLength];
    if (index < headerLenght -tagLength)
    {
      memcpy(stringTemp ,&(headerString[index]), tagLength);
      if(strcmp(stringTemp,tag)==0)
      {
        beginIndex = index+tagLength+2;
      }
    }
#if defined(_WIN32) || defined(__CYGWIN__)
    if (index < headerLenght - 1)
    {
      char *endChar = new char[2];
      memcpy(endChar,&(headerString[index]), 2);
      if(beginIndex>=0 && (strcmp(endChar, "\r\n") == 0))
      {
        endIndex = index;
        break;
      }
    }
#else
    char *endChar = new char[1];
    memcpy(endChar,&(headerString[index]), 1);
    if(beginIndex>=0 && (strcmp(endChar, "\n") == 0))
    {
      endIndex = index;
      break;
    }
#endif
  }
  tagValueString = new char[endIndex-beginIndex];
  memcpy(tagValueString, &(headerString[beginIndex]), endIndex-beginIndex);
  return 1;
}

std::string GetValueByDelimiter(std::string &inputString, std::string delimiter, int index)
{
  size_t pos = 0;
  std::string token;
  int tokenIndex = 0;
  while ((pos = inputString.find(delimiter)) != std::string::npos) {
    token = inputString.substr(0, pos);
    std::cout << token << std::endl;
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
  int   size[]     = {256, 256, 1};       // image dimension
  float spacing[]  = {1.0, 1.0, 5.0};     // spacing (mm/pixel)
  int   svsize[]   = {256, 256, 1};       // sub-volume size
  int   svoffset[] = {0, 0, 0};           // sub-volume offset
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
  char * tagValueString = NULL;
  int tagValueLength;
  if(GetTagValue(headerString, headerLength, "type", 4, tagValueString, tagValueLength))
  {
    if (strcmp(tagValueString, "short")==0)
    {
      scalarType = igtl::ImageMessage::TYPE_UINT16;
    }
  }
  if(GetTagValue(headerString, headerLength, "space", 5, tagValueString, tagValueLength))
  {
    if(strcmp(tagValueString, "left-posterior-superior")==0 || strcmp(tagValueString, "LPS")==0)
    {
      imgMsg->SetCoordinateSystem(igtl::ImageMessage::COORDINATE_LPS);
    }
    if(strcmp(tagValueString, "right-anterior-superior")==0 || strcmp(tagValueString, "RAS")==0)
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
    std::string x00 = GetValueByDelimiter(firstSpaceDirect,",",1);
    std::string x10 = GetValueByDelimiter(firstSpaceDirect,",",1);
    std::string x20 = GetValueByDelimiter(firstSpaceDirect,")",1);
    std::string secondSpaceDirect = GetValueByDelimiter(stringTemp," ", 1);
    temp = GetValueByDelimiter(secondSpaceDirect,"(", 1);// get rid of the "("
    std::string x01 = GetValueByDelimiter(secondSpaceDirect,",",1);
    std::string x11 = GetValueByDelimiter(secondSpaceDirect,",",1);
    std::string x21 = GetValueByDelimiter(secondSpaceDirect,")",1);
    std::string thirdSpaceDirect = GetValueByDelimiter(stringTemp,")", 1);
    temp = GetValueByDelimiter(thirdSpaceDirect,"(", 1);// get rid of the "("
    std::string x02 = GetValueByDelimiter(thirdSpaceDirect,",",1);
    std::string x12 = GetValueByDelimiter(thirdSpaceDirect,",",1);
    std::string x22 = thirdSpaceDirect;
    spacing[0] = atof(x00.c_str());
    spacing[1] = atof(x11.c_str());
    spacing[2] = atof(x22.c_str());
    matrix[0][0] = atof(x00.c_str());
    matrix[0][1] = atof(x01.c_str());
    matrix[0][2] = atof(x02.c_str());
    matrix[1][0] = atof(x10.c_str());
    matrix[1][1] = atof(x11.c_str());
    matrix[1][2] = atof(x12.c_str());
    matrix[2][0] = atof(x20.c_str());
    matrix[2][1] = atof(x21.c_str());
    matrix[2][2] = atof(x22.c_str());
  }
  
  if(GetTagValue(headerString, headerLength, "endian", 6, tagValueString, tagValueLength))
  {
    if(strcmp(tagValueString, "little")==0)
    {
      imgMsg->SetCoordinateSystem(igtl::ImageMessage::ENDIAN_LITTLE);
    }
    if(strcmp(tagValueString, "big")==0)
    {
      imgMsg->SetCoordinateSystem(igtl::ImageMessage::ENDIAN_BIG);
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

  std::cerr << "done." << std::endl;

  return 0;
}
