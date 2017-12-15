/*=========================================================================

  Program:   Video Streaming server
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "VideoStreamIGTLinkServer.h"
#if defined(OpenIGTLink_USE_H264)
  #include "H264Encoder.h"
#endif

#if defined(OpenIGTLink_USE_VP9)
  #include "VP9Encoder.h"
#endif


#if defined(ANDROID_NDK) || defined(APPLE_IOS) || defined (WINDOWS_PHONE)
extern "C" int EncMain (int argc, char** argv)
#else
int main (int argc, char** argv)
#endif
{
  if (argc != 2) // check number of arguments
  {
    // If not correct, print usage
    std::cerr << "Usage: " << argv[0] << " <configurationfile>"    << std::endl;
    std::cerr << "    <configurationfile> : file name "  << std::endl;
    exit(0);
  }
  
  VideoStreamIGTLinkServer server(argv[1]);
#if defined(OpenIGTLink_USE_VP9)
  VP9Encoder* encoder = new VP9Encoder();
  encoder->SetSpeed(6);
#elif defined(OpenIGTLink_USE_H264)
  H264Encoder* encoder = new H264Encoder();
#endif
  server.SetEncoder(encoder);
  server.SetupServer();
  server.InitializeEncoder();
  server.SetWaitSTTCommand(true);
  if(server.transportMethod==server.UseTCP)
  {
    server.StartTCPServer();
  }
  else if(server.transportMethod==server.UseUDP)
  {
    server.StartUDPServer();
  }
  server.StartSendPacketThread();
  server.StartReadFrameThread(30);
  while(1)
  {
    igtl::Sleep(10);
    if(server.transportMethod==server.UseTCP)
    {
      if(server.GetServerConnectStatus())
      {
        if (!server.GetServerSetStatus())
        {
          server.SetupServer();
        }
        if(server.useCompress == 1)
        {
          int iRet = server.EncodeFile();
          if (iRet == -1)
          {
            break;
          }
        }
        else
        {
          server.SendOriginalData();
        }
      }
    }
    else if(server.transportMethod==server.UseUDP)
    {
      if (!server.GetServerSetStatus())
      {
        server.SetupServer();
      }
      if(server.useCompress == 1)
      {
        int iRet = server.EncodeFile();
        if (iRet == -1)
        {
          break;
        }
      }
      else
      {
        server.SendOriginalData();
      }
    }
  }
  server.Stop();
  return 0;
}
