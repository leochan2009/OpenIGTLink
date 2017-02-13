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
#include <cstdio>

#include "igtl_header.h"
#include "igtlOSUtil.h"
#include "igtlVideoMessage.h"
#include "igtlServerSocket.h"

#include <lz4frame.h>

#define BUF_SIZE 16*1024
#define LZ4_HEADER_SIZE 19
#define LZ4_FOOTER_SIZE 4

static const LZ4F_preferences_t lz4_preferences = {
  { LZ4F_max256KB, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0, { 0, 0 } },
  0,   /* compression level */
  0,   /* autoflush */
  { 0, 0, 0, 0 },  /* reserved, must be set to 0 */
};

static unsigned char encodedStream[BUF_SIZE*10000];

int GetTestImage(igtl::VideoMessage::Pointer& msg, const char* dir, int i);

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
    std::cerr << "    <imgdir>   : file directory, where \"igtlTestImage[1-5].raw\" are placed." << std::endl;
    std::cerr << "                 (usually, in the Examples/Imager/img directory.)" << std::endl;
    exit(0);
    }

  int    port     = atoi(argv[1]);
  double fps      = atof(argv[2]);
  int    interval = (int) (1000.0 / fps);
  char*  filedir  = argv[3];


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
        std::cerr << "A client is connected." << std::endl;
        // Create a message buffer to receive header
        igtl::MessageHeader::Pointer headerMsg;
        headerMsg = igtl::MessageHeader::New();
        headerMsg->InitPack();
        
        // Receive generic header from the socket
        int rs = socket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
        if (rs == headerMsg->GetPackSize())
        {
          headerMsg->Unpack();
          if (strcmp(headerMsg->GetDeviceType(), "STT_VIDEO") == 0)
          {
            //------------------------------------------------------------
            // loop
            for (int i = 0; i < 100; i ++)
            {
              igtl::VideoMessage::Pointer videoMsg;
              videoMsg = igtl::VideoMessage::New();
              videoMsg->SetHeaderVersion(IGTL_HEADER_VERSION_2);
              videoMsg->SetDeviceName("LZ4Server");
              static igtl_uint32 messageID = -1;
              messageID ++;
              videoMsg->SetMessageID(messageID);
              videoMsg->AllocateScalars();
              
              // Following line may be called in case of 16-, 32-, and 64-bit scalar types.
              // imgMsg->SetEndian(igtl::ImageMessage::ENDIAN_BIG);

              //------------------------------------------------------------
              // Set image data (See GetTestImage() bellow for the details)
              if (GetTestImage(videoMsg, filedir, i % 5))
              {
              
              
                //------------------------------------------------------------
                // Pack (serialize) and send
                videoMsg->Pack();
                socket->Send(videoMsg->GetPackPointer(), videoMsg->GetPackSize());
              }
              igtl::Sleep(interval); // wait
            }
          }
        }
      }
    }
  
  //------------------------------------------------------------
  // Close connection (The example code never reachs to this section ...)
  
  socket->CloseSocket();

}


//------------------------------------------------------------
// Function to read test image data
int GetTestImage(igtl::VideoMessage::Pointer& msg, const char* dir, int i)
{

  //------------------------------------------------------------
  // Check if image index is in the range
  if (i < 0 || i >= 5) 
    {
    std::cerr << "Image index is invalid." << std::endl;
    return 0;
    }

  //------------------------------------------------------------
  // Generate path to the raw image file
  char filename[128];
  sprintf(filename, "%s/igtlTestImage%d.raw", dir, i+1);
  std::cerr << "Reading " << filename << "...";

  //------------------------------------------------------------
  // Load raw data from the file
  //int fsize = msg->GetImageSize();
  //size_t b = fread(msg->GetScalarPointer(), 1, fsize, fp);
  /* compress */
  FILE* const inpFp = fopen(filename, "rb");
  if (inpFp == NULL)
  {
    std::cerr << "File opeining error: " << filename << std::endl;
    return 0;
  }
  
  //////----------------------------------
  // This section is copied and modified from LZ4 example
  LZ4F_errorCode_t r;
  LZ4F_compressionContext_t ctx;
  char *src, *buf = NULL;
  size_t size, n, k, count_out, offset = 0, frame_size;
  int streamOffset = 0;
  r = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
  if (LZ4F_isError(r)) {
    printf("Failed to create context: error %zu\n", r);
    goto cleanup;
  }
  r = 1;
  
  src = (char*)malloc(BUF_SIZE);
  if (!src) {
    printf("Not enough memory\n");
    goto cleanup;
  }
  
  frame_size = LZ4F_compressBound(BUF_SIZE, &lz4_preferences);
  size =  frame_size + LZ4_HEADER_SIZE + LZ4_FOOTER_SIZE;
  buf = (char*)malloc(size);
  if (!buf) {
    printf("Not enough memory\n");
    goto cleanup;
  }
  
  n = offset = count_out = LZ4F_compressBegin(ctx, buf, size, &lz4_preferences);
  if (LZ4F_isError(n)) {
    printf("Failed to start compression: error %zu\n", n);
    goto cleanup;
  }
  
  printf("Buffer size is %zu bytes, header size %zu bytes\n", size, n);
  for (;;) {
    k = fread(src, 1, BUF_SIZE, inpFp);
    if (k == 0)
      break;
    
    n = LZ4F_compressUpdate(ctx, buf + offset, size - offset, src, k, NULL);
    if (LZ4F_isError(n)) {
      printf("Compression failed: error %zu\n", n);
      goto cleanup;
    }
    
    offset += n;
    count_out += n;
    if (size - offset < frame_size + LZ4_FOOTER_SIZE) {
      printf("copying %zu bytes\n", offset);
      memcpy(encodedStream+streamOffset, buf, offset);
      offset = 0;
    }
    streamOffset += n;
  }
  
  n = LZ4F_compressEnd(ctx, buf + offset, size - offset, NULL);
  offset += n;
  count_out += n;
  memcpy(encodedStream+streamOffset, buf, offset);
  
  if (LZ4F_isError(n)) {
    printf("Failed to end compression: error %zu\n", n);
    goto cleanup;
  }
  printf("Writing %zu bytes\n", offset);
  
  ////------------------------------------
  
  msg->SetBitStreamSize(count_out);
  msg->AllocateBuffer();
  msg->SetScalarType(msg->TYPE_UINT8);
  msg->SetEndian(igtl_is_little_endian()==true?2:1); //little endian is 2 big endian is 1
  memcpy(msg->GetPackFragmentPointer(2), encodedStream, count_out);
  msg->Pack();
  std::cerr << "done." << std::endl;

  return 1;
cleanup:
  if (ctx)
    LZ4F_freeCompressionContext(ctx);
  free(src);
  free(buf);
  fclose(inpFp);
  return 0;
}

