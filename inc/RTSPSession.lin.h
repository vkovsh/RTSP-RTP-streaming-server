// MediaLAN 02/2013
// CRtspSession
// - parsing of RTSP requests and generation of RTSP responses

#pragma once

#include <stdint.h> 
#include "Streamer.lin.h"
#include "rtsp_cmd_codes.h"
#include "RetCode.h"

#define RTSP_BUFFER_SIZE       10000    // for incoming requests, and outgoing responses
#define RTSP_PARAM_STRING_MAX  200
#define RTCP_CLIENT_PORT_STR_MAX 256  

class RTSPSession
{
public:
    RTSPSession(int aRtspClient, Streamer* aStreamer);
    ~RTSPSession(void);

    RetCode   Handle_RtspRequest(RTSP_Code& rtsp_code,
                                    char const* aRequest,
                                    const uint32_t aRequestSize);

public:
    int getStreamID(void) const;

private:
    RetCode _init();
    RetCode _parseRtspRequest(char const* aRequest, const size_t aRequestSize);
    char const* _dateHeader();

private:
    // RTSP request command handlers
    RetCode _handle_RtspOPTION();
    RetCode _handle_RtspDESCRIBE();
    RetCode _handle_RtspSETUP(); 
    RetCode _handle_RtspPLAY();

private:
    // global session state parameters
    int         _rtspSessionID;                         // session id
    int         _rtspClient;                            // RTSP socket of that session
    int         _streamID;                              // number of simulated stream of that session
    uint16_t    _clientRTPPort;                         // client port for UDP based RTP transport
    uint16_t    _clientRTCPPort;                        // client port for UDP based RTCP transport  
    bool        _tcpTransport;                          // if Tcp based streaming was activated
    Streamer*  _streamer;                              // the UDP or TCP streamer of that session

    // parameters of the last received RTSP request
    RTSP_Code   _rtspCmdType;                           // command type (if any) of the current request
    char        _URLPreSuffix[RTSP_PARAM_STRING_MAX];   // stream name pre suffix 
    char        _URLSuffix[RTSP_PARAM_STRING_MAX];      // stream name suffix
    char        _cmdSeq[RTSP_PARAM_STRING_MAX];         // RTSP command sequence number
    char        _URLHostPort[RTSP_BUFFER_SIZE];         // host:port part of the URL
    unsigned    _сontentLength;                         // SDP string size
};