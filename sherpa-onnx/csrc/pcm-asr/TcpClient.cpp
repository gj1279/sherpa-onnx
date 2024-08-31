#include "TcpClient.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Timespan.h"

using namespace Poco;
using namespace Poco::Net;

TcpClient::TcpClient()
{
    m_iPort = 0;
    m_bFirstRecv = false;
}

TcpClient::~TcpClient()
{

}

int TcpClient::ConnectTo(std::string strIP, int iPort)
{
    try
    {
        if (strIP == m_strIP && iPort == m_iPort)
        {
            return 0;
        }
        printf("ConnectTo [%s:%d]\n", strIP.c_str(), iPort);
        m_bFirstRecv = false;
        SocketAddress sa(strIP, iPort);
        Poco::Timespan ts(5, 0);
        m_tcpSocket.connect(sa, ts);
        m_strIP = strIP;
        m_iPort = iPort;
        return 0;
    }
    catch (Poco::Exception &ec)
    {
        printf("%s catched, ec className [%s] code [%d] displayText [%s] message [%s] name [%s] what [%s]\n", __FUNCTION__, ec.className(), ec.code(), ec.displayText().c_str(), ec.message().c_str(), ec.name(), ec.what());
        return -1;
    }
    catch (...)
    {
        printf("%s catched\n", __FUNCTION__);
        return -1;
    }
}

int TcpClient::Disconnect()
{
    try
    {
        m_tcpSocket.close();
        return 0;
    }
    catch (...)
    {
        printf("%s catched\n", __FUNCTION__);
        return -1;
    }
}

int TcpClient::Start()
{
    try
    {
        printf("TcpClient Start\n");
        m_bStop = false;
        m_thread.setStackSize(2 * 1024 * 1024);
        m_thread.start(*this);
        return 0;
    }
    catch (...)
    {
        printf("%s catched\n", __FUNCTION__);
        return -1;
    }
}

int TcpClient::Stop()
{
    printf("%s in\n", __PRETTY_FUNCTION__);
    if (m_bStop)
    {
        return 0;
    }

    m_bStop = true;
    m_thread.join();
    return 0;
}

int TcpClient::SendData(const char *buf, int iLen)
{
    try
    {
        return m_tcpSocket.sendBytes(buf, iLen);
    }
    catch (...)
    {
        printf("%s catched\n", __PRETTY_FUNCTION__);
        return -1;
    }
}

int TcpClient::GetData(char *buf, int iLen)
{
    if (iLen <= 0)
    {
        return -1;
    }
    Poco::Mutex::ScopedLock lock(m_mutexRecvedData);
    if (m_vectRecvedData.size() > iLen)
    {
        std::copy_n(m_vectRecvedData.begin(), iLen, buf);
        m_vectRecvedData.erase(m_vectRecvedData.begin(), m_vectRecvedData.begin() + iLen);
        return iLen;
    }
    if (m_vectRecvedData.size() == iLen)
    {
        std::copy_n(m_vectRecvedData.begin(), iLen, buf);
        m_vectRecvedData.clear();
        return iLen;
    }
    return -1;
}

int TcpClient::GetDataLength()
{
    Poco::Mutex::ScopedLock lock(m_mutexRecvedData);
    return m_vectRecvedData.size();
}

void TcpClient::run()
{
    while (m_bStop == false)
    {
        if (Routine() != 0)
        {
            break;
        }
    }
}

int TcpClient::Routine()
{
    try
    {
        Timespan ts((long)60, (long)0);
        if (!m_tcpSocket.poll(ts, Poco::Net::Socket::SELECT_READ))
        {
            return 0;
        }
        if (m_tcpSocket.available() > 0)
        {
            char buf[2048];
            int iRecv = m_tcpSocket.receiveBytes(buf, 2048);
            Poco::Mutex::ScopedLock lock(m_mutexRecvedData);
            m_vectRecvedData.insert(m_vectRecvedData.end(), buf, buf + iRecv);
            if (m_bFirstRecv == false)
            {
                m_bFirstRecv = true;
                printf("first recv pcm data\n");
            }
            if (m_vectRecvedData.size() > 10 * 1024 * 1024)
            {
                //m_vectRecvedData.erase(m_vectRecvedData.begin(), m_vectRecvedData.begin() + m_vectRecvedData.size() - 10 * 1024 * 1024);
                m_vectRecvedData.clear();
                printf("error data received from tcp [%s:%d] too large\n", m_strIP.c_str(), m_iPort);
            }
        }
        else
        {
            printf("%s Error! tcp server is [%s:%d]\n", __PRETTY_FUNCTION__, m_strIP.c_str(), m_iPort);
            return -1;
        }
        return 0;
    }
    catch (...)
    {
        printf("%s catched\n", __FUNCTION__);
        return -1;
    }
}
