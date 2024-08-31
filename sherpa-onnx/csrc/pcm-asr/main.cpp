#include "TcpClient.h"
#include "online-websocket-client-poco.h"
#include "pcm.h"
#include "main.h"

#include "Poco/JSON/JSON.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Query.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/JSON/ParseHandler.h"
#include "Poco/JSON/PrintHandler.h"
#include "Poco/JSON/Template.h"
#include "HttpServerModule.h"

using namespace std;

Poco::Mutex asrResultMutex;
std::map<int, std::vector<std::string>> asrResult_;

std::string server_ip = "127.0.0.1";
int32_t server_port = 50082;
std::string pcm_server_ip = "10.1.7.236";
int32_t pcm_server_port = 5700;
std::string result_server_ip = "127.0.0.1";
int32_t result_server_port = 5701;

int init_params(int32_t argc, char *argv[], string &server_ip,
                int32_t &server_port, string &pcm_server_ip,
                int32_t &pcm_server_port, string &res_server_ip,
                int32_t &res_server_port) {
  if (!websocketpp::uri_helper::ipv4_literal(server_ip.begin(),
                                             server_ip.end())) {
    printf("Invalid server IP: %s", server_ip.c_str());
    return -1;
  }

  if (server_port <= 0 || server_port > 65535) {
    printf("Invalid server port: %d", server_port);
    return -1;
  }

  if (!websocketpp::uri_helper::ipv4_literal(pcm_server_ip.begin(),
                                             pcm_server_ip.end())) {
    printf("Invalid pcm server IP: %s", server_ip.c_str());
    return -1;
  }

  if (pcm_server_port <= 0 || pcm_server_port > 65535) {
    printf("Invalid pcm server port: %d", server_port);
    return -1;
  }

  if (!websocketpp::uri_helper::ipv4_literal(res_server_ip.begin(),
                                             res_server_ip.end())) {
    printf("Invalid result server IP: %s", server_ip.c_str());
    return -1;
  }

  if (res_server_port <= 0 || res_server_port > 65535) {
    printf("Invalid result server port: %d", server_port);
    return -1;
  }
  return 0;
}

void DisplayMessage(int channel, std::vector<std::string> messages) {
  try {
    for (size_t i = 0; i < messages.size(); ++i) {
      Poco::JSON::Parser parser;
      Poco::Dynamic::Var result = parser.parse(messages[i]);
      Poco::JSON::Object::Ptr pObj = result.extract<Poco::JSON::Object::Ptr>();
      std::string text = pObj->get("text").toString();
      if (text.empty()) {
        continue;
	  }
      printf("mic [%d][%d], message [%s]\n", channel, i, text.c_str());
    }
  }
  catch (...) {

  }
}

std::map<int, std::vector<std::string>> GetMessage_() {
  Poco::Mutex::ScopedLock lock(asrResultMutex);
  std::map<int, std::vector<std::string>> tmp(asrResult_);
  asrResult_.clear();
  return tmp;
}

void AddMessage(int channel, std::vector<std::string> messages) {
  Poco::Mutex::ScopedLock lock(asrResultMutex);
  asrResult_[channel].insert(asrResult_[channel].end(), messages.begin(),messages.end());
}

void GetMessage()
{
	for (int i = 0; i < 16; ++i)
	{
		std::vector<std::string> messages = UseWsClient::GetInstance()->GetMessage(i);
		if (messages.empty())
		{
            continue;
		}
        DisplayMessage(i, messages);
        AddMessage(i, messages);
	}
}

int32_t main(int32_t argc, char *argv[]) {
  if (init_params(argc, argv, server_ip, server_port, pcm_server_ip,
                  pcm_server_port, result_server_ip, result_server_port) != 0) {
    return -1;
  }

  UseWsClient::GetInstance();
  UseWsClient::GetInstance()->init();

  PcmTrans pcm;
  pcm.ConnectTo(pcm_server_ip, pcm_server_port);
  pcm.Start();

  TcpClient tcp;
  //tcp.ConnectTo()

  HttpServerModule::Initialize();
  HttpServerModule::GetInstance()->Init();

  while (1)
  {
    GetMessage();
    usleep(50 * 1000);
  }

  return 0;
}
