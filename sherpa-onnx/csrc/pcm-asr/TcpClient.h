#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <vector>
#include "Poco/Mutex.h"
#include "Poco/Net/StreamSocket.h"

class FileWriteTest
{
public:
    FileWriteTest(const char* sPath, int iSizeLimit)
    {
        m_iTotalSize = 0;
        m_iSizeLimit = iSizeLimit;
        m_pfd = fopen(sPath, "w");
    }
    ~FileWriteTest()
    {
        if (m_pfd)
        {
            fclose(m_pfd);
        }
        m_pfd = NULL;
    }

    int Write(const char* buf, int iLen)
    {
        if (m_pfd)
        {
            if (m_iTotalSize >= m_iSizeLimit)
            {
                fclose(m_pfd);
                m_pfd = NULL;
                return 0;
            }
            fwrite(buf, iLen, 1, m_pfd);
            fflush(m_pfd);
            m_iTotalSize += iLen;
            return 0;
        }
        return -1;
    }

    bool IsOpen() { return m_pfd == NULL; }

private:
    FILE* m_pfd;
    int m_iSizeLimit;
    int m_iTotalSize;
};

class TcpClient : public Poco::Runnable
{
public:
    TcpClient();
    ~TcpClient();
    int ConnectTo(std::string strIP, int iPort);
    int Disconnect();
    int Start();
    int Stop();

    int SendData(const char *buf, int iLen);
    int GetData(char *buf, int iLen);
    int GetDataLength();

protected:
    void run();
    int Routine();

private:
    Poco::Thread                m_thread;
    bool                        m_bStop;
    Poco::Mutex                 m_mutexRecvedData;
    std::vector<unsigned char>  m_vectRecvedData;
    Poco::Net::StreamSocket     m_tcpSocket;

private:
    std::string                 m_strIP;
    int                         m_iPort;
    bool                        m_bFirstRecv;
};

#endif //__TCP_CLIENT_H__