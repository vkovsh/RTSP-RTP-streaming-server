// MediaLAN 02/2013
// CRtspSession
// - JPEG packetizer and UDP/TCP based streaming

#pragma once

class CStreamer
{
public:
    CStreamer(int aClient);
    ~CStreamer();

    void        InitTransport(uint16_t aRtpPort, uint16_t aRtcpPort, bool TCP);
    void        StreamImage(int StreamID);

public:
    uint16_t    GetRtpServerPort(void) const;
    uint16_t    GetRtcpServerPort(void) const;

private:
    RetCode     SendRtpPacket(char* Jpeg, int JpegLen, int Chn);

    int         m_RtpSocket;          // RTP socket for streaming RTP packets to client
    int         m_RtcpSocket;         // RTCP socket for sending/receiving RTCP packages

    uint16_t    m_RtpClientPort;      // RTP receiver port on client 
    uint16_t    m_RtcpClientPort;     // RTCP receiver port on client
    uint16_t    m_RtpServerPort;      // RTP sender port on server 
    uint16_t    m_RtcpServerPort;     // RTCP sender port on server

    uint16_t    m_SequenceNumber;
    uint32_t    m_Timestamp;
    int         m_SendIdx;
    bool        m_TCPTransport;
    int         m_Client;
};