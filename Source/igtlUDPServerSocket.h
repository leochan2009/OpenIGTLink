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
  Module:    $RCSfile: vtkServerSocket.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME igtlUDPServerSocket - Encapsulate a socket that accepts connections.
// .SECTION Description
//

#ifndef __igtlUDPServerSocket_h
#define __igtlUDPServerSocket_h

//#include <vector>

#include "igtlGeneralSocket.h"
#include "igtlUDPClientSocket.h"
#include "igtlWin32Header.h"
#include "igtl_header.h"


namespace igtl
{
class ClientDest
{
public:
  ClientDest(const char* add, igtl_uint16 port, unsigned int clientID)
  {
    this->address = new unsigned char[INET_ADDRSTRLEN];
    strcpy((char*)this->address, add);
    this->portNum = port;
    this->clientId = clientID;
  };
  unsigned char* address;
  u_int16_t portNum; // should be in network byte order
  unsigned int clientId;
};
  
class IGTLCommon_EXPORT UDPServerSocket : public GeneralSocket
{
public:
  typedef UDPServerSocket              Self;
  typedef GeneralSocket  Superclass;
  typedef SmartPointer<Self>        Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  igtlTypeMacro(igtl::UDPServerSocket, igtl::GeneralSocket);
  igtlNewMacro(igtl::UDPServerSocket);

  // Description:
  // Add a client socket with given address at a given port and binds to it.
  // Returns -1 on error. return clientID on success.
  int AddClient(const char* add, igtl_uint16 port, unsigned int clientID);
  
  // Description:
  // Add a client socket with given address at a given port and binds to it.
  // Returns -1 on error. 0 on success.
  int DeleteClient(unsigned int clientID);
  
  // Description:
  // Creates a UDP server socket at a given port and binds to it.
  // Returns -1 on error. 0 on success.
  int CreateUDPServer(int port);
  
  // Desciption:
  // Write the data to all clients
  int WriteSocket(unsigned char* buffer, unsigned bufferSize);

protected:
  UDPServerSocket();
  ~UDPServerSocket();

  std::vector<ClientDest> clients;

  void PrintSelf(std::ostream& os) const;

private:
  UDPServerSocket(const UDPServerSocket&); // Not implemented.
  void operator=(const UDPServerSocket&); // Not implemented.
};

} // end of igtl namespace 


#endif

