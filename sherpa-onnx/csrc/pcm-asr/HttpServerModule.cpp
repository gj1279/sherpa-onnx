#include "HttpServerModule.h"
#include "main.h"

#ifdef PFRK3588
#define MHD_Result int
#endif

using namespace std;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Dynamic;

HttpServerModule* HttpServerModule::m_pInstance = NULL;

#define PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define REDIRECT "<html><head><title>redirect</title></head><body><a href=\"http://127.0.0.1:80/web/login.html\">main</a></body></html>"
#define BUFFER_SIZE  (128*1024)
#define MAX_HEADER_LENGTH (256*1024)
#define MAX_BODY_LENGTH (60*1024*1024)


static ssize_t
file_reader(void *cls,
uint64_t pos,
char *buf,
size_t max)
{
    FILE *file = (FILE*)cls;
    fseeko(file, pos, SEEK_SET);
    return fread(buf, 1, max, file);
}


static void
free_callback(void *cls)
{
    FILE *file = (FILE*)cls;
    fclose(file);
}

static int
process_upload_data(void *cls,
enum MHD_ValueKind kind,
    const char *key,
    const char *filename,
    const char *content_type,
    const char *transfer_encoding,
    const char *data,
    uint64_t off,
    size_t size)
{
    int ret = MHD_NO;
    
    return ret;
}

static MHD_Result Access_callback(void *cls,
struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size, void **ptr)
{
    try
    {
        MHD_Result ret = MHD_NO;

        if ((0 != strcmp(method, MHD_HTTP_METHOD_GET)) &&
            (0 != strcmp(method, MHD_HTTP_METHOD_POST)))
            return MHD_NO;

        if (std::string(url).find("..") != std::string::npos)
        {
            return MHD_NO;
        }

        if (0 == strcmp(method, MHD_HTTP_METHOD_POST))
        {
            if (NULL != strstr(url, "passiveProtocol.action"))
            {
                if (*ptr == NULL)
                {
                    char * psz = new char[BUFFER_SIZE];
                    psz[0] = 0;
                    psz[BUFFER_SIZE - 1] = 0;
                    //memset(psz, 0, BUFFER_SIZE);
                    *ptr = psz;
                    return MHD_YES;
                }
                char *  pszContent = (char *)*ptr;

                if (*upload_data_size == 0)
                {
                  ret = (MHD_Result)((HttpServerModule *)cls)->ProcessActiveProtocol(connection, pszContent, url);
                    if (pszContent != NULL)
                    {
                        delete[] pszContent;
                        pszContent = NULL;
                        *ptr = NULL;
                    }
                }
                else
                {
                    int iSize = strlen(pszContent);
                    if ((iSize + *upload_data_size) > (BUFFER_SIZE - 4))
                    {
                        printf("protocol too big\n");
                    }
                    else
                    {
                        memcpy(pszContent + iSize, upload_data, *upload_data_size);
                        pszContent[iSize + (*upload_data_size)] = 0;
                    }
                    *upload_data_size = 0;
                    return MHD_YES;
                }
            }
            else if (
                (NULL != strstr(url, "asrkaldi.action"))
                || (NULL != strstr(url, "manage.co"))
                || (NULL != strstr(url, "adjust.co"))
                || (NULL != strstr(url, "cameraJoint.co"))
                )
            {
                if (*ptr == NULL)
                {
                    char * psz = new char[BUFFER_SIZE];
                    psz[0] = 0;
                    psz[BUFFER_SIZE - 1] = 0;
                    //memset(psz, 0, BUFFER_SIZE);
                    *ptr = psz;
                    return MHD_YES;
                }
                char *  pszContent = (char *)*ptr;

                if (*upload_data_size == 0)
                {
                  ret = (MHD_Result)((HttpServerModule *)cls)->ProcessPassiveProtocol(connection, pszContent, url);
                    if (pszContent != NULL)
                    {
                        delete[] pszContent;
                        pszContent = NULL;
                        *ptr = NULL;
                    }
                }
                else
                {
                    int iSize = strlen(pszContent);
                    if ((iSize + *upload_data_size) > (BUFFER_SIZE - 4))
                    {
                        printf("protocol too big\n");
                    }
                    else
                    {
                        memcpy(pszContent + iSize, upload_data, *upload_data_size);
                        pszContent[iSize + (*upload_data_size)] = 0;
                    }
                    *upload_data_size = 0;
                    return MHD_YES;
                }
            }
            else
            {
                ret = MHD_NO;
            }
        }
        else if ((0 == strcmp(method, MHD_HTTP_METHOD_GET)))
        {
          ret = (MHD_Result)((HttpServerModule *)cls)->ProcessGETRequest(connection, url);
        }
        return ret;
    }
    catch (...)
    {
        printf("Access_callback  catch!\n");
        return MHD_NO;
    }
}

static void
response_completed_callback(void *cls,
struct MHD_Connection *connection,
    void **con_cls,
enum MHD_RequestTerminationCode toe)
{

}


HttpServerModule::HttpServerModule(void) :
m_mutexHttpServer()
{
    //m_eStatus = UPGRADE_STATUS_NONE;
    m_iUploadPercent = 0;
    m_sDescription = "";
    m_bEnterScreenSaveEnable = false;
    m_bFormatting = false;
    m_bCatchedForDisk = false;
}

HttpServerModule::~HttpServerModule(void)
{

}

HttpServerModule *HttpServerModule::Initialize()
{
    return HttpServerModule::GetInstance();
}

void HttpServerModule::Uninitialize()
{
    if (m_pInstance != NULL)
    {
        m_pInstance->UnInit();
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

HttpServerModule *HttpServerModule::GetInstance()
{
    if (m_pInstance == NULL)
    {
        m_pInstance = new HttpServerModule;
    }

    return m_pInstance;
}

void HttpServerModule::Init()
{
  // http
  HttpServerChannel *pChannel = new HttpServerChannel();
  pChannel->Start(1080, &Access_callback, (void *)this,
                  &response_completed_callback, false);
  m_vectChannels.push_back(pChannel);
}

void HttpServerModule::UnInit()
{
    for (int i = 0; i< (int)m_vectChannels.size(); i++)
    {
        if (m_vectChannels[i] != NULL)
        {
            m_vectChannels[i]->Stop();
            delete m_vectChannels[i];
            m_vectChannels[i] = NULL;
        }
    }
}

int HttpServerModule::ProcessActiveProtocol(struct MHD_Connection *connection, char *  pszContent, const char *  url)
{
    try
    {
        struct MHD_Response *response;
        int ret = MHD_NO;
        //解析协议，并返回
        std::string jsonRecv = pszContent;
        std::string jsonSend = "";

        if (NULL != strstr(url, "passiveProtocol.action"))
        {
        }

        response = MHD_create_response_from_buffer(jsonSend.length(), (void *)jsonSend.c_str(), MHD_RESPMEM_MUST_COPY);
        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);


        return ret;
    }
    catch (...)
    {
        printf("http server  process active protocol break\n");
        return MHD_NO;
    }
}

int HttpServerModule::ProcessPassiveProtocol(struct MHD_Connection *connection, char *  pszContent, const char * url)
{
    try
    {
        int ret = MHD_NO;
        struct MHD_Response *response;
        //解析协议，并返回
        std::string jsonRecv = pszContent;
        std::string jsonSend = "";

        if (NULL != strstr(url, "asrkaldi.action"))
        {
            Parser parser;
            Var result;
            result = parser.parse(jsonRecv);
            if (result.type() == typeid(Object::Ptr))
            {
                Object::Ptr object = result.extract<Object::Ptr>();
                if ((!object.isNull()) && (object->size() > 0))
                {
                    DynamicStruct ds = Object::makeStruct(object);
                    std::string sMethod = ds["method"];
                    if (ds.size() > 3)
                    {
                        jsonSend = ErrJson();
                    }
                    else
                    {
                        jsonSend = ProcessProtocol(sMethod, jsonRecv);
                    }
                }
            }
        }
        

        response = MHD_create_response_from_buffer(jsonSend.length(), (void *)jsonSend.c_str(), MHD_RESPMEM_MUST_COPY);


        MHD_add_response_header(response, "Content-Type", "application/json");
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");

        if (jsonSend != "ErrJson")
        {
            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        }
        else
        {
            ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        }
        MHD_destroy_response(response);
        return ret;
    }
    catch (...)
    {
        printf("http server  process passive protocol break, url [%s]\n", url);
        return MHD_NO;
    }
}

std::string HttpServerModule::ProcessProtocol(std::string sMethod, std::string jsonRecv)
{
    try
    {
        std::string jsonSend = "";
        if (sMethod == "getASRResult")
        {
          jsonSend = GetASRResult(sMethod, jsonRecv);
        }
        else
        {
            printf("%s : do not support this method [%s], recv [%s]\n", __PRETTY_FUNCTION__, sMethod.c_str(), jsonRecv.c_str());
        }
        return jsonSend;
    }
    catch (...)
    {
        printf("%s catched\n", __PRETTY_FUNCTION__);
        return "";
    }
}


std::string HttpServerModule::ErrJson()
{
    printf("%s in\n", __FUNCTION__);
    return "ErrJson";
}

static int
print_out_key(void *cls, enum MHD_ValueKind kind, const char *key,
const char *value)
{
    printf("%s: %s\n", key, value);
    return MHD_YES;
}

int HttpServerModule::ProcessGETRequest(struct MHD_Connection *connection, const char *  url)
{
    bool bDownloadLog = false;
    try
    {
        struct MHD_Response *response;
        int ret = MHD_NO;
        std::string sUrlTemp = url;
        int iUrlTempLen = sUrlTemp.length();

        if (((sUrlTemp.length() >= 4) && (sUrlTemp.substr(0, 4) != "/web"))
            && ((sUrlTemp.find("download") == std::string::npos)))
        {
            return MHD_NO;
        }

        if ((iUrlTempLen >= 8) && (sUrlTemp.substr(iUrlTempLen - 8, 8) == "download"))
        {
            /*MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, print_out_key, NULL);
            const char* szFileType = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "fileType");
            const char* szToken = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "token");

            if (szFileType == NULL)
            {
                return MHD_NO;
            }*/
        }
        else
        {
            FILE *file = NULL;
            struct stat buf;
            char szUrl[1024];
            int iPosUrl = 0;
            memset(szUrl, 0, 1024);
            strncpy(szUrl, &url[1], 1024);
            //printf("szUrl [%s]\n", szUrl);

#ifdef WIN32
            if (GetModuleFileNameA(NULL, szUrl, 1024) > 0)
            {
                iPosUrl = strlen(szUrl) - 1;
                while (iPosUrl > 0 && szUrl[iPosUrl] != '\\')
                {
                    szUrl[iPosUrl] = '\0';
                    iPosUrl--;
                }
                strncat(szUrl, &url[1], 1024);
            }
#endif

            if (0 == stat(szUrl, &buf))
                file = fopen(szUrl, "rb");
            else
                file = NULL;
            if (NULL == file)
            {
                if (strlen(&url[1]) == 0)
                {
                    response = MHD_create_response_from_buffer(strlen(REDIRECT),
                        (void *)REDIRECT,
                        MHD_RESPMEM_PERSISTENT);
                    MHD_add_response_header(response, MHD_HTTP_HEADER_LOCATION, "/web/login.html");
                    ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, response);
                    MHD_destroy_response(response);
                }
                else
                {
                    response = MHD_create_response_from_buffer(strlen(PAGE),
                        (void *)PAGE,
                        MHD_RESPMEM_PERSISTENT);
                    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
                    MHD_destroy_response(response);
                }
            }
            else
            {
                response = MHD_create_response_from_callback(buf.st_size, 32 * 1024,     /* 32k page size */
                    &file_reader,
                    file,
                    &free_callback);
                if (NULL == response)
                {
                    fclose(file);
                    ret = MHD_NO;
                }
                else
                {
                    std::string sUrl = szUrl;
                  std::string sContentType;
                    if (sContentType != "")
                    {
                        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, sContentType.c_str());
                    }
                    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                    MHD_destroy_response(response);
                }
            }
        }
        return ret;
    }
    catch (...)
    {
        printf("%s catched\n", __PRETTY_FUNCTION__);
        if (bDownloadLog)
        {
            m_bCatchedForDisk = true;
        }
        return MHD_NO;
    }
}

std::string HttpServerModule::GetASRResult(std::string sMethod, std::string jsonRecv)
{
	try
	{
    Poco::JSON::Object::Ptr pObj =  new Poco::JSON::Object(true);
    Poco::JSON::Object::Ptr pObj1 = new Poco::JSON::Object(true);
    Poco::JSON::Object::Ptr pObj2 = new Poco::JSON::Object(true);
    std::map<int, std::vector<std::string>> asrREsult = GetMessage_();
    Poco::JSON::Array::Ptr pArr = new Poco::JSON::Array;
	for (std::map<int, std::vector<std::string>>::iterator i = asrREsult.begin(); i != asrREsult.end(); ++i)
	{
		for (std::vector<std::string>::iterator i2 = i->second.begin(); i2 < i->second.end(); ++i2)
		{
            printf("first %d i2 %s\n", i->first, i2->c_str());
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var var = parser.parse(*i2);
            Poco::JSON::Object::Ptr pObjASR = var.extract<Poco::JSON::Object::Ptr>();
            pObjASR->set("channel", i->first);
            pArr->add(pObjASR);
            //printf("")
		}
	}
    pObj2->set("retCode", 0);
    pObj2->set("retMessage", "ok");
    pObj2->set("asrResult", pArr);
    pObj1->set("method", sMethod);
    pObj1->set("params", pObj2);
    pObj->set("result", pObj1);

    std::stringstream ss;
    pObj->stringify(ss);
    std::string strRespose = ss.str();

    printf("[%s] out!\n", __PRETTY_FUNCTION__);
    return strRespose;
	}
	catch (...)
	{
        printf("%s catched\n", __FUNCTION__);
        return "";
	}
}
