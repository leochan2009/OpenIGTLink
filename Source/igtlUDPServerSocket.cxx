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
  Module:    $RCSfile: vtkServerSocket.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "igtlUDPServerSocket.h"

namespace igtl
{

//-----------------------------------------------------------------------------
UDPServerSocket::UDPServerSocket()
{
}

//-----------------------------------------------------------------------------
UDPServerSocket::~UDPServerSocket()
{
}
  
int UDPServerSocket::AddClient(struct in_addr add, u_int16_t port, unsigned int clientID)
{
  for(int i = 0; this->clients.size(); i++)
  {
    if (this->clients[i].clientId == clientID &&
        this->clients[i].address.s_addr == add.s_addr
        && this->clients[i].portNum ==  port )
    {
      
      return -1;
    }
  }
  this->clients.push_back(ClientDest(add, port, clientID));
  return 0;
}


int UDPServerSocket::WriteSocket(unsigned char* buffer, unsigned bufferSize)
{
  int successul = 0;
  for(int i = 0; this->clients.size(); i++)
  {
#if defined(OpenIGTLink_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
    socklen_t addressLength = sizeof(this->clients[i].address);
#else
    int addressLength = sizeof(this->clients[i].address);
#endif
    int bytesSent = sendto(this->m_SocketDescriptor, (char*)buffer, bufferSize, 0,
                           (struct sockaddr*)&this->clients[i].address, addressLength);
    if (bytesSent != (int)bufferSize) {
      successul = -1;
      continue; //to do: how to handle the unsuccessful transmission?
    }
  }
  return successul;
}
  
int UDPServerSocket::CreateUDPServer(int port)
{
  if (this->m_SocketDescriptor != -1)
  {
    igtlWarningMacro("Server Socket already exists. Closing old socket.");
    this->CloseSocket(this->m_SocketDescriptor);
    this->m_SocketDescriptor = -1;
  }
  this->m_SocketDescriptor = this->CreateUDPSocket();
  if (this->m_SocketDescriptor < 0)
  {
    return -1;
  }
  if ( this->BindSocket(this->m_SocketDescriptor, port) != 0||
      this->Listen(this->m_SocketDescriptor) != 0)
  {
    // failed to bind or listen.
    this->CloseSocket(this->m_SocketDescriptor);
    this->m_SocketDescriptor = -1;
    return -1;
  }
  // Success.
  return 0;
}
  
int UDPServerSocket::DeleteClient(unsigned int clientID)
{
  for(int i = 0; this->clients.size(); i++)
  {
    if (this->clients[i].clientId == clientID)
    {
      this->clients.erase(this->clients.begin()+i);
      return 0;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void UDPServerSocket::PrintSelf(std::ostream& os) const
{
  this->Superclass::PrintSelf(os);
}


} // end of igtl namespace
