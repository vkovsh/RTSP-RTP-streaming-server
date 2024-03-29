// MediaLAN 02/2013
// CRtspSession
// - parsing of RTSP requests and generation of RTSP responses

#pragma once

#include <Winsock2.h>
#include <windows.h>  
#include "Streamer.win.h"
#include "rtsp_cmd_codes.h"

#define RTSP_BUFFER_SIZE       10000    // for incoming requests, and outgoing responses
#define RTSP_PARAM_STRING_MAX  200  

class CRtspSession
{
public:
    CRtspSession(SOCKET aRtspClient, CStreamer * aStreamer);
    ~CRtspSession();

    RTSP_CMD_TYPES Handle_RtspRequest(char const * aRequest, unsigned aRequestSize);
    int            GetStreamID();

private:
    void Init();
    bool ParseRtspRequest(char const * aRequest, unsigned aRequestSize);
    char const * DateHeader();

    // RTSP request command handlers
    void Handle_RtspOPTION();
    void Handle_RtspDESCRIBE();
    void Handle_RtspSETUP(); 
    void Handle_RtspPLAY();

    // global session state parameters
    int            m_RtspSessionID;
    SOCKET         m_RtspClient;                              // RTSP socket of that session
    int            m_StreamID;                                // number of simulated stream of that session
    u_short        m_ClientRTPPort;                           // client port for UDP based RTP transport
    u_short        m_ClientRTCPPort;                          // client port for UDP based RTCP transport  
    bool           m_TcpTransport;                            // if Tcp based streaming was activated
    CStreamer    * m_Streamer;                                // the UDP or TCP streamer of that session

    // parameters of the last received RTSP request

    RTSP_CMD_TYPES m_RtspCmdType;                             // command type (if any) of the current request
    char           m_URLPreSuffix[RTSP_PARAM_STRING_MAX];     // stream name pre suffix 
    char           m_URLSuffix[RTSP_PARAM_STRING_MAX];        // stream name suffix
    char           m_CSeq[RTSP_PARAM_STRING_MAX];             // RTSP command sequence number
    char           m_URLHostPort[RTSP_BUFFER_SIZE];           // host:port part of the URL
    unsigned       m_ContentLength;                           // SDP string size
};