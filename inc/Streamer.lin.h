// MediaLAN 02/2013
// CRtspSession
// - JPEG packetizer and UDP/TCP based streaming

#pragma once

#include <stdint.h>

#include "RetCode.h"

class Streamer
{
public:
    Streamer(int aClient);
    ~Streamer();
    
    RetCode InitTransport(uint16_t aRtpPort, uint16_t aRtcpPort, bool TCP);
    RetCode StreamImage(int StreamID);
    RetCode StreamVideo(int StreamID);

public:
    uint16_t    getRtpServerPort(void) const;
    uint16_t    getRtcpServerPort(void) const;

private:
    RetCode     SendRtpPacket(char* Jpeg, int JpegLen, int Chn);

    int         _rtpSocket;          // RTP socket for streaming RTP packets to client
    int         _rtcpSocket;         // RTCP socket for sending/receiving RTCP packages

    uint16_t    _rtpClientPort;      // RTP receiver port on client 
    uint16_t    _rtcpClientPort;     // RTCP receiver port on client
    uint16_t    _rtpServerPort;      // RTP sender port on server 
    uint16_t    _rtcpServerPort;     // RTCP sender port on server

    uint16_t    _sequenceNumber;
    uint32_t    _timestamp;
    int         _sendIdx;
    bool        _TCPTransport;
    int         _client;
};