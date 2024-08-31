#include "online-websocket-client-poco.h"

UseWsClient *UseWsClient::m_pInstance = NULL;

UseWsClient *UseWsClient::GetInstance() {
  if (m_pInstance == NULL) {
    static Poco::Mutex s_mutex;
    Poco::Mutex::ScopedLock lock(s_mutex);
    if (m_pInstance == NULL) {
      UseWsClient *tmp = new UseWsClient;
      m_pInstance = tmp;
    }
  }
  return m_pInstance;
}

void UseWsClient::init()
{
  for (int i = 0; i < 16; i++)
  {
    websocket_endpoint *p = new websocket_endpoint;
    int id = p->connect("127.0.0.1", 50082);
    std::pair<int, websocket_endpoint *> pair(id, p);
    m_vectWebSocketEP.push_back(pair);
  }
}

int UseWsClient::SendPcmData(unsigned int channel, const char *pbuf, unsigned int length)
{
  if (channel > m_vectWebSocketEP.size())
  {
    return -1;
  }

  //printf("SendPcmData [%d]\n", channel);
  float pcm[length / 2];
  memset((void *)pcm, 0, length / 2);
  for (int i = 0; i < length;)
  {
    pcm[i / 2] = *(int16_t *)&pbuf[i] / 32768.0;
    i += 2;
  }
  WriteFloatPcm(channel, (char *)pcm, (length / 2) * sizeof(float));
  std::pair<int, websocket_endpoint *> pair = m_vectWebSocketEP[channel];
  return pair.second->send(pair.first, (char*)pcm, (length / 2) * sizeof(float));
}

void UseWsClient::WriteFloatPcm(unsigned int channel, const char *pbuf, unsigned int length)
{
  if (m_mapFILE.find(channel) == m_mapFILE.end())
  {
    char buf[32];
    memset(buf, 0, 32);
    sprintf(buf, "data/ap_float_%d.pcm", channel);
    m_mapFILE[channel] = new FileWriteTest(buf, 1024 * 1024);
    printf("open [%s] %d\n", buf, m_mapFILE[channel]->IsOpen());
  }
  m_mapFILE[channel]->Write(pbuf, length);
}

std::vector<std::string> UseWsClient::GetMessage(unsigned int channel)
{
  if (channel > m_vectWebSocketEP.size()) {
    return std::vector<std::string>();
  }
  std::pair<int, websocket_endpoint *> pair = m_vectWebSocketEP[channel];
  return pair.second->GetMessage(pair.first);
}
