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

#include "websocketpp/server.hpp"
#include "websocketpp/client.hpp"

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
    void run(std::string docroot, uint16_t port, int time);
    void send(void const * payload, size_t len);
    void on_http(connection_hdl hdl);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
  private:
    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
    
    server m_endpoint;
    con_list m_connections;
    server::timer_ptr m_timer;
    
    std::string m_docroot;
    
    // data count
    uint64_t m_count;
};



#endif /* igtlWebSocket_hpp */
