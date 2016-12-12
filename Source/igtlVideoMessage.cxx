/*=========================================================================

  Program:   The OpenIGTLink Library
  Language:  C++
  Web page:  http://openigtlink.org/

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "igtlVideoMessage.h"

#include <string.h>

namespace igtl {

  
StartVideoDataMessage::~StartVideoDataMessage()
{
}

int StartVideoDataMessage::GetBodyPackSize()
{
  return IGTL_STT_VIDEO_SIZE;
}

int StartVideoDataMessage::CalculateContentBufferSize()
{
  return IGTL_STT_VIDEO_SIZE;
}
  
int StartVideoDataMessage::PackContent()
{
  AllocatePack();
  
  igtl_stt_video* stt_video = (igtl_stt_video*)this->m_Content;
  strncpy(stt_video->codec, this->m_CodecType.c_str(), IGTL_VIDEO_CODEC_NAME_SIZE);
  stt_video->time_interval = this->m_TimeInterval;
  stt_video->useCompress = this->m_UseCompress;
  
  igtl_stt_video_convert_byte_order(stt_video);
  
  return 1;
  
}


int StartVideoDataMessage::UnpackContent()
{
  igtl_stt_video* stt_video = (igtl_stt_video*)this->m_Content;
  
  igtl_stt_video_convert_byte_order(stt_video);
  
  this->m_TimeInterval = stt_video->time_interval;
  this->m_UseCompress = stt_video->useCompress;
  this->m_CodecType = std::string(stt_video->codec);
  return 1;
}

  
  
VideoMessage::VideoMessage():
  MessageBase()
{
  endian        = ENDIAN_BIG;
  scalarType    = TYPE_UINT8;
  m_FrameHeader = NULL;
  m_Frame       = NULL;

  m_SendMessageType  = "Video";

  ScalarSizeTable[0] = 0;
  ScalarSizeTable[1] = 0;
  ScalarSizeTable[2] = sizeof(igtlInt8);    // TYPE_INT8
  ScalarSizeTable[3] = sizeof(igtlUint8);   // TYPE_UINT8
  ScalarSizeTable[4] = sizeof(igtlInt16);   // TYPE_INT16 
  ScalarSizeTable[5] = sizeof(igtlUint16);  // TYPE_UINT16
  ScalarSizeTable[6] = sizeof(igtlInt32);   // TYPE_INT32
  ScalarSizeTable[7] = sizeof(igtlUint32);  // TYPE_UINT32
  ScalarSizeTable[8] = 0;                   // not defined
  ScalarSizeTable[9] = 0;                   // not defined

  this->m_Body   = NULL;
  m_Frame = NULL;
  m_FrameHeader = NULL;

}


VideoMessage::~VideoMessage()
{
  /*if (this->m_FrameHeader)
    {
    delete [] this->m_FrameHeader;
    }
  if (this->m_Frame && this->m_FrameHeader!= this->m_Body)
  {
    delete [] this->m_Frame;
  }
  if(this->m_Body)
  {
    delete [] this->m_Body;
  }*/
}

void VideoMessage::AllocateScalars()
{
  // Memory area to store frame scalar is allocated with
  // message and frame header, by using AllocatePack() implemented
  // in the parent class.
  int s = GetBitStreamSize();
  this->m_MessageSize = IGTL_HEADER_SIZE + IGTL_VIDEO_HEADER_SIZE + s;

  AllocatePack(IGTL_VIDEO_HEADER_SIZE + s);
  this->m_FrameHeader = m_Body;
  this->m_Frame  = &m_FrameHeader[IGTL_VIDEO_HEADER_SIZE];
  //this->m_Frame = new unsigned char [s];
}

  
void VideoMessage::AllocateBuffer()
{
  MessageBase::AllocateBuffer();
#if OpenIGTLink_HEADER_VERSION >= 2
  if (m_HeaderVersion == IGTL_HEADER_VERSION_2)
  {
    this->m_FrameHeader = &m_Body[sizeof(igtl_extended_header)];
  }
  else
  {
    this->m_FrameHeader = m_Body;
  }
  this->m_Frame  = &m_FrameHeader[IGTL_VIDEO_HEADER_SIZE];
#else
  this->m_FrameHeader = m_Body;
  this->m_Frame = m_Body;
#endif
}
  
  
int VideoMessage::CalculateContentBufferSize()
{
  return bitStreamSize + IGTL_VIDEO_HEADER_SIZE;
}


void* VideoMessage::GetScalarPointer()
{
  return (void*)m_Frame;
}

void  VideoMessage::SetScalarPointer(unsigned char * p)
{
  if (this->m_Frame)
  {
    delete [] this->m_Frame;
  }
  this->m_Frame =  p;
}


unsigned char* VideoMessage::GetPackFragmentPointer(int id)
{
  if (id == 0)
    {
    return this->m_Header;
    }
  else if (id == 1)
    {
    return this->m_FrameHeader;
    }
  else if (id == 2)
    {
    return this->m_Frame;
    }

  return NULL;

}


int VideoMessage::GetPackFragmentSize(int id)
{
  if (id == 0)
    {
    return IGTL_HEADER_SIZE;
    }
  else if (id == 1)
    {
    return IGTL_VIDEO_HEADER_SIZE;
    }
  else if (id == 2)
    {
    return GetBitStreamSize();
    }

  return 0;
}

int VideoMessage::GetBodyPackSize()
{
  // This function is called by:
  //   MessageBase::Pack()
  //   MessageBase::AllocatePack()
#if OpenIGTLink_HEADER_VERSION >= 2
  int size(-1);
  if (m_HeaderVersion == IGTL_HEADER_VERSION_2)
  {
    size =  IGTL_VIDEO_HEADER_SIZE + bitStreamSize + sizeof(igtl_extended_header) + GetMetaDataHeaderSize() + GetMetaDataSize();
  }
  else
  {
    size =  IGTL_VIDEO_HEADER_SIZE + bitStreamSize;
  }
#else
  int size = IGTL_HEADER_SIZE + bitStreamSize;
#endif
  return size;
}

int VideoMessage::PackContent()
{
  igtl_frame_header* frame_header = (igtl_frame_header*)this->m_FrameHeader;

  frame_header->version           = IGTL_VIDEO_HEADER_VERSION;
  frame_header->scalar_type       = this->scalarType;
  frame_header->endian            = this->endian;
  frame_header->width             = this->width;
  frame_header->height            = this->height;
  igtl_frame_convert_byte_order(frame_header);
  m_MetaDataHeader = &m_Body[CalculateContentBufferSize()+sizeof(igtl_extended_header)];
  return 1;

}


int VideoMessage::UnpackContent()
{
#if OpenIGTLink_HEADER_VERSION >= 2
  if (m_HeaderVersion == IGTL_HEADER_VERSION_2)
  {
    this->m_FrameHeader = &m_Body[sizeof(igtl_extended_header)];
  }
  else
  {
    this->m_FrameHeader = m_Body;
  }
  this->m_Frame  = &m_FrameHeader[IGTL_VIDEO_HEADER_SIZE];
#else
  this->m_FrameHeader = m_Body;
  this->m_Frame = m_Body;
#endif
  
  igtl_frame_header* frame_header = (igtl_frame_header*)m_FrameHeader;
  igtl_frame_convert_byte_order(frame_header);

  if (frame_header->version == IGTL_VIDEO_HEADER_VERSION)
  {
      // Video format version 1
      this->scalarType       = frame_header->scalar_type;
      this->endian           = frame_header->endian;
      this->width            = frame_header->width;
      this->height           = frame_header->height;
      this->m_Frame = this->m_FrameHeader;
      this->m_Frame += IGTL_VIDEO_HEADER_SIZE;
      
      return 1;
  }
  else
  {
      // Incompatible version.
      return 0;
  }
}

void VideoMessage::AllocatePack(int bodySize)
{

  if (bodySize <= 0)
    {
    bodySize = 0;
    this->m_IsBodyUnpacked = 0;
    }

  int s = IGTL_HEADER_SIZE + bodySize;

  m_IsHeaderUnpacked = 0;
  m_IsBodyUnpacked = 0;

  if (bodySize > 0)
    {
    if (this->m_FrameHeader)
      {
      delete [] this->m_FrameHeader;
      this->m_FrameHeader = NULL;
      }
    if (this->m_Frame)
      {
      delete [] this->m_Frame;
      this->m_Frame = NULL;
      }
    this->m_Body = new unsigned char [bodySize];
    this->m_MessageSize = s;
    }
}

} // namespace igtl