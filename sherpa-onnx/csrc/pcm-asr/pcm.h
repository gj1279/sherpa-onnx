#ifndef _PCM_H_
#define _PCM_H_

#include <cstdio>
#include <cstdlib>
#include <map>
#include <deque>

#include "TcpClient.h"
#include "Poco/Thread.h"

#define BUF_LENGTH 20 * 10 * 1024

class PcmProtocolAChannel
{
public:
    PcmProtocolAChannel(){ m_bHasData = false; }
    bool                    m_bHasData;
    std::deque<char>        m_dequeData;
};

class PcmTrans : public Poco::Runnable
{
public:
    PcmTrans();

public:
    int ConnectTo(std::string strIP, int iPort);
    int AddPcmData(int iIndex, char* pData, int iLen);
    int ReadFromSFT16AP();

public:
    int Start();

protected:
    virtual void run();
    int Routine();

private:
    Poco::Thread            m_thread;
    bool                    m_bStop;

private:
    TcpClient*              m_pTcpClient;
    unsigned char*          m_pBuf;
    unsigned char*          m_pTmp;
    int                     m_iTotalBufLength;
    int                     m_iLastLength;
    Poco::Mutex             m_mutexPcmChannels;
    std::map<int, PcmProtocolAChannel> m_mapPcmChannels;
    std::map<int, FileWriteTest*> m_mapFILE;
};

#endif // _PCM_H_
