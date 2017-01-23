//
//  igtlWebSocket.hpp
//  OpenIGTLink
//
//  Created by Longquan Chen on 1/20/17.
//
//

#ifndef igtlWebSocket_h
#define igtlWebSocket_h


#include "websocketpp/config/asio_no_tls.hpp"
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "websocketpp/server.hpp"
#include "websocketpp/client.hpp"

#include "igtlOSUtil.h"
#include "igtlMutexLock.h"
#include "igtlMultiThreader.h"
#include "igtlConditionVariable.h"

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

/**
 */

class webSocketServer{
  public:
    typedef websocketpp::connection_hdl connection_hdl;
    typedef websocketpp::server<websocketpp::config::asio> server;
    webSocketServer();
    int CreateServer(uint16_t port);
    int CreateHTTPServer(std::string docroot, uint16_t port);
  
    void Send(void const * inputMessage, size_t len);
    void on_http(connection_hdl hdl);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    webSocketServer* WaitForConnection(unsigned long msec);
    void SetTimeInterval(unsigned int time);
    void on_timer(websocketpp::lib::error_code const & ec);
    void set_timer();
    server m_endpoint;
    std::string m_docroot;
    bool serverCreated;
    igtl::SimpleMutexLock* glock;
    igtl::ConditionVariable::Pointer conditionVar;
  private:
    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
    int run(std::string docroot, uint16_t port);
  
    con_list m_connections;
    server::timer_ptr m_timer;
  
  
  
    unsigned int m_timeInterval;
    // data count
    uint64_t m_count;
    std::string payload;
    igtl::MultiThreader::Pointer threader; 
};

class webSocketClient {
public:
  typedef websocketpp::client<websocketpp::config::asio_client> client;
  typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
  
  webSocketClient() ;
  // This method will block until the connection is complete
  void ConnectToServer(const std::string & uri);
  
  // The open handler will signal that we are ready to start sending data
  void on_open(websocketpp::connection_hdl);
  
  
  // The close handler will signal that we should stop sending data
  void on_close(websocketpp::connection_hdl) ;
  
  // The fail handler will signal that we should stop sending data
  void on_fail(websocketpp::connection_hdl) ;
  
  int ConnectToServer(const char* hostName, int port);
  
  int Send(const void* data, int length);
  
private:
  client m_client;
  websocketpp::connection_hdl m_hdl;
  websocketpp::lib::mutex m_lock;
  bool m_open;
  bool m_done;
};



#endif /* igtlWebSocket_hpp */
