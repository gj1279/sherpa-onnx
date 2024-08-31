#ifndef HTTPSERVERMODULE_INCLUDED
#define HTTPSERVERMODULE_INCLUDED
#include "HttpServerChannel.h"
#include "Poco/JSON/JSON.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/JSON/ParseHandler.h"
#include "Poco/JSON/PrintHandler.h"
#include "Poco/Mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <vector>

class HttpServerModule
{
private:
    HttpServerModule(void);
    ~HttpServerModule(void);
public:
    static HttpServerModule *Initialize();
    static void Uninitialize();
    static HttpServerModule *GetInstance();
public:
    void Init();
private:
    void UnInit();
public:
    int ProcessActiveProtocol(struct MHD_Connection *connection, char *  pszContent, const char *  url);
    int ProcessPassiveProtocol(struct MHD_Connection *connection,char *  pszContent,const char *  url);
    int ProcessGETRequest(struct MHD_Connection *connection,const char *  url);
    std::string ProcessProtocol(std::string sMethod, std::string jsonRecv);
    std::string ErrJson();
    std::string GetASRResult(std::string sMethod, std::string jsonRecv);
    
private:
    static HttpServerModule   *m_pInstance;
    std::vector<HttpServerChannel*>  m_vectChannels;
    Poco::Mutex                 m_mutexHttpServer;

    int m_iUploadPercent;
    bool m_bEnterScreenSaveEnable;
    std::string m_sDescription;

    bool m_bFormatting;
    //Poco::Clock                     m_clkDevRunTime;
    bool m_bCatchedForDisk;
};

#endif