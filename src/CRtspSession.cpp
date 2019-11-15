// MediaLAN 02/2013
// CRtspSession
// - parsing of RTSP requests and generation of RTSP responses

#ifdef __linux__ 
# include "CRtspSession.lin.h"
#elif _WIN32
# include "CRtspSession.win.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "JPEGSamples.h"

RTSPSession::RTSPSession(int aRtspClient, CStreamer* aStreamer):
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
        char* tmpPtr = strstr(curRequest, "RTP/AVP/TCP");
        _tcpTransport = (tmpPtr != NULL) ? true : false;
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
            {   // This is a "rtsp://" URL; skip over the host:port part that follows:
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
            if (k - k1 + 1 > sizeof(m_URLSuffix)) return false;
            unsigned n = 0, k2 = k1+1;

            while (k2 <= k) m_URLSuffix[n++] = CurRequest[k2++];
            m_URLSuffix[n] = '\0';

            if (k1 - i > sizeof(m_URLPreSuffix)) return false;
            n = 0; k2 = i + 1;
            while (k2 <= k1 - 1) m_URLPreSuffix[n++] = CurRequest[k2++];
            m_URLPreSuffix[n] = '\0';
            i = k + 7; 
            parseSucceeded = true;
            break;
        }
    }
    if (!parseSucceeded) return false;

    // Look for "CSeq:", skip whitespace, then read everything up to the next \r or \n as 'CSeq':
    parseSucceeded = false;
    for (j = i; (int)j < (int)(CurRequestSize-5); ++j) 
    {
        if (CurRequest[j]   == 'C' && CurRequest[j+1] == 'S' && 
            CurRequest[j+2] == 'e' && CurRequest[j+3] == 'q' && 
            CurRequest[j+4] == ':') 
        {
            j += 5;
            while (j < CurRequestSize && (CurRequest[j] ==  ' ' || CurRequest[j] == '\t')) ++j;
            unsigned n;
            for (n = 0; n < sizeof(m_CSeq)-1 && j < CurRequestSize; ++n,++j) 
            {
                char c = CurRequest[j];
                if (c == '\r' || c == '\n') 
                {
                    parseSucceeded = true;
                    break;
                }
                m_CSeq[n] = c;
            }
            m_CSeq[n] = '\0';
            break;
        }
    }
    if (!parseSucceeded) return false;

    // Also: Look for "Content-Length:" (optional)
    for (j = i; (int)j < (int)(CurRequestSize-15); ++j) 
    {
        if (CurRequest[j]    == 'C'  && CurRequest[j+1]  == 'o'  && 
            CurRequest[j+2]  == 'n'  && CurRequest[j+3]  == 't'  && 
            CurRequest[j+4]  == 'e'  && CurRequest[j+5]  == 'n'  && 
            CurRequest[j+6]  == 't'  && CurRequest[j+7]  == '-'  && 
            (CurRequest[j+8] == 'L' || CurRequest[j+8]   == 'l') &&
            CurRequest[j+9]  == 'e'  && CurRequest[j+10] == 'n' && 
            CurRequest[j+11] == 'g' && CurRequest[j+12]  == 't' && 
            CurRequest[j+13] == 'h' && CurRequest[j+14] == ':') 
        {
            j += 15;
            while (j < CurRequestSize && (CurRequest[j] ==  ' ' || CurRequest[j] == '\t')) ++j;
            unsigned num;
            if (sscanf(&CurRequest[j], "%u", &num) == 1) m_ContentLength = num;
        }
    }
    return true;
};

RTSP_CMD_TYPES CRtspSession::Handle_RtspRequest(char const * aRequest, unsigned aRequestSize)
{
    if (ParseRtspRequest(aRequest,aRequestSize))
    {
        switch (m_RtspCmdType)
        {
            case RTSP_OPTIONS:  { Handle_RtspOPTION();   break; };
            case RTSP_DESCRIBE: { Handle_RtspDESCRIBE(); break; };
            case RTSP_SETUP:    { Handle_RtspSETUP();    break; };
            case RTSP_PLAY:     { Handle_RtspPLAY();     break; };
            default: {};
        };
    };
    return m_RtspCmdType;
};

void CRtspSession::Handle_RtspOPTION()
{
    char   Response[1024];

    _snprintf(Response,sizeof(Response),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n",m_CSeq);

    send(m_RtspClient,Response,strlen(Response),0);   
}

void CRtspSession::Handle_RtspDESCRIBE()
{
    char   Response[1024];
    char   SDPBuf[1024];
    char   URLBuf[1024];

    // check whether we know a stream with the URL which is requested
    m_StreamID = -1;        // invalid URL
    if ((strcmp(m_URLPreSuffix,"mjpeg") == 0) && (strcmp(m_URLSuffix,"1") == 0)) m_StreamID = 0; else
    if ((strcmp(m_URLPreSuffix,"mjpeg") == 0) && (strcmp(m_URLSuffix,"2") == 0)) m_StreamID = 1;
    if (m_StreamID == -1)
    {   // Stream not available
        _snprintf(Response,sizeof(Response),
            "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
            m_CSeq, 
            DateHeader());

        send(m_RtspClient,Response,strlen(Response),0);   
        return;
    };

    // simulate DESCRIBE server response
    char OBuf[256];
    char * ColonPtr;
    strcpy(OBuf,m_URLHostPort);
    ColonPtr = strstr(OBuf,":");
    if (ColonPtr != nullptr) ColonPtr[0] = 0x00;

    _snprintf(SDPBuf,sizeof(SDPBuf),
        "v=0\r\n"
        "o=- %d 1 IN IP4 %s\r\n"           
        "s=\r\n"
        "t=0 0\r\n"                                            // start / stop - 0 -> unbounded and permanent session
        "m=video 0 RTP/AVP 26\r\n"                             // currently we just handle UDP sessions
        "c=IN IP4 0.0.0.0\r\n",
        rand(),
        OBuf);
    char StreamName[64];
    switch (m_StreamID)
    {
        case 0: strcpy(StreamName,"mjpeg/1"); break;
        case 1: strcpy(StreamName,"mjpeg/2"); break;
    };
    _snprintf(URLBuf,sizeof(URLBuf),
        "rtsp://%s/%s",
        m_URLHostPort,
        StreamName);
    _snprintf(Response,sizeof(Response),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s\r\n"
        "Content-Base: %s/\r\n"
        "Content-Type: application/sdp\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        m_CSeq,
        DateHeader(),
        URLBuf,
        strlen(SDPBuf),
        SDPBuf);

    send(m_RtspClient,Response,strlen(Response),0);   
}

void CRtspSession::Handle_RtspSETUP()
{
    char Response[1024];
    char Transport[255];

    // init RTP streamer transport type (UDP or TCP) and ports for UDP transport
    m_Streamer->InitTransport(m_ClientRTPPort,m_ClientRTCPPort,m_TcpTransport);

    // simulate SETUP server response
    if (m_TcpTransport)
        _snprintf(Transport,sizeof(Transport),"RTP/AVP/TCP;unicast;interleaved=0-1");
    else
        _snprintf(Transport,sizeof(Transport),
            "RTP/AVP;unicast;destination=127.0.0.1;source=127.0.0.1;client_port=%i-%i;server_port=%i-%i",
            m_ClientRTPPort,
            m_ClientRTCPPort,
            m_Streamer->GetRtpServerPort(),
            m_Streamer->GetRtcpServerPort());
    _snprintf(Response,sizeof(Response),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s\r\n"
        "Transport: %s\r\n"
        "Session: %i\r\n\r\n",
        m_CSeq,
        DateHeader(),
        Transport,
        m_RtspSessionID);

    send(m_RtspClient,Response,strlen(Response),0);
}

void CRtspSession::Handle_RtspPLAY()
{
    char   Response[1024];

    // simulate SETUP server response
    _snprintf(Response,sizeof(Response),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %i\r\n"
        "RTP-Info: url=rtsp://127.0.0.1:8554/mjpeg/1/track1\r\n\r\n",
        m_CSeq,
        DateHeader(),
        m_RtspSessionID);

    send(m_RtspClient,Response,strlen(Response),0);
}

char const * CRtspSession::DateHeader() 
{
    char buf[200];    
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT", gmtime(&tt));
    return buf;
}

int CRtspSession::GetStreamID()
{
    return m_StreamID;
};
