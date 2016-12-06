/*=========================================================================

  Program:   The OpenIGTLink Library
  Language:  C++
  Web page:  http://openigtlink.org/

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkClientSocket.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.UDPClientSocket
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME igtlUDPClientSocket - Encapsulates a client UDPsocket.

#ifndef __igtlUDPClientSocket_h
#define __igtlUDPClientSocket_h

#include <errno.h>
#include "igtlGeneralSocket.h"
#include "igtlWin32Header.h"

namespace igtl
{

class UDPServerSocket;

  
class ReorderBuffer
{
public:
  ReorderBuffer(){firstPaketLen=0;lastPaketLen=0;filledPaketNum=0; totFragNumber = 0;receivedLastFrag=false;receivedFirstFrag=false;};
  ReorderBuffer(ReorderBuffer const &anotherBuffer){firstPaketLen=anotherBuffer.firstPaketLen;lastPaketLen=anotherBuffer.lastPaketLen;filledPaketNum=anotherBuffer.filledPaketNum;receivedLastFrag=anotherBuffer.receivedLastFrag;receivedFirstFrag=anotherBuffer.receivedFirstFrag;};
  ~ReorderBuffer(){};
  unsigned char buffer[RTP_PAYLOAD_LENGTH*(16384-2)];  // we use 14 bits for fragment number, 2^14 = 16384. maximum
  unsigned char firstFragBuffer[RTP_PAYLOAD_LENGTH];
  unsigned char lastFragBuffer[RTP_PAYLOAD_LENGTH];
  uint32_t firstPaketLen;
  uint32_t lastPaketLen;
  uint32_t filledPaketNum;
  uint32_t totFragNumber;
  bool receivedLastFrag;
  bool receivedFirstFrag;
};
  
  class UnWrappedMessage
  {
  public:
    UnWrappedMessage(){};
    UnWrappedMessage(UnWrappedMessage const &anotherMessage){};
    ~UnWrappedMessage(){};
    unsigned char messagePackPointer[RTP_PAYLOAD_LENGTH*(16384-2)];  // we use 14 bits for fragment number, 2^14 = 16384. maximum
    uint32_t messageDataLength;
  };
  
class IGTLCommon_EXPORT UDPClientSocket : public GeneralSocket
{
public:
  typedef UDPClientSocket              Self;
  typedef GeneralSocket                Superclass;
  typedef SmartPointer<Self>        Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  igtlTypeMacro(igtl::UDPClientSocket, igtl::GeneralSocket)
  igtlNewMacro(igtl::UDPClientSocket);
  
  int JoinNetwork(const char* groupIPAddr, int portNum, bool joinGroup);

  int ReadSocket(unsigned char* buffer, unsigned bufferSize);
  
protected:
  UDPClientSocket();
  ~UDPClientSocket();

  void PrintSelf(std::ostream& os) const;

  friend class UDPServerSocket;

private:
  UDPClientSocket(const UDPClientSocket&); // Not implemented.
  void operator=(const UDPClientSocket&); // Not implemented.
};

}

#endif

