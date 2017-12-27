//
//  CodecEvaluation.cpp
//  OpenIGTLink
//
//  Created by Longquan Chen on 4/28/17.
//
//

#include <stdio.h>


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
#include "igtlVideoMessage.h"
#include "igtlMessageDebugFunction.h"
#include "igtl_video.h"
#include "igtl_types.h"
#include "igtl_header.h"
#include "igtl_util.h"
#include "string.h"
#include <cstring>
#include <string>

#if defined(OpenIGTLink_USE_H264)
#include "H264Encoder.h"
#include "H264Decoder.h"
#endif

#if defined(OpenIGTLink_USE_VP9)
#include "VP9Encoder.h"
#include "VP9Decoder.h"
#include "../video_reader.h"
#include "./vpx_config.h"
#include "vpx_dsp_rtcd.h"
#include "vpx_dsp/ssim.h"
#endif

#if defined(OpenIGTLink_USE_X265)
#include "H265Encoder.h"
#include "H265Decoder.h"
#endif

double ssim = 0.0;

int Width = 446;
int Height = 460;
std::string sendFileDir("/Users/longquanchen/Documents/VideoStreaming/PaperCodecOptimization/EvaluationUltraSonixAndHospitalNetwork/EvaluationWithSSIM/VP9-TargetBitRate5/SendFrames/");
std::string receivedFileDir("/Users/longquanchen/Documents/VideoStreaming/PaperCodecOptimization/EvaluationUltraSonixAndHospitalNetwork/EvaluationWithSSIM/VP9-TargetBitRate5/ReceivedFrames/");
std::string evalFileName("/Users/longquanchen/Documents/VideoStreaming/PaperCodecOptimization/EvaluationUltraSonixAndHospitalNetwork/EvaluationWithSSIM/VP9-TargetBitRate5/EvalFile.txt");
FILE* pEval = NULL;

template <typename T>
std::string ToString(T variable)
{
  std::stringstream stream;
  stream << variable;
  return stream.str();
}

//------------------------
//Taken from the libvpx and modified, as the original code is broken,
//will be removed when the original code bug is fixed.
void vpx_ssim_parms_8x8_c(const uint8_t *s, int sp, const uint8_t *r, int rp,
                          uint32_t *sum_s, uint32_t *sum_r, uint32_t *sum_sq_s,
                          uint32_t *sum_sq_r, uint32_t *sum_sxr) {
  int i, j;
  for (i = 0; i < 8; i++, s += sp, r += rp) {
    for (j = 0; j < 8; j++) {
      *sum_s += s[j];
      *sum_r += r[j];
      *sum_sq_s += s[j] * s[j];
      *sum_sq_r += r[j] * r[j];
      *sum_sxr += s[j] * r[j];
    }
  }
}

static const int64_t cc1 = 26634;        // (64^2*(.01*255)^2
static const int64_t cc2 = 239708;       // (64^2*(.03*255)^2
static const int64_t cc1_10 = 428658;    // (64^2*(.01*1023)^2
static const int64_t cc2_10 = 3857925;   // (64^2*(.03*1023)^2
static const int64_t cc1_12 = 6868593;   // (64^2*(.01*4095)^2
static const int64_t cc2_12 = 61817334;  // (64^2*(.03*4095)^2

static double similarity(uint32_t sum_s, uint32_t sum_r, uint32_t sum_sq_s,
                         uint32_t sum_sq_r, uint32_t sum_sxr, int count,
                         uint32_t bd) {
  int64_t ssim_n, ssim_d;
  int64_t c1, c2;
  if (bd == 8) {
    // scale the constants by number of pixels
    c1 = (cc1 * count * count) >> 12;
    c2 = (cc2 * count * count) >> 12;
  } else if (bd == 10) {
    c1 = (cc1_10 * count * count) >> 12;
    c2 = (cc2_10 * count * count) >> 12;
  } else if (bd == 12) {
    c1 = (cc1_12 * count * count) >> 12;
    c2 = (cc2_12 * count * count) >> 12;
  } else {
    c1 = c2 = 0;
    //assert(0);
  }
  
  ssim_n = (2 * sum_s * sum_r + c1) *
  ((int64_t)2 * count * sum_sxr - (int64_t)2 * sum_s * sum_r + c2);
  
  ssim_d = (sum_s * sum_s + sum_r * sum_r + c1) *
  ((int64_t)count * sum_sq_s - (int64_t)sum_s * sum_s +
   (int64_t)count * sum_sq_r - (int64_t)sum_r * sum_r + c2);
  
  return ssim_n * 1.0 / ssim_d;
}

static double local_vpx_ssim2(const uint8_t *img1, const uint8_t *img2,
                              int stride_img1, int stride_img2, int width,
                              int height) {
  int i, j;
  int samples = 0;
  double ssim_total = 0;
  
  // sample point start with each 4x4 location
  for (i = 0; i <= height - 8;
       i += 4, img1 += stride_img1 * 4, img2 += stride_img2 * 4) {
    for (j = 0; j <= width - 8; j += 4) {
      uint32_t sum_s = 0, sum_r = 0, sum_sq_s = 0, sum_sq_r = 0, sum_sxr = 0;
      vpx_ssim_parms_8x8_c(img1 + j, stride_img1, img2 + j, stride_img2, &sum_s, &sum_r, &sum_sq_s, &sum_sq_r,
                           &sum_sxr);
      double v = similarity(sum_s, sum_r, sum_sq_s, sum_sq_r, sum_sxr, 64, 8);
      ssim_total += v;
      samples++;
    }
  }
  ssim_total /= samples;
  return ssim_total;
}
//Taken from the libvpx and modified, as the original code is broken,
//will be removed when the original code bug is fixed.
//-----------------------------------
void TestWithVersion()
{
  // Get thread information
  int kiPicResSize = Width*Height;
  SourcePicture* pSrcPic = new SourcePicture();
  pSrcPic->colorFormat = FormatI420;
  pSrcPic->picWidth = Width; // check the test image
  pSrcPic->picHeight = Height;
  pSrcPic->data[0] = new igtl_uint8[kiPicResSize];
  pSrcPic->data[1] = pSrcPic->data[0] + kiPicResSize;
  pSrcPic->data[2] = pSrcPic->data[1] + kiPicResSize/4;
  pSrcPic->stride[0] = pSrcPic->picWidth;
  pSrcPic->stride[1] = pSrcPic->stride[2] = pSrcPic->stride[0] >> 1;
  
  SourcePicture* pDecodedPic = new SourcePicture();
  pDecodedPic->data[0] = new igtl_uint8[kiPicResSize];
  memset(pDecodedPic->data[0], 0, Width * Height);
  pEval = fopen (evalFileName.c_str(), "a");
  std::string assembledReceivedVideo = "/Users/longquanchen/Documents/VideoStreaming/PaperCodecOptimization/EvaluationUltraSonixAndHospitalNetwork/EvaluationWithSSIM/VP9-TargetBitRate5/ReceivedVideo.yuv";
  std::string assembledSendVideo = "/Users/longquanchen/Documents/VideoStreaming/PaperCodecOptimization/EvaluationUltraSonixAndHospitalNetwork/EvaluationWithSSIM/VP9-TargetBitRate5/SendVideo.yuv";
  for(int i = 0; i <50000; i++)
  {
    std::string sep = "/";
#if defined(_WIN32) || defined(_WIN64)
    sep = "\\";
#endif
    std::string imageIndexStr = static_cast< std::ostringstream & >(std::ostringstream() << std::dec << (i)).str();
    std::string tempDir = sendFileDir;
    std::string sendFrameFileName = tempDir.append(imageIndexStr).append(".yuv");
    tempDir = receivedFileDir;
    std::string recievedFrameFileName = tempDir.append(imageIndexStr).append(".yuv");
    FILE* pFileYUVSend = NULL;
    pFileYUVSend = fopen (sendFrameFileName.c_str(), "rb");
    FILE* pFileYUVReceived = NULL;
    pFileYUVReceived = fopen (recievedFrameFileName.c_str(), "rb");
    //FILE* pFileYUVReceivedVideo = fopen (assembledReceivedVideo.c_str(), "a");
    //FILE* pFileYUVSendVideo = fopen (assembledSendVideo.c_str(), "a");
    if (pFileYUVSend != NULL && pFileYUVReceived != NULL) {
      if(fread (pSrcPic->data[0], 1, kiPicResSize, pFileYUVSend ) == kiPicResSize &&
         fread (pDecodedPic->data[0], 1, kiPicResSize, pFileYUVReceived ) == kiPicResSize )
      {
        ssim = local_vpx_ssim2(pSrcPic->data[0], pDecodedPic->data[0],pSrcPic->stride[0], pSrcPic->stride[0],pSrcPic->picWidth, pSrcPic->picHeight);
        std::string localline = std::string("");
        localline.append(ToString(i));
        localline.append(" ");
        localline.append(ToString(ssim));
        localline.append(" ");
        localline.append("\r\n");
        fwrite(localline.c_str(), 1, localline.size(), pEval);
        //fwrite(pSrcPic->data[0], 1, kiPicResSize, pFileYUVReceivedVideo);
        //fwrite(pDecodedPic->data[0], 1, kiPicResSize, pFileYUVSendVideo);
        if (pFileYUVSend && pFileYUVReceived)
        {
          fclose(pFileYUVSend);
          pFileYUVSend = NULL;
          fclose(pFileYUVReceived);
          pFileYUVReceived = NULL;
          //fclose(pFileYUVReceivedVideo);
          //fclose(pFileYUVSendVideo);
        }
      }
    }
  }
  fclose(pEval);
}


int main(int argc, char **argv)
{
  TestWithVersion();
  return 0;
}
