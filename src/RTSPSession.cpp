// MediaLAN 02/2013
// CRtspSession
// - parsing of RTSP requests and generation of RTSP responses

#ifdef __linux__
#include <sys/socket.h>
# include "RTSPSession.lin.h"
#elif _WIN32
# include "RTSPSession.win.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "JPEGSamples.h"

RTSPSession::RTSPSession(int aRtspClient, Streamer* aStreamer):
    _rtspClient(aRtspClient), _streamer(aStreamer)
{
    _init();

    // create a session ID
    {
        _rtspSessionID = rand() << 16;
        _rtspSessionID |= rand();
        _rtspSessionID |= 0x80000000;         
    }
    _streamID = -1;
    _clientRTPPort = 0;
    _clientRTCPPort = 0;
    _tcpTransport = false;
};

RTSPSession::~RTSPSession() {};

RetCode RTSPSession::_init()
{
    _rtspCmdType = RTSP_UNDEFINED;
    memset(_URLPreSuffix, 0, sizeof(_URLPreSuffix));
    memset(_URLSuffix, 0, sizeof(_URLSuffix));
    memset(_cmdSeq, 0, sizeof(_cmdSeq));
    memset(_URLHostPort, 0, sizeof(_URLHostPort));
    _сontentLength  =  0;
    return RetCode::RC_SUCCESS;
};

RetCode RTSPSession::_parseRtspRequest(char const* aRequest, const size_t aRequestSize)
{
    char    cmdName[RTSP_PARAM_STRING_MAX + 1];
    char    curRequest[RTSP_BUFFER_SIZE + 1];

    _init();
    size_t curRequestSize = aRequestSize;
    memcpy(curRequest, aRequest, aRequestSize);

    // check whether the request contains information about the RTP/RTCP UDP client ports (SETUP command)

    char* clientPortPtr = strstr(curRequest, "client_port");
    if (clientPortPtr != NULL)
    {
        char* tmpPtr = strstr(clientPortPtr, "\r\n");
        if (tmpPtr != NULL) 
        {
            tmpPtr[0] = '\0';
            char clientPort[RTCP_CLIENT_PORT_STR_MAX + 1];
            strcpy(clientPort, clientPortPtr);
            char* pCP = strstr(clientPort, "=");
            if (pCP != nullptr)
            {
                pCP++;
                char clientPort[1024];
                strcpy(clientPort, pCP);
                pCP = strstr(clientPort, "-");
                if (pCP != nullptr)
                {
                    pCP[0] = '\0';
                    _clientRTPPort  = atoi(clientPort);
                    _clientRTCPPort = _clientRTPPort + 1;
                }
            }
        }
    }
    // Read everything up to the first space as the command name
    bool parseSucceeded = false;
    unsigned i;
    for (i = 0; i < sizeof(cmdName) - 1 && i < curRequestSize; ++i) 
    {
        char c = curRequest[i];
        if (c == ' ' || c == '\t') 
        {
            parseSucceeded = true;
            break;
        }
        cmdName[i] = c;
    }
    cmdName[i] = '\0';
    if (false == parseSucceeded)
        return RC_ERR_RTSP_BAD_PARSING;

    // find out the command type
    if (strstr(cmdName, "OPTIONS") != NULL)
    {
        _rtspCmdType = RTSP_OPTIONS;
    }
    else if (strstr(cmdName, "DESCRIBE") != NULL)
    {
        _rtspCmdType = RTSP_DESCRIBE;
    }
    else if (strstr(cmdName, "SETUP") != NULL)
    {
        _rtspCmdType = RTSP_SETUP;
        // check whether the request contains transport information (UDP or TCP)
        _tcpTransport = (strstr(curRequest, "RTP/AVP/TCP") != NULL) ? true : false;
    }
    else if (strstr(cmdName, "PLAY") != NULL)
    {
        _rtspCmdType = RTSP_PLAY;
    }
    else if (strstr(cmdName,"TEARDOWN") != NULL)
    {
        _rtspCmdType = RTSP_TEARDOWN;
    }

    // Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
    size_t j = i + 1;
    while (j < curRequestSize && (curRequest[j] == ' ' || curRequest[j] == '\t'))
    {
        ++j; // skip over any additional white space
    }
    for (; (int)j < (int)(curRequestSize - 8); ++j) 
    {
        constexpr size_t RTSP_STR_LEN = 6;
        if (strncmp("rtsp:/", curRequest + j,  RTSP_STR_LEN) == 0 ||
            strncmp("RTSP:/", curRequest + j,  RTSP_STR_LEN) == 0)
        {
            j += 6;
            if (curRequest[j] == '/') 
            {
                // This is a "rtsp://" URL; skip over the host:port part that follows:
                ++j;
                size_t uidx = 0;
                while (j < curRequestSize) 
                { 
                    //until '/' or ' '
                    if (curRequest[j] == '/' || curRequest[j] == ' ')
                    {
                        break ;
                    }
                    // extract the host:port part of the URL here
                    _URLHostPort[uidx] = curRequest[j];
                    uidx++;
                    ++j;
                }
            } 
            else
            {
                --j;
            }
            i = j;
            break;
        }
    }

    // Look for the URL suffix (before the following "RTSP/"):
    parseSucceeded = false;
    for (size_t k = i + 1; (int)k < (int)(curRequestSize - 5); ++k) 
    {
        if (strncmp("RTSP/", curRequest, 5) == 0)
        {
            while (--k >= i && curRequest[k] == ' ');
            size_t k1 = k;
            while (k1 > i && curRequest[k1] != '/')
            {
                --k1;
            }
            if (k - k1 + 1 > sizeof(_URLSuffix))
            {
                return RC_ERR_RTSP_BAD_URL_SUFFIX;
            }
            unsigned n = 0, k2 = k1+1;

            while (k2 <= k)
            {
                _URLSuffix[n++] = curRequest[k2++];
            }
            _URLSuffix[n] = '\0';

            if (k1 - i > sizeof(_URLPreSuffix))
            {
                return RC_ERR_RTSP_BAD_URL_PRESUFFIX;
            }
            n = 0;
            k2 = i + 1;
            while (k2 <= k1 - 1)
            {
                _URLPreSuffix[n++] = curRequest[k2++];
            }
            _URLPreSuffix[n] = '\0';
            i = k + 7; 
            parseSucceeded = true;
            break;
        }
    }
    if (parseSucceeded == false)
    {
        return RC_ERR_RTSP_BAD_PARSING;
    }

    // Look for "CSeq:", skip whitespace, then read everything up to the next \r or \n as 'CSeq':
    parseSucceeded = false;
    for (j = i; (int)j < (int)(curRequestSize - 5); ++j) 
    {
        if (strncmp("CSeq:", curRequest + j, 5) == 0)
        {
            j += 5;
            while (j < curRequestSize && (curRequest[j] ==  ' ' || curRequest[j] == '\t'))
            {
                ++j;
            }
            unsigned n;
            for (n = 0; n < sizeof(_cmdSeq) - 1 && j < curRequestSize; ++n,++j) 
            {
                char c = curRequest[j];
                if (c == '\r' || c == '\n') 
                {
                    parseSucceeded = true;
                    break;
                }
                _cmdSeq[n] = c;
            }
            _cmdSeq[n] = '\0';
            break;
        }
    }
    if (!parseSucceeded)
    {
        return RC_ERR_RTSP_BAD_PARSING;
    }
    // Also: Look for "Content-Length:" (optional)
    for (j = i; (int)j < (int)(curRequestSize - 15); ++j) 
    {
        if (strncmp("Content-Length:", curRequest + j, sizeof("Content-Length:")) == 0 ||
            strncmp("Content-Length:", curRequest + j, sizeof("Content-length:")) == 0)
        {
            j += 15;
            while (j < curRequestSize && (curRequest[j] ==  ' ' || curRequest[j] == '\t'))
            {
                ++j;
            }
            unsigned num;
            if (sscanf(&curRequest[j], "%u", &num) == 1)
            {
                _сontentLength = num;
            }
        }
    }
    return RC_SUCCESS;
};

RetCode RTSPSession::Handle_RtspRequest(RTSP_Code& rtsp_code,
                                    char const* aRequest,
                                    const uint32_t aRequestSize)
{
    RetCode ret = _parseRtspRequest(aRequest,aRequestSize);
    if (ret == RC_SUCCESS)
    {
        switch (_rtspCmdType)
        {
            case RTSP_OPTIONS:
            {
                _handle_RtspOPTION();
                break;
            };
            case RTSP_DESCRIBE:
            {
                _handle_RtspDESCRIBE();
                break;
            };
            case RTSP_SETUP:
            {
                _handle_RtspSETUP();
                break;
            };
            case RTSP_PLAY:
            {
                _handle_RtspPLAY();
                break;
            };
            default: {};
        };
    };
    rtsp_code = _rtspCmdType;
    return ret;
};

RetCode RTSPSession::_handle_RtspOPTION()
{
    char   response[1024];

    RetCode ret = RC_ERR;
    snprintf(response, sizeof(response),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n", _cmdSeq);

    if (-1 == send(_rtspClient, response, sizeof(response), 0))
    {
        return RC_ERR_BAD_TRANSMIT_TO_SOCKET;
    }
    else
    {
        ret = RC_SUCCESS;
    }
    return ret;
}

RetCode RTSPSession::_handle_RtspDESCRIBE()
{
    constexpr size_t RESPONSE_SIZE = 1024;
    char   response[RESPONSE_SIZE + 1];
    constexpr size_t SPD_SIZE = 1024;
    char   SDPBuf[SPD_SIZE + 1];
    constexpr size_t URL_BUF = 1024;
    char   URLBuf[URL_BUF + 1];

    // check whether we know a stream with the URL which is requested
    _streamID = -1;        // invalid URL
    if ((strcmp(_URLPreSuffix, "mjpeg") == 0) &&
        (strcmp(_URLSuffix, "1") == 0))
    {
        _streamID = 0;
    }
    else if ((strcmp(_URLPreSuffix, "mjpeg") == 0) &&
        (strcmp(_URLSuffix, "2") == 0))
    {
        _streamID = 1;
    }
    if (_streamID == -1)
    {
        // Stream not available
        snprintf(response,
            RESPONSE_SIZE,
            "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
            _cmdSeq, 
            _dateHeader());

        if (-1 == send(_rtspClient, response, RESPONSE_SIZE, 0))
        {
            return RC_ERR_BAD_TRANSMIT_TO_SOCKET;
        }
        return RC_ERR_STREAM_NOT_FOUND;
    };

    // simulate DESCRIBE server response
    char OBuf[256];
    char * ColonPtr;
    strcpy(OBuf, _URLHostPort);
    ColonPtr = strstr(OBuf, ":");
    if (ColonPtr != NULL)
        ColonPtr[0] = 0x00;

    snprintf(SDPBuf,sizeof(SDPBuf),
        "v=0\r\n"
        "o=- %d 1 IN IP4 %s\r\n"           
        "s=\r\n"
        "t=0 0\r\n"                                            // start / stop - 0 -> unbounded and permanent session
        "m=video 0 RTP/AVP 26\r\n"                             // currently we just handle UDP sessions
        "c=IN IP4 0.0.0.0\r\n",
        rand(),
        OBuf);
    char StreamName[64];
    switch (_streamID)
    {
        case 0: strcpy(StreamName, "mjpeg/1"); break;
        case 1: strcpy(StreamName, "mjpeg/2"); break;
    };
    snprintf(URLBuf, sizeof(URLBuf),
        "rtsp://%s/%s",
        _URLHostPort,
        StreamName);
    snprintf(response,sizeof(response),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s\r\n"
        "Content-Base: %s/\r\n"
        "Content-Type: application/sdp\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        _cmdSeq,
        _dateHeader(),
        URLBuf,
        strlen(SDPBuf),
        SDPBuf);

    RetCode ret = RC_ERR;
    if (-1 == send(_rtspClient, response, RESPONSE_SIZE, 0))
    {
        return RC_ERR_BAD_TRANSMIT_TO_SOCKET;
    }
    else
    {
        ret = RC_SUCCESS;
    }
    return ret;
}

RetCode RTSPSession::_handle_RtspSETUP()
{
    constexpr size_t RESPONSE_SIZE = 1024;
    char response[RESPONSE_SIZE + 1];
    constexpr size_t TRANSPORT_SIZE = 255;
    char transport[TRANSPORT_SIZE + 1];

    // init RTP streamer transport type (UDP or TCP) and ports for UDP transport
    _streamer->InitTransport(_clientRTPPort,
                _clientRTCPPort,
                _tcpTransport);

    // simulate SETUP server response
    if (_tcpTransport == true)
    {
        snprintf(transport,
            TRANSPORT_SIZE,
            "RTP/AVP/TCP;unicast;interleaved=0-1");
    }
    else
    {
        snprintf(transport,
            TRANSPORT_SIZE,
            "RTP/AVP;unicast;destination=127.0.0.1;source=127.0.0.1;client_port=%i-%i;server_port=%i-%i",
            _clientRTPPort,
            _clientRTCPPort,
            _streamer->getRtpServerPort(),
            _streamer->getRtcpServerPort());
    }
    snprintf(response,
        RESPONSE_SIZE,
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s\r\n"
        "Transport: %s\r\n"
        "Session: %i\r\n\r\n",
        _cmdSeq,
        _dateHeader(),
        transport,
        _rtspSessionID);

    RetCode ret = RC_ERR;
    if (-1 == send(_rtspClient, response, RESPONSE_SIZE, 0))
    {
        return RC_ERR_BAD_TRANSMIT_TO_SOCKET;
    }
    else
    {
        ret = RC_SUCCESS;
    }
    return ret;
}

RetCode RTSPSession::_handle_RtspPLAY()
{
    constexpr size_t RESPONSE_SIZE = 1024;
    char   response[RESPONSE_SIZE + 1];

    // simulate SETUP server response
    snprintf(response,
        RESPONSE_SIZE,
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %i\r\n"
        "RTP-Info: url=rtsp://127.0.0.1:8554/mjpeg/1/track1\r\n\r\n",
        _cmdSeq,
        _dateHeader(),
        _rtspSessionID);

    RetCode ret = RC_ERR;
    if (-1 == send(_rtspClient, response, RESPONSE_SIZE, 0))
    {
        return RC_ERR_BAD_TRANSMIT_TO_SOCKET;
    }
    else
    {
        ret = RC_SUCCESS;
    }
    return ret;
}

const char* RTSPSession::_dateHeader() 
{
    constexpr size_t BUFF_SIZE = 256;
    char buf[BUFF_SIZE + 1];    
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT", gmtime(&tt));
    return buf;
}

int RTSPSession::getStreamID() const
{
    return _streamID;
};
