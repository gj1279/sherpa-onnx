#include "HttpServerChannel.h"

HttpServerChannel::HttpServerChannel(void)
{
    m_d = NULL;
}

HttpServerChannel::~HttpServerChannel(void)
{
    if (m_d != NULL)
    {
        MHD_stop_daemon(m_d);
        m_d = NULL;
    }
}


/* default server key */
const char g_SrvSignedKeyPem[] = "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEA8NbL7quKklGMkWISzJiP4+NfQC1KBXYopqJJVYGtixFJM/aC\n"
"QuRJhiIGnlgMCtMDf1bH0ONiLkxL6EEuxy59o0cu7e37Y2u+ugvTY9pf/RyA0+hk\n"
"bpmdAAASZiwV5ur5EpVGyOgA5zcuUNtD1KaO3dYOZ7N+CsKWuqv1k33qT7A+j8HU\n"
"23Gg3R4Ca2oKv9yik64FBCffWBFpZdtDBkCESmI3ukSH+C3W2R8F05rB6uRcetJK\n"
"RrskZ4hv7Ez+arYkhmVi7yG7DU2Mm/dAsaWIiYn9vwSm61jW3ByBAUwsbnTBFZL5\n"
"2MGSre3yoVf8IUYc8elkFX52BC+cOEm0vT6QeQIDAQABAoIBAQCLIFpwhrInMmrU\n"
"tWYMwPkmeXvV1BPVsdtr34YjmiUJmDPEi9vWDpCWpuNR56UVP67eieAmQKTAAB09\n"
"jyIZuDal8wIeOoMlfOGSEjDMlyMqUaPuRmp2JdE3ZdM7KK6CVi0KWieo+bVD3HGq\n"
"DfBSUOFpLzXlHCBs0NpH/fyAdpW78AJXSY2zxON2Orrlky120Ua4bXPb8ds0/yTJ\n"
"8Kr4agLs341NEUr3cET+h9SjCKoTRpKF3LuU6TspPkDapumt/R7CBBvFI8+sJDhL\n"
"XMsZ3gM/TQGo4EYXPO5BbFLOOZGIw1v33AxnjDjt+Jg4whOPAut0WLFBm4dFm6i9\n"
"5U66Mt2BAoGBAPyl3KtQGAFO35M2j+ISDLCgzXrxwkm78D9UVfzGM5GdoXpYuVoQ\n"
"x8TdLafkFeCfD7Zm/VJqFfFAuzKwjc8LKVHfDrWNKx+WohgFLOF8wRdJhZjMwrxB\n"
"Z2UhWWuswOKlGCyq34MlAZ17dofxGA7Zj9I4eixOrda+wLHsHKQ5CTupAoGBAPQI\n"
"0zBrRjVMPwIgrktCbz7vcphbbXfBxxHd7kR91aBUKxjWt5kkqQRoVL707TPRud9z\n"
"FCmH29Q3tCiVRQWQm/inoVgPFctpqReySLA8tVhBfw5Y19zb+XsZZDELMCjHgnSE\n"
"7+1WEpJlsPrSzRmxIJtKfYyRuMDbWIRwVqji6zBRAoGBAPx1gRxu3I3yEtc4Hm75\n"
"OlUFWk8QZeToOBoQXBxKsh1ANqvbHNwzCGxf1898y5+5Uioin+BKwcJhvK3aXtoV\n"
"fSSikEA64GygSTlXcpGf6BMtFKKreaxEbt1PhdtitRvFfXKlDNQvln2XccpF0JSG\n"
"MoEOjobX2D5OPp+MINK5a9CBAoGAQ57uZkxTeBFYpE5J/bC7AsV3C6DKBJU9hEXq\n"
"8C+uMm8gQhG9bkIqU35VfVSBJgrHZpOM0rVCxNtqoJQ8O/6GObC5lJ0ZfPQaVuag\n"
"HbW0ym/btS4JIroRt3qhuLVQ8vvVulPb5/ghzU9Aa0BedPAQCPfrMjhu6YHeVVSL\n"
"ruCyKwECgYBurLFT5d46AwNQp4OyajP3uRrocKePACqkjNkWdA0EkOM1i0MVCvbu\n"
"zp8ntkuZ+oTQHqzyKp4kwaBC2eyIbi7Zvbe4Ae984FfeDqp4xJkfF6ao9jp5w6Qj\n"
"HwiKppUU9zyvLLm2g6O7Fas+2xNXjxw93gQSv4q0yv6oxhrDB4u4iw==\n"
"-----END RSA PRIVATE KEY-----\n";

const char g_SrvSignedCertPem[] = "-----BEGIN CERTIFICATE-----\n"
"MIIC3jCCAcYCCQDTPWSPqgiT/jANBgkqhkiG9w0BAQUFADAxMQswCQYDVQQGEwJD\n"
"TjEQMA4GA1UECAwHYmVpamluZzEQMA4GA1UEBwwHYmVpamluZzAeFw0xNzA5MjAx\n"
"MTUxMTFaFw0xNzEwMjAxMTUxMTFaMDExCzAJBgNVBAYTAkNOMRAwDgYDVQQIDAdi\n"
"ZWlqaW5nMRAwDgYDVQQHDAdiZWlqaW5nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\n"
"MIIBCgKCAQEA8NbL7quKklGMkWISzJiP4+NfQC1KBXYopqJJVYGtixFJM/aCQuRJ\n"
"hiIGnlgMCtMDf1bH0ONiLkxL6EEuxy59o0cu7e37Y2u+ugvTY9pf/RyA0+hkbpmd\n"
"AAASZiwV5ur5EpVGyOgA5zcuUNtD1KaO3dYOZ7N+CsKWuqv1k33qT7A+j8HU23Gg\n"
"3R4Ca2oKv9yik64FBCffWBFpZdtDBkCESmI3ukSH+C3W2R8F05rB6uRcetJKRrsk\n"
"Z4hv7Ez+arYkhmVi7yG7DU2Mm/dAsaWIiYn9vwSm61jW3ByBAUwsbnTBFZL52MGS\n"
"re3yoVf8IUYc8elkFX52BC+cOEm0vT6QeQIDAQABMA0GCSqGSIb3DQEBBQUAA4IB\n"
"AQBpAev1iYikoaGPdM+4AN0irQep0ZtOhRM94fzCdhh7//DMRHIbgWrlo3sAOQci\n"
"LWVtSAl32LIeopis2yf82xujVWd1XZXjSPWhfJCpI6S9s8KtjMekCn1M6l2UFn8U\n"
"oryUL7Dj/AUfTG7ofTNOSvB+zRd0SDoxL8f/fnFKcimHnazPc74R9ZYOY5qWta54\n"
"aqmeJqmZxWaBN9ZnlEcWGQdodqvRoJzjiMpxXkWSqng2rUulLoaGSwqpg2DK39x3\n"
"nZ8tvvUVjtl/rqilsgqKgupw92Dqflf4N+LQu/IoqODJ2IRR1d5dS4eU88zJrsOC\n"
"4TgybTBhuGCx32+uFLAfGHRw\n"
"-----END CERTIFICATE-----\n";




int HttpServerChannel::Start(int iPort, MHD_AccessHandlerCallback cbfAccess, void* dh_cls, MHD_RequestCompletedCallback cbfComplete, bool bIsHttps)
{
    if (bIsHttps)
    {
        char *pKeyPem = NULL;
        char *pCertPem = NULL;
        int nKeyPemSize;
        int nCertPemSize;
        //pKeyPem = CCBUtility::GetFileContent((char *)"./https/ssl.key", nKeyPemSize);
        //pCertPem = CCBUtility::GetFileContent((char *)"./https/ssl.crt", nCertPemSize);
        if (pKeyPem != NULL && pCertPem != NULL)
        {
            m_d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL,
                iPort,
                NULL, NULL, cbfAccess, dh_cls,
                MHD_OPTION_CONNECTION_TIMEOUT, 120 /* seconds */,
                MHD_OPTION_NOTIFY_COMPLETED, cbfComplete, NULL,
                MHD_OPTION_HTTPS_MEM_KEY, pKeyPem,
                MHD_OPTION_HTTPS_MEM_CERT, pCertPem,
                MHD_OPTION_END);
        }
        else
        {
            m_d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL,
                iPort,
                NULL, NULL, cbfAccess, dh_cls,
                MHD_OPTION_CONNECTION_TIMEOUT, 120 /* seconds */,
                MHD_OPTION_NOTIFY_COMPLETED, cbfComplete, NULL,
                MHD_OPTION_HTTPS_MEM_KEY, g_SrvSignedKeyPem,
                MHD_OPTION_HTTPS_MEM_CERT, g_SrvSignedCertPem,
                MHD_OPTION_END);
        }
        if (pKeyPem != NULL)
        {
            delete[]pKeyPem;
            pKeyPem = NULL;
        }
        if (pCertPem != NULL)
        {
            delete[]pCertPem;
            pCertPem = NULL;
        }
    }
    else
    {
        m_d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
            iPort,
            NULL, NULL, cbfAccess, dh_cls,
            MHD_OPTION_CONNECTION_TIMEOUT, 120 /* seconds */,
            MHD_OPTION_NOTIFY_COMPLETED, cbfComplete, NULL,
            MHD_OPTION_END);
    }
    if (m_d == NULL)
    {
        printf("http server start fail , port = %d\n",iPort);
        return -1;
    }
    else
    {
        printf("http server start success , port = %d\n",iPort);
        return 0;
    }
}

int HttpServerChannel::Stop()
{
    if (m_d != NULL)
    {
        MHD_stop_daemon(m_d);
        m_d = NULL;
    }
    return 0;
}
