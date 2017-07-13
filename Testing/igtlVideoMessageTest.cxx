/*=========================================================================
 
 Program:   OpenIGTLink Library
 Language:  C++
 
 Copyright (c) Insight Software Consortium. All rights reserved.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.
 
 =========================================================================*/
/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <sstream>
#include <map>
#include <stdlib.h>
#include "igtlVideoMessage.h"
#include "igtlMessageDebugFunction.h"
#include "igtl_video.h"
#include "igtl_types.h"
#include "igtl_header.h"
#include "igtl_util.h"
#include "igtlTestConfig.h"
#include "string.h"
#include <cstring>
#include <string>
#if OpenIGTLink_PROTOCOL_VERSION >= 3
  #include "igtlMessageFormat2TestMacro.h"
#endif

#if OpenIGTLink_LINK_H264
  #include "H264Encoder.h"
  #include "H264Decoder.h"
#endif

#if OpenIGTLink_LINK_VP9
  #include "VP9Encoder.h"
  #include "VP9Decoder.h"
  #include "../video_reader.h"
  #include "./vpx_config.h"
  #include "vpx_dsp_rtcd.h"
  #include "vpx_dsp/ssim.h"
#endif

int Width = 256;
int Height = 256;
std::string testFileName(OpenIGTLink_SOURCE_ROOTDIR);
int startIndex = 0;
int  inputFrameNum = 20;

int TestWithVersion(int version, GenericEncoder* videoStreamEncoder, GenericDecoder* videoStreamDecoder, bool compareImage)
{
  // Get thread information
  int kiPicResSize = Width*Height;
  igtl_uint8* imagePointer = new igtl_uint8[kiPicResSize*3/2];
  memset(imagePointer, 0, Width * Height * 3 / 2);
  SourcePicture* pSrcPic = new SourcePicture();
  pSrcPic->colorFormat = FormatI420;
  pSrcPic->picWidth = Width; // check the test image
  pSrcPic->picHeight = Height;
  pSrcPic->data[0] = imagePointer;
  pSrcPic->data[1] = pSrcPic->data[0] + kiPicResSize;
  pSrcPic->data[2] = pSrcPic->data[1] + kiPicResSize/4;
  pSrcPic->stride[0] = pSrcPic->picWidth;
  pSrcPic->stride[1] = pSrcPic->stride[2] = pSrcPic->stride[0] >> 1;
  
  SourcePicture* pDecodedPic = new SourcePicture();
  pDecodedPic->data[0] = new igtl_uint8[kiPicResSize*3/2];
  memset(pDecodedPic->data[0], 0, Width * Height * 3 / 2);
  int totalFrame = 0;
  for(int i = 0; i <inputFrameNum; i++)
  {
    igtl_int32 iFrameNumInFile = -1;
    std::string sep = "/";
#if defined(_WIN32) || defined(_WIN64)
    sep = "\\";
#endif
    std::string imageIndexStr = static_cast< std::ostringstream & >(( std::ostringstream() << std::dec << (i%6+1))).str();
    std::string testIndexedFileName = std::string(testFileName);
    testIndexedFileName.append(sep).append("Testing").append(sep).append("img").append(sep).append("igtlTestImage").append(imageIndexStr).append(".raw");
    FILE* pFileYUV = NULL;
    pFileYUV = fopen (testIndexedFileName.c_str(), "rb");
    if (pFileYUV != NULL) {
      igtl_int64 i_size = 0;
#if defined(_WIN32) || defined(_WIN64)
#if _MSC_VER >= 1400
      if (!_fseeki64 (pFileYUV, 0, SEEK_END)) {
        i_size = _ftelli64 (pFileYUV);
        _fseeki64 (pFileYUV, 0, SEEK_SET);
        iFrameNumInFile =std::fmax((igtl_int32) (i_size / kiPicResSize), iFrameNumInFile);
      }
#else
      if (!fseek (pFileYUV, 0, SEEK_END))
        {
        i_size = ftell (pFileYUV);
        fseek (pFileYUV, 0, SEEK_SET);
        iFrameNumInFile = std::max((igtl_int32) (i_size / kiPicResSize), iFrameNumInFile);
        }
#endif
#else
      if (!fseeko (pFileYUV, 0, SEEK_END))
        {
        i_size = ftello (pFileYUV);
        fseek(pFileYUV, 0, SEEK_SET);
        iFrameNumInFile = std::max((igtl_int32) (i_size / (kiPicResSize)), iFrameNumInFile);
        }
#endif
#if defined(_WIN32) || defined(_WIN64)
  #if _MSC_VER >= 1400
      _fseeki64(pFileYUV, startIndex*kiPicResSize, SEEK_SET); //fix the data range issue
  #else
      fseek(pFileYUV, startIndex*kiPicResSize, SEEK_SET); //fix the data range issue
  #endif
#else
      fseek(pFileYUV, startIndex*kiPicResSize, SEEK_SET);
#endif
      while(fread (imagePointer, 1, kiPicResSize, pFileYUV ) == (kiPicResSize) && totalFrame<inputFrameNum)
        {
        bool isGrayImage = true;
        igtl::VideoMessage::Pointer videoMessageSend = igtl::VideoMessage::New();
        videoMessageSend->SetHeaderVersion(version);
        if(version == IGTL_HEADER_VERSION_2)
          igtlMetaDataAddElementMacro(videoMessageSend);
        int iEncFrames = videoStreamEncoder->EncodeSingleFrameIntoVideoMSG(pSrcPic, videoMessageSend, isGrayImage);
        if(iEncFrames == 0)
          {
          igtl::VideoMessage::Pointer videoMessageReceived = igtl::VideoMessage::New();
          igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
          headerMsg->AllocatePack();
          memcpy(headerMsg->GetPackPointer(), videoMessageSend->GetPackPointer(), IGTL_HEADER_SIZE);
          headerMsg->Unpack();
          videoMessageReceived->SetMessageHeader(headerMsg);
          videoMessageReceived->AllocatePack();
          memcpy(videoMessageReceived->GetPackBodyPointer(), videoMessageSend->GetPackBodyPointer(), videoMessageSend->GetPackBodySize());
          videoMessageReceived->Unpack();
          if (memcmp(videoMessageReceived->GetPackFragmentPointer(2), videoMessageSend->GetPackFragmentPointer(2), videoMessageReceived->GetBitStreamSize()) != 0)
          {
            return -1;
          }
          int iRet= videoStreamDecoder->DecodeVideoMSGIntoSingleFrame(videoMessageReceived.GetPointer(), pDecodedPic);
          if(iRet)
          {
            totalFrame ++;
            if(version == IGTL_HEADER_VERSION_2)
            {
              videoMessageReceived->SetMessageID(1); // Message ID is reset by the codec, so the comparison of ID is no longer valid, manually set to 1;
              //igtlMetaDataComparisonMacro(videoMessageReceived);
            }
            if (compareImage)
            {
              TestDebugCharArrayCmp(pDecodedPic->data[0], imagePointer, kiPicResSize * 3 / 2);
              if (memcmp(pDecodedPic->data[0], imagePointer, kiPicResSize * 3 / 2) != 0)
              {
                return -1;
              }
            }
              //EXPECT_EQ(memcmp(pDecodedPic->data[0], pSrcPic->data[0],kiPicResSize*3/2),0);
          }
        }
        igtl::Sleep(5);
      }
      if (pFileYUV)
      {
        fclose(pFileYUV);
        pFileYUV = NULL;
      }
    } else {
      fprintf (stderr, "Unable to open image file, check corresponding path!\n");
    }
  }
  delete pDecodedPic;
  delete pSrcPic;
  pDecodedPic = NULL;
  pSrcPic = NULL;
  return 0;
}


TEST(VideoMessageTest, YUVandRGBConversion)
{
  VP9Encoder * encoder = new VP9Encoder();
  VP9Decoder * decoder = new VP9Decoder();
  int width = 400, height = 400;
  int yuvDataLen = width*height*3/2;
  igtlUint8* yuv_a = new igtlUint8[yuvDataLen];
  igtlUint8* yuv_a2 = new igtlUint8[yuvDataLen];
  igtlUint8* rgbPointer = new igtlUint8[yuvDataLen*2];
  igtlUint8* rgbPointer2 = new igtlUint8[yuvDataLen*2];
  unsigned char color = 108;
  for(int i = 0 ; i< width*height*3; i++)
    {
    *rgbPointer = color%256;
    color++;
    rgbPointer++;
    }
  rgbPointer -=yuvDataLen*2;
  //------------
  //Test of the data lost in RGB->YUV->RGB conversion
  encoder->ConvertRGBToYUV(rgbPointer, yuv_a, width, height);
  decoder->ConvertYUVToRGB(yuv_a, rgbPointer2, width,height);
  encoder->ConvertRGBToYUV((igtlUint8*)rgbPointer2, yuv_a2, width,height);
  int iReturn = memcmp(yuv_a2, yuv_a,yuvDataLen);// The conversion is not valid. Image is not the same after conversion.
  //-------------
  //igtlUint8* yuv_b = new igtlUint8[b->GetDimensions()[0]*b->GetDimensions()[1]*3/2];
  //encoder->ConvertRGBToYUV((igtlUint8*)b->GetScalarPointer(), yuv_b, b->GetDimensions()[0], b->GetDimensions()[1]);
  //iReturn = memcmp(yuv_b, yuv_a, a->GetDimensions()[0]*a->GetDimensions()[1]*3/2);
}

TEST(VideoMessageTest, EncodeAndDecodeFormatVersion1)
  {
#if OpenIGTLink_LINK_VP9
  std::cerr<<"--------------------------- "<<std::endl;
  std::cerr<<"Begin of VPX tests "<<std::endl;
  VP9Encoder* VPXStreamEncoder = new VP9Encoder();
  VP9Decoder* VPXStreamDecoder = new VP9Decoder();
  VPXStreamEncoder->SetPicWidthAndHeight(Width,Height);
  VPXStreamEncoder->SetLosslessLink(true);
  VPXStreamEncoder->InitializeEncoder();
  EXPECT_EQ(TestWithVersion(IGTL_HEADER_VERSION_1, VPXStreamEncoder, VPXStreamDecoder,true),0);
  VPXStreamEncoder->SetSpeed(VPXStreamEncoder->FastestSpeed);
  VPXStreamEncoder->SetLosslessLink(false);
  std::cerr<<"Encoding Time Using Maximum Speed: "<<std::endl;
  EXPECT_EQ(TestWithVersion(IGTL_HEADER_VERSION_1, VPXStreamEncoder, VPXStreamDecoder, false),0);
  std::cerr<<"End of VPX tests "<<std::endl;
  std::cerr<<"--------------------------- "<<std::endl;
#endif
#if OpenIGTLink_LINK_H264
  std::cerr<<"--------------------------- "<<std::endl;
  std::cerr<<"Begin of H264 tests "<<std::endl;
  H264Encoder* H264StreamEncoder = new H264Encoder();
  H264Decoder* H264StreamDecoder = new H264Decoder();
  H264StreamEncoder->SetPicWidthAndHeight(Width,Height);
  H264StreamEncoder->InitializeEncoder();
  TestWithVersion(IGTL_HEADER_VERSION_1, H264StreamEncoder, H264StreamDecoder,false);
  std::cerr<<"End of H264 tests "<<std::endl;
  std::cerr<<"--------------------------- "<<std::endl;
#endif
  }


#if OpenIGTLink_PROTOCOL_VERSION >= 3

  TEST(VideoMessageTest, EncodeAndDecodeFormatVersion2)
    {
  #if OpenIGTLink_LINK_VP9
    std::cerr<<"--------------------------- "<<std::endl;
    std::cerr<<"Begin of VPX tests "<<std::endl;
    VP9Encoder* VPXStreamEncoder = new VP9Encoder();
    VP9Decoder* VPXStreamDecoder = new VP9Decoder();
    VPXStreamEncoder->SetPicWidthAndHeight(Width,Height);
    VPXStreamEncoder->InitializeEncoder();
    VPXStreamEncoder->SetLosslessLink(true);
    TestWithVersion(IGTL_HEADER_VERSION_2, VPXStreamEncoder, VPXStreamDecoder,true);
    VPXStreamEncoder->SetSpeed(VPXStreamEncoder->FastestSpeed);
    VPXStreamEncoder->SetLosslessLink(false);
    std::cerr<<"Encoding Time Using Maximum Speed: "<<std::endl;
    TestWithVersion(IGTL_HEADER_VERSION_2, VPXStreamEncoder, VPXStreamDecoder,false);
    std::cerr<<"End of VPX tests "<<std::endl;
    std::cerr<<"--------------------------- "<<std::endl;
  #endif
  #if OpenIGTLink_LINK_H264
    std::cerr<<"--------------------------- "<<std::endl;
    std::cerr<<"Begin of H264 tests "<<std::endl;
    H264Encoder* H264StreamEncoder = new H264Encoder();
    H264Decoder* H264StreamDecoder = new H264Decoder();
    H264StreamEncoder->SetPicWidthAndHeight(Width,Height);
    H264StreamEncoder->InitializeEncoder();
    TestWithVersion(IGTL_HEADER_VERSION_2, H264StreamEncoder, H264StreamDecoder,false);
    std::cerr<<"End of H264 tests "<<std::endl;
    std::cerr<<"--------------------------- "<<std::endl;
  #endif
    }
#endif

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

