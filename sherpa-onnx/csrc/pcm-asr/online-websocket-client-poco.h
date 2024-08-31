#ifndef __ONLINE_WEBSOCKET_CLIENT_POCO_H__
#define __ONLINE_WEBSOCKET_CLIENT_POCO_H__

#include <chrono>  // NOLINT
#include <fstream>
#include <string>
#include <map>

#include "Poco/Mutex.h"
#include "sherpa-onnx/csrc/macros.h"
#include "sherpa-onnx/csrc/parse-options.h"
#include "sherpa-onnx/csrc/wave-reader.h"
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_no_tls_client.hpp"
#include "websocketpp/uri.hpp"
#include "TcpClient.h"

using client = websocketpp::client<websocketpp::config::asio_client>;

using message_ptr = client::message_ptr;
using websocketpp::connection_hdl;

class Client {
 public:
  Client(boost::asio::io_context &io,  // NOLINT
         const std::string &ip, int16_t port, const std::vector<float> &samples,
         int32_t samples_per_message, float seconds_per_message)
      : io_(io),
        uri_(/*secure*/ false, ip, port, /*resource*/ "/"),
        samples_(samples),
        samples_per_message_(samples_per_message),
        seconds_per_message_(seconds_per_message) {
    c_.clear_access_channels(websocketpp::log::alevel::all);
    // c_.set_access_channels(websocketpp::log::alevel::connect);
    // c_.set_access_channels(websocketpp::log::alevel::disconnect);

    c_.init_asio(&io_);
    c_.set_open_handler([this](connection_hdl hdl) { OnOpen(hdl); });
    c_.set_close_handler(
        [](connection_hdl /*hdl*/) { SHERPA_ONNX_LOGE("Disconnected"); });
    c_.set_message_handler(
        [this](connection_hdl hdl, message_ptr msg) { OnMessage(hdl, msg); });

    Run();
  }

 private:
  void Run() {
    websocketpp::lib::error_code ec;
    client::connection_ptr con = c_.get_connection(uri_.str(), ec);
    if (ec) {
      SHERPA_ONNX_LOGE("Could not create connection to %s because %s",
                       uri_.str().c_str(), ec.message().c_str());
      exit(EXIT_FAILURE);
    }

    c_.connect(con);
  }

  void OnOpen(connection_hdl hdl) {
    auto start_time = std::chrono::steady_clock::now();
    boost::asio::post(
        io_, [this, hdl, start_time]() { this->SendMessage(hdl, start_time); });
  }

  void OnMessage(connection_hdl hdl, message_ptr msg) {
    const std::string &payload = msg->get_payload();

    if (payload == "Done!") {
      websocketpp::lib::error_code ec;
      c_.close(hdl, websocketpp::close::status::normal, "I'm exiting now", ec);
      if (ec) {
        SHERPA_ONNX_LOGE("Failed to close because %s", ec.message().c_str());
        exit(EXIT_FAILURE);
      }
    } else {
      SHERPA_ONNX_LOGE("%s", payload.c_str());
    }
  }

  void SendMessage(
      connection_hdl hdl,
      std::chrono::time_point<std::chrono::steady_clock> start_time) {
    int32_t num_samples = samples_.size();
    int32_t num_messages = num_samples / samples_per_message_;

    websocketpp::lib::error_code ec;
    auto time = std::chrono::steady_clock::now();
    int elapsed_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(time - start_time)
            .count();

    if (elapsed_time_ms <
        static_cast<int>(seconds_per_message_ * num_sent_messages_ * 1000)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(int(
          seconds_per_message_ * num_sent_messages_ * 1000 - elapsed_time_ms)));
    }

    if (num_sent_messages_ < 1) {
      SHERPA_ONNX_LOGE("Starting to send audio");
    }

    if (num_sent_messages_ < num_messages) {
      c_.send(hdl, samples_.data() + num_sent_messages_ * samples_per_message_,
              samples_per_message_ * sizeof(float),
              websocketpp::frame::opcode::binary, ec);

      if (ec) {
        SHERPA_ONNX_LOGE("Failed to send audio samples because %s",
                         ec.message().c_str());
        exit(EXIT_FAILURE);
      }

      ec.clear();

      ++num_sent_messages_;
    }

    if (num_sent_messages_ == num_messages) {
      int32_t remaining_samples = num_samples % samples_per_message_;
      if (remaining_samples) {
        c_.send(hdl,
                samples_.data() + num_sent_messages_ * samples_per_message_,
                remaining_samples * sizeof(float),
                websocketpp::frame::opcode::binary, ec);

        if (ec) {
          SHERPA_ONNX_LOGE("Failed to send audio samples because %s",
                           ec.message().c_str());
          exit(EXIT_FAILURE);
        }
        ec.clear();
      }

      // To signal that we have send all the messages
      c_.send(hdl, "Done", websocketpp::frame::opcode::text, ec);
      SHERPA_ONNX_LOGE("Sent Done Signal");

      if (ec) {
        SHERPA_ONNX_LOGE("Failed to send audio samples because %s",
                         ec.message().c_str());
        exit(EXIT_FAILURE);
      }
    } else {
      boost::asio::post(io_, [this, hdl, start_time]() {
        this->SendMessage(hdl, start_time);
      });
    }
  }

 private:
  client c_;
  boost::asio::io_context &io_;
  websocketpp::uri uri_;
  std::vector<float> samples_;
  int32_t samples_per_message_ = 8000;  // 0.5 seconds
  float seconds_per_message_ = 0.2;
  int32_t num_sent_messages_ = 0;
};



//typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
//typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
class connection_metadata {
 public:
  typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

  connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id),
        m_hdl(hdl),
        m_status("Connecting"),
        m_uri(uri),
        m_server("N/A"),
        m_bOpen(false) {}

  void on_open(client *c, websocketpp::connection_hdl hdl) {
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    m_server = con->get_response_header("Server");

    m_status = "Open";
    m_bOpen = true;
    printf("%s out\n", __PRETTY_FUNCTION__);
  }

  void on_fail(client *c, websocketpp::connection_hdl hdl) {
    m_bOpen = false;
    m_status = "Failed";

    client::connection_ptr con = c->get_con_from_hdl(hdl);
    m_server = con->get_response_header("Server");
    m_error_reason = con->get_ec().message();
    printf("%s failed rason [%s]\n", __PRETTY_FUNCTION__,
                   m_error_reason.c_str());
    printf("uri is [%s]\n", m_uri.c_str());
  }

  void on_close(client *c, websocketpp::connection_hdl hdl) {
    m_bOpen = false;
    m_status = "Closed";

    client::connection_ptr con = c->get_con_from_hdl(hdl);
    std::stringstream s;
    s << "close code: " << con->get_remote_close_code() << " ("
      << websocketpp::close::status::get_string(con->get_remote_close_code())
      << "), close reason: " << con->get_remote_close_reason();
    m_error_reason = s.str();
    printf("> on_close, m_error_reason [%s]\n", m_error_reason.c_str());
  }

  void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
    if (msg->get_payload() != "ok") {
      //printf("asr %#x message %s\n", this, msg->get_payload().c_str());
      std::lock_guard<std::mutex> lock(mutex_);
      m_messages.push_back(msg->get_payload());
    }
    return;
  }

  websocketpp::connection_hdl get_hdl() const { return m_hdl; }

  int get_id() const { return m_id; }

  std::string get_status() const { return m_status; }

  std::string Uri() { return m_uri; }
  std::string Status() { return m_status; }
  std::string Server() { return m_server; }
  std::string ErrorReason() { return m_error_reason; }
  bool IsOpen() { return m_bOpen; }
  std::vector<std::string> GetMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> messages;
    messages.swap(m_messages);
    return messages;
  }

 private:
  int m_id;
  websocketpp::connection_hdl m_hdl;
  std::string m_status;
  std::string m_uri;
  std::string m_server;
  std::string m_error_reason;
  bool m_bOpen;

  std::mutex mutex_;
  std::vector<std::string> m_messages;
};

//std::ostream &operator<<(std::ostream &out, connection_metadata const &data);

class websocket_endpoint {
 public:
  websocket_endpoint() : m_next_id(0) {
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

    m_endpoint.init_asio();
    m_endpoint.start_perpetual();

    m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(
        &client::run, &m_endpoint);
  }

  ~websocket_endpoint() {
    m_endpoint.stop_perpetual();

    for (con_list::const_iterator it = m_connection_list.begin();
         it != m_connection_list.end(); ++it) {
      if (it->second->get_status() != "Open") {
        // Only close open connections
        continue;
      }

      std::cout << "> Closing connection " << it->second->get_id() << std::endl;

      websocketpp::lib::error_code ec;
      m_endpoint.close(it->second->get_hdl(),
                       websocketpp::close::status::going_away, "", ec);
      if (ec) {
        std::cout << "> Error closing connection " << it->second->get_id()
                  << ": " << ec.message() << std::endl;
      }
    }

    m_thread->join();
  }

  int connect(std::string const& server_ip, int32_t server_port) {
    websocketpp::uri uri_(false, server_ip, server_port, "/");
    return connect(uri_.str());
  }

  int connect(std::string const &uri) {
    websocketpp::lib::error_code ec;

    /*m_endpoint.set_tls_init_handler(
        bind(&websocket_endpoint::on_tls_init, this,
                                         uri.c_str(),
                                         websocketpp::lib::placeholders::_1));*/
    client::connection_ptr con = m_endpoint.get_connection(uri, ec);

    if (ec) {
      printf("> Connect initialization error: %s\n",
                     ec.message().c_str());
      return -1;
    }

    int new_id = m_next_id++;
    connection_metadata::ptr metadata_ptr =
        websocketpp::lib::make_shared<connection_metadata>(
            new_id, con->get_handle(), uri);
    m_connection_list[new_id] = metadata_ptr;

    con->set_open_handler(websocketpp::lib::bind(
        &connection_metadata::on_open, metadata_ptr, &m_endpoint,
        websocketpp::lib::placeholders::_1));
    con->set_fail_handler(websocketpp::lib::bind(
        &connection_metadata::on_fail, metadata_ptr, &m_endpoint,
        websocketpp::lib::placeholders::_1));
    con->set_close_handler(websocketpp::lib::bind(
        &connection_metadata::on_close, metadata_ptr, &m_endpoint,
        websocketpp::lib::placeholders::_1));
    con->set_message_handler(
        websocketpp::lib::bind(&connection_metadata::on_message, metadata_ptr,
                               websocketpp::lib::placeholders::_1,
                               websocketpp::lib::placeholders::_2));

    m_endpoint.connect(con);

    return new_id;
  }

  void close(int id, websocketpp::close::status::value code,
             std::string reason) {
    websocketpp::lib::error_code ec;

    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
      std::cout << "> No connection found with id " << id << std::endl;
      return;
    }

    m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
    if (ec) {
      std::cout << "> Error initiating close: " << ec.message() << std::endl;
    }
  }

  void send(int id, std::string message) {
    websocketpp::lib::error_code ec;

    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
      printf("> No connection found with id %d\n", id);
      return;
    }

    m_endpoint.send(metadata_it->second->get_hdl(), message,
                    websocketpp::frame::opcode::text, ec);
    if (ec) {
      printf("> Error sending message: %s\n", ec.message().c_str());
      return;
    } else {
      printf("> Send Success: %s\n", message.c_str());
    }
  }

  int send(int id, const char *pbuf, unsigned int len) {
    websocketpp::lib::error_code ec;

    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
      // printf("> No connect found with id %d\n", id);
      return -1;
    }

    m_endpoint.send(metadata_it->second->get_hdl(), pbuf, len,
                    websocketpp::frame::opcode::binary, ec);
    if (ec) {
      // printf("> Error sending binary2: %s\n", ec.message().c_str());
      return -1;
    }
    return 0;
  }

  void clear() {
    m_connection_list.clear();
    m_next_id = 0;
  }

  std::vector<std::string> GetMessage(int id) {
    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
      // printf("> No connect found with id %d\n", id);
      return std::vector<std::string>();
    }
    return metadata_it->second->GetMessage();
  }

  connection_metadata::ptr get_metadata(int id) const {
    con_list::const_iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
      return connection_metadata::ptr();
    } else {
      return metadata_it->second;
    }
  }

 private:
  typedef std::map<int, connection_metadata::ptr> con_list;

  client m_endpoint;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

  con_list m_connection_list;
  int m_next_id;
};

class UseWsClient {
 public:
  static UseWsClient *GetInstance();
  void init();
  
 public:
  int SendPcmData(unsigned int channel, const char *pbuf, unsigned int length);
  void WriteFloatPcm(unsigned int channel, const char *pbuf, unsigned int length);
  std::vector<std::string> GetMessage(unsigned int channel);

 private:
  static UseWsClient *m_pInstance;
  std::vector<std::pair<int, websocket_endpoint*>> m_vectWebSocketEP;
  std::map<int, FileWriteTest*> m_mapFILE;
};

#endif  //__ONLINE_WEBSOCKET_CLIENT_POCO_H__
