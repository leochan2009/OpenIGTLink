/*=========================================================================
 
 Program:   H265Encoder
 Language:  C++
 
 Copyright (c) Insight Software Consortium. All rights reserved.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.
 
 =========================================================================*/

#include "H265Decoder.h"

H265Decoder::H265Decoder()
{
  this->pDecoder->create();
  pDecoder->setDecodedPictureHashSEIEnabled(0);
  this->deviceName = "";
  stats = AnnexBStats();
  std::istream  stream();
  //bytestream = new InputByteStream(&stream);
}

H265Decoder::~H265Decoder()
{
  pDecoder->destroy();
  pDecoder = NULL;
}

igtl_int32 iFrameCountTotal = 0;


void H265Decoder::ComposeByteSteam(igtl_uint8** inputData, int dimension[2], int iStride[2], igtl_uint8 *outputFrame)
{
  int iWidth = dimension[0];
  int iHeight = dimension[1];
#pragma omp parallel for default(none) shared(outputByteStream,inputData, iStride, iHeight, iWidth)
  for (int i = 0; i < iHeight; i++)
  {
    igtl_uint8* pPtr = inputData[0]+i*iStride[0];
    for (int j = 0; j < iWidth; j++)
    {
      outputFrame[i*iWidth + j] = pPtr[j];
    }
  }
#pragma omp parallel for default(none) shared(outputByteStream,inputData, iStride, iHeight, iWidth)
  for (int i = 0; i < iHeight/2; i++)
  {
    igtl_uint8* pPtr = inputData[1]+i*iStride[1];
    for (int j = 0; j < iWidth/2; j++)
    {
      outputFrame[i*iWidth/2 + j + iHeight*iWidth] = pPtr[j];
    }
  }
#pragma omp parallel for default(none) shared(outputByteStream, inputData, iStride, iHeight, iWidth)
  for (int i = 0; i < iHeight/2; i++)
  {
    igtl_uint8* pPtr = inputData[2]+i*iStride[1];
    for (int j = 0; j < iWidth/2; j++)
    {
      outputFrame[i*iWidth/2 + j + iHeight*iWidth*5/4] = pPtr[j];
    }
  }
}


int H265Decoder::DecodeVideoMSGIntoSingleFrame(igtl::VideoMessage* videoMessage, SourcePicture* pDecodedPic)
{
  if(videoMessage->GetBitStreamSize())
  {
    igtl_int32 iWidth = videoMessage->GetWidth();
    igtl_int32 iHeight = videoMessage->GetHeight();
    igtl_uint64 iStreamSize = videoMessage->GetBitStreamSize();
    igtl_uint16 frameType = videoMessage->GetFrameType();
    isGrayImage = false;
    if (frameType>0X00FF)
    {
      frameType= frameType>>8;
      isGrayImage = true;
    }
    pDecodedPic->picWidth = iWidth;
    pDecodedPic->picHeight = iHeight;
    pDecodedPic->data[1]= pDecodedPic->data[0] + iWidth*iHeight;
    pDecodedPic->data[2]= pDecodedPic->data[1] + iWidth*iHeight/4;
    pDecodedPic->stride[0] = iWidth;
    pDecodedPic->stride[1] = pDecodedPic->stride[2] = iWidth>>1;
    pDecodedPic->stride[3] = 0;
    igtl_uint32 dimensions[2] = {iWidth, iHeight};
    int iRet = this->DecodeBitStreamIntoFrame(videoMessage->GetPackFragmentPointer(2), pDecodedPic->data[0], dimensions, iStreamSize);
    return iRet;
  }
  return -1;
}

int H265Decoder::DecodeBitStreamIntoFrame(unsigned char* kpH265BitStream,igtl_uint8* outputFrame, igtl_uint32 dimensions[2], igtl_uint64& iStreamSize) {
  int m_iSkipFrame = 0;
  int m_iPOCLastDisplay = m_iSkipFrame;
  byteStreamNALUnit(*bytestream, nalu.getBitstream().getFifo(), stats);
  read(nalu);
  
  Bool bNewPicture = pDecoder->decode(nalu, m_iSkipFrame, m_iPOCLastDisplay);
  if (bNewPicture)
  {
    return 0;
  }
}
