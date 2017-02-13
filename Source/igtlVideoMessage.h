/*=========================================================================

  Program:   The OpenIGTLink Library
  Language:  C++
  Web page:  http://openigtlink.org/

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __igtlVideoMessage_h
#define __igtlVideoMessage_h

#include <string>

#include "igtlObject.h"
#include "igtlMacro.h"
#include "igtlMath.h"
#include "igtlMessageBase.h"

#include "igtl_header.h"
#include "igtl_video.h"

namespace igtl
{  
  /// A class for the STT_TDATA message type.
class IGTLCommon_EXPORT StartVideoDataMessage: public MessageBase
{
  
public:
  typedef StartVideoDataMessage        Self;
  typedef MessageBase                     Superclass;
  typedef SmartPointer<Self>              Pointer;
  typedef SmartPointer<const Self>        ConstPointer;
  
  igtlTypeMacro(igtl::StartVideoDataMessage, igtl::MessageBase);
  igtlNewMacro(igtl::StartVideoDataMessage);
  
public:
  /// Sets the time resolution for streaming of QTDATA messages
  void         SetTimeInterval(igtlInt32 interval)  { this->m_TimeInterval = interval; }; // ms
  
  /// Gets the time resolution for streaming of QTDATA messages
  igtlInt32    GetTimeInterval()               { return this->m_TimeInterval; };
  
  void    SetUseCompress(bool useCompress) { this->m_UseCompress = useCompress; };
  
  igtlInt32    GetUseCompress()               { return this->m_UseCompress; };
  
  void    SetCodecType(const char codecType[]){ this->m_CodecType = std::string(codecType); };
                                                        
  std::string    GetCodecType()               { return this->m_CodecType; };
  
protected:
  StartVideoDataMessage() : MessageBase()
  {
    this->m_SendMessageType  = "STT_VIDEO";
    this->m_CodecType = "H264";
  };
  ~StartVideoDataMessage();
  
  /// Gets the size of the serialized content.
  virtual int  CalculateContentBufferSize();
  
protected:
  virtual int  GetBodyPackSize();
  virtual int  PackContent();
  virtual int  UnpackContent();
  
protected:
  
  /// codec is define here, "H264" is the default
  /// A variable for codec protocal
  std::string   m_CodecType;
  /// Minimum time between two frames (ms). Use 0 for as fast as possible.
  igtlInt32     m_TimeInterval;
  bool m_UseCompress;
  
};
  
  
class IGTLCommon_EXPORT VideoMessageHeader: public MessageHeader
{
public:
  typedef VideoMessageHeader                Self;
  typedef MessageHeader                    Superclass;
  typedef SmartPointer<Self>             Pointer;
  typedef SmartPointer<const Self>       ConstPointer;
  
  igtlTypeMacro(igtl::VideoMessageHeader, igtl::MessageBase);
  igtlNewMacro(igtl::VideoMessageHeader);
  virtual int  GetBodyPackSize() { return m_BodySizeToRead; };

protected:
  VideoMessageHeader() : MessageBase() { this->m_SendMessageType  = "VIDEO_HEADER"; };
  ~VideoMessageHeader() {};
protected:
  virtual int  PackBody()        { AllocatePack(); return 1; };
  virtual int  UnpackBody()  { return 1; };
};
  
  
class IGTLCommon_EXPORT StopVideoMessage: public MessageBase
{
public:
  typedef StopVideoMessage                Self;
  typedef MessageBase                    Superclass;
  typedef SmartPointer<Self>             Pointer;
  typedef SmartPointer<const Self>       ConstPointer;
  
  igtlTypeMacro(igtl::StopVideoMessage, igtl::MessageBase);
  igtlNewMacro(igtl::StopVideoMessage);
  
protected:
  StopVideoMessage() : MessageBase() { this->m_SendMessageType  = "STP_VIDEO"; };
  ~StopVideoMessage() {};
protected:
  virtual int  GetBodyPackSize() { return 0; };
  virtual int  PackBody()        { AllocatePack(); return 1; };
  virtual int  UnpackBody()      { return 1; };
};


class IGTLCommon_EXPORT VideoMessage: public MessageBase
{
public:
  typedef VideoMessage              Self;
  typedef MessageBase               Superclass;
  typedef SmartPointer<Self>        Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  igtlTypeMacro(igtl::VideoMessage, igtl::MessageBase)
  igtlNewMacro(igtl::VideoMessage);

public:

  /// Endian used in the bite array for the image data.
  enum {
    ENDIAN_BIG=1,
    ENDIAN_LITTLE=2
  };

  /// Pixel data type either scalar or vector. 
  enum {
    DTYPE_SCALAR = 1,
    DTYPE_VECTOR = 3
  };

  /// Pixel data type.
  enum {
    TYPE_INT8    = 2,
    TYPE_UINT8   = 3,
    TYPE_INT16   = 4,
    TYPE_UINT16  = 5,
    TYPE_INT32   = 6,
    TYPE_UINT32  = 7,
  };


public:

  /// Sets the image scalar type.
  void SetScalarType(int t)    { scalarType = t; };

  /// Sets the image scalar type to 8-bit integer.
  void SetScalarTypeToInt8()   { scalarType = TYPE_INT8; };

  /// Sets the image scalar type to unsigned 8-bit integer.
  void SetScalarTypeToUint8()  { scalarType = TYPE_UINT8; };

  /// Sets the image scalar type to 16-bit integer.
  void SetScalarTypeToInt16()  { scalarType = TYPE_INT16; };

  /// Sets the image scalar type to unsigned 16-bit integer.
  void SetScalarTypeToUint16() { scalarType = TYPE_UINT16; };

  /// Sets the image scalar type to 32-bit integer.
  void SetScalarTypeToInt32()  { scalarType = TYPE_INT32; };

  /// Sets the image scalar type to unsigned 32-bit integer.
  void SetScalarTypeToUint32() { scalarType = TYPE_UINT32; };

  /// Gets the image scalar type.
  int  GetScalarType()         { return scalarType; };

  /// Gets the size of the scalar type used in the current image data.
  /// (e.g. 1 byte for 8-bit integer)
  int  GetScalarSize()         { return ScalarSizeTable[scalarType]; };

  /// Gets the size of the specified scalar type. (e.g. 1 byte for 8-bit integer)
  int  GetScalarSize(int type) { return ScalarSizeTable[type]; };

  /// Sets the Endianess of the image scalars. (default is ENDIAN_BIG)
  void SetEndian(int e)        { endian = e; };

  /// Gets the Endianess of the image scalars.
  int  GetEndian()             { return endian; };
  
  /// Sets the width of the image scalars.
  void SetWidth(int w)        { width = w; };
  
  /// Gets the width of the image scalars.
  int  GetWidth()             { return width; };
  
  /// Sets the width of the image scalars.
  void SetHeight(int h)        { height = h; };
  
  /// Gets the height of the image scalars.
  int  GetHeight()             { return height; };

  /// This should only be called when the data is unpacked
  int  GetBitStreamSize()
  {
    if (this->m_IsHeaderUnpacked)
    {
#if OpenIGTLink_HEADER_VERSION >= 2
      if (m_HeaderVersion == IGTL_HEADER_VERSION_2)
      {
       return GetPackBodySize()-IGTL_VIDEO_HEADER_SIZE - sizeof(igtl_extended_header) - GetMetaDataHeaderSize() - GetMetaDataSize();
      }
      else
      {
        return GetPackBodySize()-IGTL_VIDEO_HEADER_SIZE;
      }
#else
      return GetPackBodySize()-IGTL_VIDEO_HEADER_SIZE;
#endif
    }
    else
    {
      return -1;
    }
  };
  
  void SetBitStreamSize(int size)
  {
    bitStreamSize = size;
#if OpenIGTLink_HEADER_VERSION >= 2
    int message_size(-1);
    if (m_HeaderVersion == IGTL_HEADER_VERSION_2)
    {
      m_MessageSize = IGTL_HEADER_SIZE + bitStreamSize + sizeof(igtl_extended_header) + GetMetaDataHeaderSize() + GetMetaDataSize() + IGTL_VIDEO_HEADER_SIZE;
    }
    else
    {
      m_MessageSize = size + IGTL_VIDEO_HEADER_SIZE + IGTL_HEADER_SIZE;

    }
#else
    m_MessageSize = size + IGTL_HEADER_SIZE;

#endif
    
  };

  /// Allocates a memory area for the scalar data based on the dimensions of the subvolume,
  /// the number of components, and the scalar type.
  /// Note: If FragmentedPack is active, GetScalarPointer() causes extra memory allocation to
  /// create a single pack memroy degrading the performance.
  virtual void  AllocateScalars();

  virtual int CalculateContentBufferSize();
  
  virtual void AllocateBuffer();
  
  /// Gets a pointer to the scalar data.
  virtual void* GetScalarPointer();

  /// Sets the pointer to the scalar data (for fragmented pack support).
  virtual void  SetScalarPointer(unsigned char * p);

  /// Gets a pointer to the scalar data (for fragmented pack support).
  void* GetPackPointer(){return GetBufferPointer();};

  /// Gets the number of fragments for the packed (serialized) data. Returns 3
  /// consisting of header, image header and image body. (for fragmented pack support)
  int   GetNumberOfPackFragments() { return 3;  /* header, image header and image body */ }

  /// Gets a pointer to the specified fragment of the packed data. (for fragmented pack support)
  unsigned char* GetPackFragmentPointer(int id);
  
  int GetPackFragmentSize(int id);

protected:
  VideoMessage();
  ~VideoMessage();
  
protected:

  virtual int  GetBodyPackSize();
  
  /// Pack() serializes the header and body based on the member variables.
  /// PackBody() must be implemented in the child class. (for fragmented pack support)
  virtual int  PackContent();
  virtual int  UnpackContent();

public:  
  
  /// Allocate memory specifying the body size
  /// (used when create a brank package to receive data) (for fragmented pack support)
  virtual void AllocatePack(int bodySize);

  /// A variable for the Endian of the scalar values in the image.
  int    endian;

  /// A variable for the scalar type of the voxels.
  int    scalarType;
  
  /// The Width of the picture
  int    width;
  
  /// The Height of the picture
  int    height;
  
  /// A variable for the allocate the message body
  int    bitStreamSize;
  
  /// A pointer to the serialized image header.
  unsigned char*  m_FrameHeader;

  /// A pointer to the serialized image data.
  unsigned char*  m_Frame;

  /// A table to look up the size of a given scalar type.
  int ScalarSizeTable[12];
};


} // namespace igtl

#endif // _igtlVideoMessage_h