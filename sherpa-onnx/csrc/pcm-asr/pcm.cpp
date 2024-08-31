#include "pcm.h"
#include "online-websocket-client-poco.h"

PcmTrans::PcmTrans()
{
    m_bStop = true;
    m_pTcpClient = new TcpClient;
    m_pBuf = (unsigned char*)malloc(BUF_LENGTH);
    m_pTmp = (unsigned char*)malloc(BUF_LENGTH);
    m_iTotalBufLength = BUF_LENGTH;
    m_iLastLength = 0;
}

int PcmTrans::ConnectTo(std::string strIP, int iPort)
{
    int iRet = m_pTcpClient->ConnectTo(strIP, iPort);
    if (iRet == 0)
    {
        m_pTcpClient->Start();
        return 0;
    }
    return -1;
}

int PcmTrans::AddPcmData(int iIndex, char* pData, int iLen)
{
    if (iLen < 1)
    {
        return -1;
    }
    //printf("%s in, iIndex [%d] len [%d]\n", __FUNCTION__, iIndex, iLen);
    Poco::Mutex::ScopedLock lock(m_mutexPcmChannels);
    m_mapPcmChannels[iIndex].m_bHasData = true;
    std::deque<char> &dequeData = m_mapPcmChannels[iIndex].m_dequeData;
    dequeData.insert(dequeData.end(), pData, pData + iLen);
    int iLenLimit = 2048 * 10;
    int iSize = dequeData.size();
    if (iSize > iLenLimit)
    {
        dequeData.erase(dequeData.begin(), dequeData.begin() + (iSize - iLenLimit));
    }

    if (m_mapFILE.find(iIndex) == m_mapFILE.end())
    {
        char buf[100];
        memset(buf, 0, 100);
        sprintf(buf, "data/ap_%d.pcm", iIndex);
        FileWriteTest *fwt = new FileWriteTest(buf, 1024 * 1024);
        m_mapFILE[iIndex] = fwt;
    }
    else
    {
        m_mapFILE[iIndex]->Write(pData, iLen);
    }

	if (dequeData.size() > 10 * 1024)
	{
        char buf[10 * 1024];
        std::copy(dequeData.begin(), dequeData.begin() + 10 * 1024, buf);
        return UseWsClient::GetInstance()->SendPcmData(iIndex, pData, iLen);
	}

    return 0;
}

int PcmTrans::ReadFromSFT16AP()
{
    //printf("%s in\n", __FUNCTION__);
    if (m_pTcpClient == NULL || m_pBuf == NULL || m_pTmp == NULL)
    {
        //printf("%s tcp client is null [%#x] [%#x] [%#x]\n", __FUNCTION__, m_pTcpClient, m_pBuf, m_pTmp);
        return -1;
    }

    int iLength = m_pTcpClient->GetDataLength();
    if (iLength == 0)
    {
        //printf("%s length is 0\n", __FUNCTION__);
        return -2;
    }

    int iCurBufLength = m_iTotalBufLength - m_iLastLength;
    iLength = iLength > iCurBufLength ? iCurBufLength : iLength;
	if (m_iLastLength + iLength < 16 * 10 * 1024)
	{
        //printf("m_iLastLength + iLength < 16 * 10 * 1024\n");
        return -2;
	}

    if (m_pTcpClient->GetData((char*)&(m_pBuf[m_iLastLength]), iLength) == -1)
    {
        printf("%s GetData failed\n", __FUNCTION__);
        return -1;
    }
    //printf("GetData length [%d]\n", iLength);

    iLength += m_iLastLength;
    if (iLength < 8)
    {
        m_iLastLength = iLength;
        printf("%s audio length < 8\n", __FUNCTION__);
        return -1;
    }

    do
    {
        uint32_t u32AudioLength = *((uint32_t*)m_pBuf);
        if ((uint32_t)iLength < (u32AudioLength + 8))
        {
            m_iLastLength = iLength;
            //printf("%s audio leng [%d] < u32AudioLength [%d] + 8\n", __FUNCTION__, iLength, (int)u32AudioLength);
            break;
        }

        uint32_t u32Channels = *((uint32_t*)&(m_pBuf[4]));
        uint32_t u32ChannelLength = u32AudioLength / u32Channels;
        if ((u32AudioLength % u32Channels) != 0)
        {
            printf("error u32Audio length is [%u] channel length is [%u]\n", u32AudioLength, u32Channels);
            break;
        }
        if (u32Channels > 20)
        {
            printf("error 2 u32Audio length is [%u] channel length is [%u]\n", u32AudioLength, u32Channels);
            break;
        }
        for (uint32_t ui = 0; ui < u32Channels; ++ui)
        {
            //printf("before AddPcmData ui [%u] channelLength [%u]\n", ui, u32ChannelLength);
            AddPcmData(ui, (char*)&(m_pBuf[8 + ui * u32ChannelLength]), u32ChannelLength);
        }
        if (iLength > (int)u32AudioLength + 8)
        {
            m_iLastLength = iLength - (u32AudioLength + 8);
            iLength = m_iLastLength;
            memcpy(m_pTmp, &(m_pBuf[u32AudioLength + 8]), m_iLastLength);
            memcpy(m_pBuf, m_pTmp, m_iLastLength);
            //printf("last length [%d]\n", m_iLastLength);
        }
        else if (iLength == u32AudioLength + 8)
        {
            m_iLastLength = 0;
            break;
        }
        else
        {
        }
    } while (true);

    return 0;
}

int PcmTrans::Start()
{
    try
    {
        m_bStop = false;
        //m_thread.setStackSize(2 * 1024 * 1024);
        m_thread.start(*this);
        return 0;
    }
    catch (...)
    {
        printf("%s catched\n", __FUNCTION__);
        return -1;
    }
}

void PcmTrans::run()
{
    while (!m_bStop)
    {
        if (Routine() == 0)
        {
            Poco::Thread::sleep(2);
            continue;
        }
        Poco::Thread::sleep(5);
    }
}

int PcmTrans::Routine()
{
    return ReadFromSFT16AP();
}
