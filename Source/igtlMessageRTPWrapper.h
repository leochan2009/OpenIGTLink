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
#include "igtlMessageFactory.h"
#include "igtlUDPServerSocket.h"
#include "igtlUDPClientSocket.h"
#include "igtlMutexLock.h"
#include "igtl_header.h"
#include "igtl_util.h"

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

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
    WaitingForFragment,
    MessageReady,
    ToUnpackAnotherMSG,
    WaitingForAnotherPaket
  };
  //virtual void  AllocateScalars();

  /// Gets a pointer to the scalar data.
  //virtual void* GetScalarPointer();

  /// Sets the pointer to the scalar data (for fragmented pack support).
  //virtual void  SetScalarPointer(unsigned char * p);

  /// Gets the number of fragments for the packed (serialized) data. Returns numberOfDataFrag
  int GetNumberODataFragments() { return numberOfDataFrag;  /* the data for transmission is too big for UDP transmission, so the data will be transmitted by multiple packets*/ };
  
  /// Gets the number of fragments to be sent for the packed (serialized) data. Returns numberOfDataFragToSent
  int GetNumberODataFragToSent() { return numberOfDataFragToSent;  /* the data for transmission is too big for UDP transmission, so the data will be transmitted by multiple packets*/ };
  
  int WrapMessageAndSend(igtl::UDPServerSocket::Pointer &socket, igtl_uint8* messagePackPointer, int msgtotalLen);
  
  int UnWrapMessageWithTypeAndName(igtlUint8 * UDPPaket, igtlUint16 totMsgLen, igtl::MessageBase::Pointer & MSGPointer, const char *deviceType, const char * deviceName);
  
  igtl::MessageBase::Pointer UnWrapMessage(igtl_uint8* messageContent, int totMsgLen);
  
  void SetPayLoadType(unsigned char type){RTPPayLoadType = type;};
  
  ///Set the synchronization source identifier, different session has different SSRC
  void SetSSRC(igtl_uint32 identifier);
  
  ///Set the Contributing source identifier. different device has different CSRC
  void SetCSRC(igtl_uint32 identifier);
  
  ///Set the sequencen number at the rtp header
  void SetSeqNum(igtl_uint16 num);
  
  ///Set the current msg header
  void SetMSGHeader(igtl_uint8* header);
  
  int GetCurMSGLocation(){return this->curMSGLocation;};
  
  int GetPackedMSGLocation(){return this->curPackedMSGLocation;};
  
  int GetRTPWrapperStatus(){return status;};
  
protected:
  MessageRTPWrapper();
  ~MessageRTPWrapper();
  
  /// Gets a pointer to the scalar data (for fragmented pack support).
  igtl_uint8* GetPackPointer(){return packedMsg;};
  
  int WrapMessage(igtl_uint8* messageContent, int bodyMsgLen);

  
private:
  igtl_uint16 extendedHeaderSize;
  igtl_uint8* packedMsg;
  igtl_uint8* MSGHeader;
  unsigned int curMSGLocation;
  unsigned int curPackedMSGLocation;
  igtl_uint8 framentNumber;
  unsigned int numberOfDataFrag;
  unsigned int numberOfDataFragToSent;
  unsigned char RTPPayLoadType;
  igtl_uint16 AvailabeBytesNum;
  igtl_uint16 SeqNum;
  int status;
  igtl_uint32 appSpecificFreq;
  igtl_uint32 SSRC;
  igtl_uint32 CSRC;
  igtl_uint32 fragmentTimeIncrement;
  igtl::MutexLock::Pointer glock;
  igtl::ReorderBuffer* reorderBuffer;
  
};


} // namespace igtl

#endif // _igtlMessageRTPWrapper_h