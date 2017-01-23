//
//  igtlWebSocket.cpp
//  OpenIGTLink
//
//  Created by Longquan Chen on 1/20/17.
//
//

#include "igtlWebSocket.h"


typedef struct {
  int   status;
  uint16_t port;
  webSocketServer * server;
} ThreadData;


  void* ThreadFunction(void* ptr)
  {
    //------------------------------------------------------------
    // Get thread information
    igtl::MultiThreader::ThreadInfo* info =
    static_cast<igtl::MultiThreader::ThreadInfo*>(ptr);
    
    ThreadData* td = static_cast<ThreadData*>(info->UserData);
    std::stringstream ss;
    ss << "Running server on port "<< td->port <<" using docroot=" << td->server->m_docroot;
    td->server->m_endpoint.get_alog().write(websocketpp::log::alevel::app,ss.str());
    td->server->glock->Lock();
    
    // listen on specified port
    td->server->m_endpoint.listen(td->port);
    
    // Start the server accept loop
    td->server->m_endpoint.start_accept();
    td->server->glock->Unlock();
    td->server->serverCreated  = true;
    td->server->conditionVar->Signal();
    // Start the ASIO io_service run loop
    try {
      td->server->m_endpoint.run();
    } catch (websocketpp::exception const & e) {
      std::cout << e.what() << std::endl;
      td->status = -1;
    }
    td->status = 0;
    return NULL;
  }


  webSocketServer::webSocketServer() : m_count(0) {
    // set up access channels to only log interesting things
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
    m_endpoint.set_access_channels(websocketpp::log::alevel::app);
    
    // Initialize the Asio transport policy
    m_endpoint.init_asio();
    
    // Bind the handlers we are using
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::bind;
    m_endpoint.set_open_handler(bind(&webSocketServer::on_open,this,_1));
    m_endpoint.set_close_handler(bind(&webSocketServer::on_close,this,_1));
    m_endpoint.set_http_handler(bind(&webSocketServer::on_http,this,_1));
    m_timeInterval = 1;
    glock = igtl::SimpleMutexLock::New();
    conditionVar = igtl::ConditionVariable::New();
    threader = igtl::MultiThreader::New();
  }

  void webSocketServer::SetTimeInterval(unsigned int time)
  {
    this->m_timeInterval = time;
  }

  int webSocketServer::CreateServer(uint16_t port)
  {
    return this->run("",port);
  }

  int webSocketServer::CreateHTTPServer(std::string docroot, uint16_t port)
  {
    return this->run(docroot,port);
  }

  webSocketServer* webSocketServer::WaitForConnection(unsigned long msec )
  {
    if(m_connections.size())
    {
      return this;
    }
    else
    {
      igtl::Sleep(msec);
      return NULL;
    }
  }

  int webSocketServer::run(std::string docroot, uint16_t port) {
    set_timer();
    m_docroot = docroot;
    serverCreated = false;
    ThreadData td;
    td.status   = -2;
    td.port     = port;
    td.server   = this;
    threader->SpawnThread((igtl::ThreadFunctionType) &ThreadFunction, &td);
    this->glock->Lock();
    while(!this->serverCreated)
    {
      this->conditionVar->Wait(this->glock);
    }
    this->glock->Unlock();
    return 1;
  }

  void webSocketServer::Send(void const * inputMessage, size_t len)
  {
    glock->Lock();
    memcpy((void*)this->payload.c_str(), inputMessage, len);
    glock->Unlock();
    m_count++;
  }

  void webSocketServer::set_timer() {
      m_timer = m_endpoint.set_timer(this->m_timeInterval,
                                     websocketpp::lib::bind( &webSocketServer::on_timer, this,websocketpp::lib::placeholders::_1)
                                     );
    }

  void webSocketServer::on_timer(websocketpp::lib::error_code const & ec) {
    if (ec) {
      // there was an error, stop telemetry
      m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                  "Timer Error: "+ec.message());
      return;
    }
    // Broadcast count to all connections
    con_list::iterator it;
    glock->Lock();
    for (it = m_connections.begin(); it != m_connections.end(); ++it) {
      if (this->payload.length())
      {
        m_endpoint.send(*it,this->payload.c_str(),websocketpp::frame::opcode::text);
      }
    }
    this->payload.clear();
    glock->Unlock();
    // set timer for next telemetry check
    set_timer();
  }

  void webSocketServer::on_http(connection_hdl hdl) {
    // Upgrade our connection handle to a full connection_ptr
    server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
    
    std::ifstream file;
    std::string filename = con->get_resource();
    std::string response;
    
    //m_endpoint.get_alog().write(websocketpp::log::alevel::app, "http request1: "+filename);
    
    if (filename == "/") {
      filename = m_docroot+"index.html";
    } else {
      filename = m_docroot+filename.substr(1);
    }
    
    //m_endpoint.get_alog().write(websocketpp::log::alevel::app, "http request2: "+filename);
    
    file.open(filename.c_str(), std::ios::in);
    if (!file) {
      // 404 error
      std::stringstream ss;
      
      ss << "<!doctype html><html><head>"
      << "<title>Error 404 (Resource not found)</title><body>"
      << "<h1>Error 404</h1>"
      << "<p>The requested URL " << filename << " was not found on this server.</p>"
      << "</body></head></html>";
      
      con->set_body(ss.str());
      con->set_status(websocketpp::http::status_code::not_found);
      return;
    }
    
    file.seekg(0, std::ios::end);
    response.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    
    response.assign((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    
    con->set_body(response);
    con->set_status(websocketpp::http::status_code::ok);
  }
  
  void webSocketServer::on_open(connection_hdl hdl) {
    m_connections.insert(hdl);
  }
  
  void webSocketServer::on_close(connection_hdl hdl) {
    m_connections.erase(hdl);
  }


  webSocketClient::webSocketClient() : m_open(false),m_done(false) {
    // set up access channels to only log interesting things
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.set_access_channels(websocketpp::log::alevel::connect);
    m_client.set_access_channels(websocketpp::log::alevel::disconnect);
    m_client.set_access_channels(websocketpp::log::alevel::app);
    
    // Initialize the Asio transport policy
    m_client.init_asio();
    
    // Bind the handlers we are using
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::bind;
    m_client.set_open_handler(bind(&webSocketClient::on_open,this,_1));
    m_client.set_close_handler(bind(&webSocketClient::on_close,this,_1));
    m_client.set_fail_handler(bind(&webSocketClient::on_fail,this,_1));
  }

  int webSocketClient::ConnectToServer(const char* hostName, int port)
  {
    websocketpp::lib::error_code ec;
    std::string uri;
    client::connection_ptr con = m_client.get_connection(uri, ec);
    if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Get Connection Error: "+ec.message());
      return -1;
    }
    
    // Grab a handle for this connection so we can talk to it in a thread
    // safe manor after the event loop starts.
    m_hdl = con->get_handle();
    
    // Queue the connection. No DNS queries or network connections will be
    // made until the io_service event loop is run.
    m_client.connect(con);
    
    // Create a thread to run the ASIO io_service event loop
    websocketpp::lib::thread asio_thread(&client::run, &m_client);
    asio_thread.join();
    return 0;
  }
  
  // The open handler will signal that we are ready to start sending telemetry
  void webSocketClient::on_open(websocketpp::connection_hdl) {
    m_client.get_alog().write(websocketpp::log::alevel::app,
                              "Connection opened, starting telemetry!");
    
    scoped_lock guard(m_lock);
    m_open = true;
  }
  
  // The close handler will signal that we should stop sending telemetry
  void webSocketClient::on_close(websocketpp::connection_hdl) {
    m_client.get_alog().write(websocketpp::log::alevel::app,
                              "Connection closed, stopping telemetry!");
    
    scoped_lock guard(m_lock);
    m_done = true;
  }
  
  // The fail handler will signal that we should stop sending telemetry
  void webSocketClient::on_fail(websocketpp::connection_hdl) {
    m_client.get_alog().write(websocketpp::log::alevel::app,
                              "Connection failed, stopping telemetry!");
    
    scoped_lock guard(m_lock);
    m_done = true;
  }

  int webSocketClient::Send(const void* data, int length)
  {
    std::stringstream val;
    websocketpp::lib::error_code ec;
    m_client.send(m_hdl,val.str(),websocketpp::frame::opcode::text,ec);
    
    // The most likely error that we will get is that the connection is
    // not in the right state. Usually this means we tried to send a
    // message to a connection that was closed or in the process of
    // closing. While many errors here can be easily recovered from,
    // in this simple example, we'll stop the telemetry loop.
    if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Send Error: "+ec.message());
      return 0;
    }
    return 1;
  }