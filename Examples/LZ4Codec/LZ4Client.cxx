/*=========================================================================

  Program:   OpenIGTLink -- Example for Imager Client Program
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <iostream>
#include <math.h>
#include <cstdlib>
#include <cstdio>

#include "igtl_header.h"
#include "igtlOSUtil.h"
#include "igtlVideoMessage.h"
#include "igtlClientSocket.h"

#include <lz4frame.h>

#define BUF_SIZE 16*1024
#define LZ4_HEADER_SIZE 19
#define LZ4_FOOTER_SIZE 4

size_t get_block_size(const LZ4F_frameInfo_t* info) {
  switch (info->blockSizeID) {
    case LZ4F_default:
    case LZ4F_max64KB:  return 1 << 16;
    case LZ4F_max256KB: return 1 << 18;
    case LZ4F_max1MB:   return 1 << 20;
    case LZ4F_max4MB:   return 1 << 22;
    default:
      printf("Impossible unless more block sizes are allowed\n");
      exit(1);
  }
}


int ReceiveImageData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header, int loop);
int DecodeImage(igtl::VideoMessage::Pointer& msg, int i);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 3) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <hostname> <port> <fps> <imgdir>"    << std::endl;
    std::cerr << "    <hostname> : IP or host name"                    << std::endl;
    std::cerr << "    <port>     : Port # (18944 in Slicer default)"   << std::endl;
    exit(0);
    }
  
  char*  hostname = argv[1];
  int    port     = atoi(argv[2]);
  //------------------------------------------------------------
  // Establish Connection
  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();
  int r = socket->ConnectToServer(hostname, port);

  //------------------------------------------------------------
  // Ask the server to start pushing tracking data
  std::cerr << "Sending Get_Image message....." << std::endl;
  igtl::StartVideoDataMessage::Pointer startVideoMsg;
  startVideoMsg = igtl::StartVideoDataMessage::New();
  startVideoMsg->SetDeviceName("ImageClient");
  startVideoMsg->SetHeaderVersion(IGTL_HEADER_VERSION_2);
  startVideoMsg->Pack();
  socket->Send(startVideoMsg->GetPackPointer(), startVideoMsg->GetPackSize());
  
  int loop = 0;
  
  while (1)
  {
    //------------------------------------------------------------
    // Wait for a reply
    igtl::MessageHeader::Pointer headerMsg;
    headerMsg = igtl::MessageHeader::New();
    headerMsg->InitPack();
    int rs = socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
    if (rs == 0)
    {
      std::cerr << "Connection closed." << std::endl;
      socket->CloseSocket();
      exit(0);
    }
    if (rs != headerMsg->GetPackSize())
    {
      std::cerr << "Message size information and actual data size don't match." << std::endl;
      socket->CloseSocket();
      exit(0);
    }
    
    headerMsg->Unpack();
    if (headerMsg->GetHeaderVersion() != IGTL_HEADER_VERSION_2)
    {
      std::cerr << "Version of the client and server don't match." << std::endl;
      socket->CloseSocket();
      exit(0);
    }
    if (strcmp(headerMsg->GetDeviceName(), "LZ4Server") == 0)
    {
      ReceiveImageData(socket, headerMsg, loop);
      igtl::Sleep(100);
    }
    else
    {
      std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
      socket->Skip(headerMsg->GetBodySizeToRead(), 0);
    }
    if (++loop >= 100) // if received 100 times
    {
      //------------------------------------------------------------
      // Ask the server to stop pushing tracking data
      std::cerr << "Sending STP_IMAGE message....." << std::endl;
      igtl::StopVideoMessage::Pointer stopVideoMsg;
      stopVideoMsg = igtl::StopVideoMessage::New();
      stopVideoMsg->InitPack();
      stopVideoMsg->SetDeviceName("VideoClient");
      stopVideoMsg->Pack();
      socket->Send(stopVideoMsg->GetPackPointer(), stopVideoMsg->GetPackSize());
      loop = 0;
    }
  }

}


//------------------------------------------------------------
// Function to read test image data
int ReceiveImageData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header,int loop)
{

  std::cerr << "Receiving TDATA data type." << std::endl;
  
  //------------------------------------------------------------
  // Allocate TrackingData Message Class
  
  igtl::VideoMessage::Pointer videoMsg;
  videoMsg = igtl::VideoMessage::New();
  videoMsg->SetMessageHeader(header);//Here the version is also set
  videoMsg->AllocateBuffer();
  
  // Receive body from the socket
  socket->Receive(videoMsg->GetPackBodyPointer(), videoMsg->GetPackBodySize());
  
  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = videoMsg->Unpack(1);
  
  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
  {
#if OpenIGTLink_HEADER_VERSION >= 2
    if (videoMsg->GetHeaderVersion() >= IGTL_HEADER_VERSION_2)
    {
      int i = 0;
      for (std::map<std::string, std::pair<IANA_ENCODING_TYPE, std::string> >::const_iterator it = videoMsg->GetMetaData().begin(); it != videoMsg->GetMetaData().end(); ++it, ++i)
      {
        std::cerr<<"The message ID is:"<< " " << videoMsg->GetMessageID() << std::endl;
        std::cerr<< it->first << " coding scheme: " << it->second.first<< " " << it->second.second << std::endl;
      }
    }
#endif
    DecodeImage(videoMsg, loop);
    return 1;
  }
  return 0;
}


//------------------------------------------------------------
// Function to read test image data
int DecodeImage(igtl::VideoMessage::Pointer& msg, int i)
{
  
  //------------------------------------------------------------
  // Check if image index is in the range
  if (i < 0 || i >= 100)
  {
    std::cerr << "Image index is invalid." << std::endl;
    return 0;
  }
  
  //------------------------------------------------------------
  // Generate path to the raw image file
  char filename[128];
  sprintf(filename, "igtlSaveImage%d.raw", i+1);
  std::cerr << "Saving " << filename << "...";
  
  //------------------------------------------------------------
  // Load raw data from the file
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL)
  {
    std::cerr << "File opeining error: " << filename << std::endl;
    return 0;
  }
  
  //////----------------------------------
  // This section is copied and modified from LZ4 example
  unsigned char* src = new unsigned char[BUF_SIZE];
  unsigned char* dst = NULL;
  size_t dstCapacity = 0;
  LZ4F_dctx *dctx = NULL;
  size_t residual, ret;
  
  /* Initialization */
  if (!src) { perror("decompress_file(src)"); goto cleanup; }
  ret = LZ4F_createDecompressionContext(&dctx, 100);
  if (LZ4F_isError(ret)) {
    printf("LZ4F_dctx creation error: %s\n", LZ4F_getErrorName(ret));
    goto cleanup;
  }
  
  /* Decompression */
  residual = 0;
  while (residual < msg->GetPackBodySize()) {
    /* Load more input */
    memcpy(src, (char*)(msg->GetPackFragmentPointer(2))+residual, BUF_SIZE);
    size_t srcSize = 0;
    if ((residual + BUFSIZ) < msg->GetPackBodySize())
    {
      srcSize = BUFSIZ;
    }
    else
    {
      srcSize = msg->GetPackBodySize() - residual;
    }
    residual += srcSize;
    unsigned char* srcPtr = src;
    unsigned char* srcEnd = srcPtr + srcSize;
    if (srcSize == 0) {
      printf("Decompress: not enough input or error reading file\n");
      goto cleanup;
    }
    /* Allocate destination buffer if it isn't already */
    if (!dst) {
      LZ4F_frameInfo_t info;
      ret = LZ4F_getFrameInfo(dctx, &info, src, &srcSize);
      if (LZ4F_isError(ret)) {
        printf("LZ4F_getFrameInfo error: %s\n", LZ4F_getErrorName(ret));
        goto cleanup;
      }
      /* Allocating enough space for an entire block isn't necessary for
       * correctness, but it allows some memcpy's to be elided.
       */
      dstCapacity = get_block_size(&info);
      dst = new unsigned char[dstCapacity];
      if (!dst) { perror("decompress_file(dst)"); goto cleanup; }
      srcPtr += srcSize;
      srcSize = srcEnd - srcPtr;
    }
    /* Decompress:
     * Continue while there is more input to read and the frame isn't over.
     * If srcPtr == srcEnd then we know that there is no more output left in the
     * internal buffer left to flush.
     */
    while (srcPtr != srcEnd && ret != 0) {
      /* INVARIANT: Any data left in dst has already been written */
      size_t dstSize = dstCapacity;
      ret = LZ4F_decompress(dctx, dst, &dstSize, srcPtr, &srcSize, /* LZ4F_decompressOptions_t */ NULL);
      if (LZ4F_isError(ret)) {
        printf("Decompression error: %s\n", LZ4F_getErrorName(ret));
        goto cleanup;
      }
      /* Flush output */
      if (dstSize != 0){
        size_t written = fwrite(dst, 1, dstSize, fp);
        printf("Writing %zu bytes\n", dstSize);
        if (written != dstSize) {
          printf("Decompress: Failed to write to file\n");
          goto cleanup;
        }
      }
      /* Update input */
      srcPtr += srcSize;
      srcSize = srcEnd - srcPtr;
    }
  }
  
cleanup:
  free(src);
  free(dst);
  return LZ4F_freeDecompressionContext(dctx);   /* note : free works on NULL */
//
////-------------------------
  
  fclose(fp);
  
  std::cerr << "done." << std::endl;
  
  return 1;
}

