/*=========================================================================

  Program:   OpenIGTLink
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __igtlVP9Encoder_h
#define __igtlVP9Encoder_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vp8cx.h"
#include "vpx_image.h"

#include "igtlCodecCommonClasses.h"
#include "igtl_header.h"
#include "igtl_video.h"
#include "igtlOSUtil.h"
#include "igtlMessageHeader.h"
#include "igtlVideoMessage.h"
#include "igtlTimeStamp.h"

namespace igtl {

#ifdef __cplusplus
extern "C" {
#endif
  typedef struct VpxInterfaceEncoder {
    vpx_codec_iface_t *(*const codec_interface)();
  } VpxInterfaceEncoder;
  
#ifdef __cplusplus
} /* extern "C" */
#endif

using namespace std;

class IGTLCommon_EXPORT VP9Encoder: public GenericEncoder
{
public:
  igtlTypeMacro(VP9Encoder, GenericEncoder);
  igtlNewMacro(VP9Encoder);

  VP9Encoder(char * configFile = NULL);
  ~VP9Encoder();

  int FillSpecificParameters() override;
  
  /**
   Parse the configuration file to initialize the encoder and server.
   */
  int InitializeEncoder() override;
  
  int ConvertToLocalImageFormat(SourcePicture* pSrcPic)  override;
  
  /**
   Encode a frame, for performance issue, before encode the frame, make sure the frame pointer is updated with a new frame.
   Otherwize, the old frame will be encoded.
   */
  int EncodeSingleFrameIntoVideoMSG(SourcePicture* pSrcPic, igtl::VideoMessage* videoMessage, bool isGrayImage = false )  override;
  
  int SetPicWidthAndHeight(unsigned int width, unsigned int height)  override;
  
  unsigned int GetPicWidth() override {return this->picWidth;};
  
  unsigned int GetPicHeight() override {return this->picHeight;};
  
  int SetRCMode(int value) override;

  virtual int SetImageFormat(VideoFormatType value);
  
  int SetKeyFrameDistance(int frameNum) override;
  
  int SetQP(int maxQP, int minQP) override;
  
  int SetRCTaregetBitRate(unsigned int bitRate) override;
  
  int SetLosslessLink(bool linkMethod) override;
  
  /* Speed value Should be set between -8 to 8 for VP9.
     Values greater than 0 will increase encoder speed at the expense of quality.
     negative value is not used.*/
  enum
  {
    SlowestSpeed = 0,
    FastestSpeed = 8
  };
  int SetSpeed(int speed) override;
  
  /*!\brief deadline parameter analogous to VPx REALTIME mode. */
  //#define VPX_DL_REALTIME (1)
  /*!\brief deadline parameter analogous to  VPx GOOD QUALITY mode. */
  //#define VPX_DL_GOOD_QUALITY (1000000)
  /*!\brief deadline parameter analogous to VPx BEST QUALITY mode. */
  //#define VPX_DL_BEST_QUALITY (0)
  /*!\brief Encode a frame*/
  int SetDeadlineMode(unsigned long mode);
  
private:
  
  const VpxInterfaceEncoder *encoder;
  
  void error_output(vpx_codec_ctx_t *ctx, const char *s);
  
  int vpx_img_plane_width(const vpx_image_t *img, int plane);
  
  int vpx_img_plane_height(const vpx_image_t *img, int plane);
  
  vpx_codec_enc_cfg_t cfg;
  
  vpx_codec_ctx_t* codec;
  
  vpx_image_t* inputImage;
  
  vpx_codec_iter_t iter;
  
  const vpx_fixed_buf_t *encodedBuf;
  
  const vpx_codec_cx_pkt_t *pkt;
  
  unsigned long deadlineMode;
  
};


}// Namespace igtl
#endif
