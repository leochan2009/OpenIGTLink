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
  Module:    $RCSfile: vtkClientSocket.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "igtlUDPClientSocket.h"

namespace igtl
{

//-----------------------------------------------------------------------------
UDPClientSocket::UDPClientSocket()
{
}

//-----------------------------------------------------------------------------
UDPClientSocket::~UDPClientSocket()
{
}

//-----------------------------------------------------------------------------
int UDPClientSocket::ReadSocket(unsigned char* buffer, unsigned bufferSize) {
   struct sockaddr_in fromAddress;
#if defined(OpenIGTLink_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
  socklen_t addressLength = sizeof(fromAddress);
#else
  int addressLength = sizeof(fromAddress);
#endif
  if (this->m_SocketDescriptor == -1)
  {
    igtlWarningMacro("Socket not created yet Closing it.");
    this->m_SocketDescriptor = this->CreateUDPSocket();
  }
  if (!this->m_SocketDescriptor)
  {
    igtlErrorMacro("Failed to create socket.");
    return -1;
  }
  int bytesRead = recvfrom(this->m_SocketDescriptor, (void*)buffer, bufferSize, 0, (struct sockaddr*)(&fromAddress), &addressLength);
  if (bytesRead < 0) {
    //##### HACK to work around bugs in Linux and Windows:
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    int errorCode = WSAGetLastError();
#else
    int errorCode = errno;
#endif
    if (errorCode == 111 /*ECONNREFUSED (Linux)*/
#if defined(__WIN32__) || defined(_WIN32)
        // What a piece of crap Windows is.  Sometimes
        // recvfrom() returns -1, but with an 'errno' of 0.
        // This appears not to be a real error; just treat
        // it as if it were a read of zero bytes, and hope
        // we don't have to do anything else to 'reset'
        // this alleged error:
        || errorCode == 0 || errorCode == EWOULDBLOCK
#else
        || errorCode == EAGAIN
#endif
        || errorCode == 113 /*EHOSTUNREACH (Linux)*/) { // Why does Linux return this for datagram sock?
      fromAddress.sin_addr.s_addr = 0;
      return 0;
    }
    //##### END HACK
  } else if (bytesRead == 0) {
    // "recvfrom()" on a stream socket can return 0 if the remote end has closed the connection.  Treat this as an error:
    return -1;
  }
  
  return bytesRead;
}
//-----------------------------------------------------------------------------
void UDPClientSocket::PrintSelf(std::ostream& os) const
{
  this->Superclass::PrintSelf(os);
}

} // end of igtl namespace

