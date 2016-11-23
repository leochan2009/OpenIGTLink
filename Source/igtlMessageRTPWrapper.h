/*=========================================================================

  Program:   The OpenIGTLink Library
  Language:  C++
  Web page:  http://openigtlink.org/

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __igtlMessageRTPWrapper_h
#define __igtlMessageRTPWrapper_h

#include <string>

#include "igtlObject.h"
#include "igtlMacro.h"
#include "igtlMath.h"
#include "igtlMessageBase.h"

#include "igtl_header.h"
#include "igtl_util.h"

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define RTP_HEADER_LENGTH 12
#define RTP_PAYLOAD_LENGTH 200 //maximum 1500, minus 12 byte RTP and 32 byte IP address
#define MinimumPaketSpace RTP_PAYLOAD_LENGTH/3


namespace igtl
{
  
class IGTLCommon_EXPORT MessageRTPWrapper: public Object
{
public:
  typedef MessageRTPWrapper              Self;
  typedef Object               Superclass;
  typedef SmartPointer<Self>        Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  igtlTypeMacro(igtl::MessageRTPWrapper, Object)
  igtlNewMacro(igtl::MessageRTPWrapper);

public:
  enum PaketStatus
  {
    PaketReady,
    WaitingForAnotherMSG,
    WaitingForFragment
  };
  //virtual void  AllocateScalars();

  /// Gets a pointer to the scalar data.
  //virtual void* GetScalarPointer();

  /// Sets the pointer to the scalar data (for fragmented pack support).
  //virtual void  SetScalarPointer(unsigned char * p);

  /// Gets a pointer to the scalar data (for fragmented pack support).
  igtl_uint8* GetPackPointer(){return packedMsg;};

  /// Gets the number of fragments for the packed (serialized) data. Returns numberOfDataFrag
  int GetNumberODataFragments() { return numberOfDataFrag;  /* the data for transmission is too big for UDP transmission, so the data will be transmitted by multiple packets*/ };
  
  /// Gets the number of fragments to be sent for the packed (serialized) data. Returns numberOfDataFragToSent
  int GetNumberODataFragToSent() { return numberOfDataFragToSent;  /* the data for transmission is too big for UDP transmission, so the data will be transmitted by multiple packets*/ };
  
  int WrapMessage(igtl_uint8* header, int totMsgLen);
  
  void SetPayLoadType(unsigned char type){RTPPayLoadType = type;};
  
  ///Set the synchronization source identifier
  void SetSSRC(igtl_uint32 identifier);
  
protected:
  MessageRTPWrapper();
  ~MessageRTPWrapper();
  
private:
  igtl_uint8* packedMsg;
  int curFragLocation;
  unsigned int numberOfDataFrag;
  unsigned int numberOfDataFragToSent;
  unsigned char RTPPayLoadType;
  igtl_uint16 AvailabeBytesNum;
  igtl_uint16 SeqNum;
  int status;
  igtl_uint32 appSpecificFreq;
  igtl_uint32 SSRC;
  igtl_uint32 fragmentTimeIncrement;
  
};


} // namespace igtl

#endif // _igtlMessageRTPWrapper_h