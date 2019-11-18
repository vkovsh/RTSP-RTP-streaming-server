// MediaLAN 02/2013
// CRtspSession
// - JPEG packetizer and UDP/TCP based streaming

#ifdef __linux__
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/libc-compat.h>
# include "Streamer.lin.h"
#include <netinet/in.h>
#elif _WIN32
# include "RTSPSession.win.h"
#endif

#include "JPEGSamples.h"

#include <stdio.h>

Streamer::Streamer(int aClient):
    _client(aClient),
    _rtpServerPort(0),
    _rtcpServerPort(0),
    _rtpClientPort(0),
    _rtcpClientPort(0),
    _sequenceNumber(0),
    _timestamp(0),
    _sendIdx(0),
    _TCPTransport(false)
{};

Streamer::~Streamer()
{
    close(_rtpSocket);
    close(_rtcpSocket);
};

RetCode Streamer::SendRtpPacket(char* jpeg, int jpegLen, int chn)
{
    constexpr size_t KRtpHeaderSize = 12;   // size of the RTP header
    constexpr size_t KJpegHeaderSize = 8;   // size of the special JPEG payload header

    constexpr size_t RTP_SIZE = 2048;
    char rtpBuf[RTP_SIZE + 1];
    struct sockaddr_in recvAddr;

    socklen_t recvLen = sizeof(recvAddr);
    int rtpPacketSize = jpegLen + KRtpHeaderSize + KJpegHeaderSize;

    // get client address for UDP transport
    getpeername(_client, (struct sockaddr*)&recvAddr, &recvLen);
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(_rtpClientPort);

    memset(rtpBuf, 0, RTP_SIZE);
    // Prepare the first 4 byte of the packet. This is the Rtp over Rtsp header in case of TCP based transport
    rtpBuf[0]  = '$';        // magic number
    rtpBuf[1]  = 0;          // number of multiplexed subchannel on RTPS connection - here the RTP channel
    rtpBuf[2]  = (rtpPacketSize & 0x0000FF00) >> 8;
    rtpBuf[3]  = (rtpPacketSize & 0x000000FF);
    // Prepare the 12 byte RTP header
    rtpBuf[4]  = 0x80;                               // RTP version
    rtpBuf[5]  = 0x9a;                               // JPEG payload (26) and marker bit
    rtpBuf[7]  = _sequenceNumber & 0x0FF;           // each packet is counted with a sequence counter
    rtpBuf[6]  = _sequenceNumber >> 8;
    rtpBuf[8]  = (_timestamp & 0xFF000000) >> 24;   // each image gets a timestamp
    rtpBuf[9]  = (_timestamp & 0x00FF0000) >> 16;
    rtpBuf[10] = (_timestamp & 0x0000FF00) >> 8;
    rtpBuf[11] = (_timestamp & 0x000000FF);
    rtpBuf[12] = 0x13;                               // 4 byte SSRC (sychronization source identifier)
    rtpBuf[13] = 0xf9;                               // we just an arbitrary number here to keep it simple
    rtpBuf[14] = 0x7e;
    rtpBuf[15] = 0x67;
    // Prepare the 8 byte payload JPEG header
    rtpBuf[16] = 0x00;                               // type specific
    rtpBuf[17] = 0x00;                               // 3 byte fragmentation offset for fragmented images
    rtpBuf[18] = 0x00;
    rtpBuf[19] = 0x00;
    rtpBuf[20] = 0x01;                               // type
    rtpBuf[21] = 0x5e;                               // quality scale factor
    if (chn == 0)
    {
        rtpBuf[22] = 0x06;                           // width  / 8 -> 48 pixel
        rtpBuf[23] = 0x04;                           // height / 8 -> 32 pixel
    }
    else
    {
        rtpBuf[22] = 0x08;                           // width  / 8 -> 64 pixel
        rtpBuf[23] = 0x06;                           // height / 8 -> 48 pixel
    };
    // append the JPEG scan data to the RTP buffer
    memcpy(&rtpBuf[24], jpeg, jpegLen);
    
    _sequenceNumber++;                              // prepare the packet counter for the next packet
    _timestamp += 3600;                             // fixed timestamp increment for a frame rate of 25fps

    if (_TCPTransport == true) // RTP over RTSP - we send the buffer + 4 byte additional header
    {
        send(_client, rtpBuf, rtpPacketSize + 4,0);
    }
    else
    {                // UDP - we send just the buffer by skipping the 4 byte RTP over RTSP header
        sendto(_rtpSocket,
                &rtpBuf[4],
                rtpPacketSize,
                0,
                (sockaddr *)&recvAddr,
                sizeof(recvAddr));
    }
    return RC_SUCCESS;
};

RetCode Streamer::InitTransport(uint16_t aRtpPort, uint16_t aRtcpPort, bool TCP)
{
    sockaddr_in server;  

    _rtpClientPort  = aRtpPort;
    _rtcpClientPort = aRtcpPort;
    _TCPTransport   = TCP;

    if (_TCPTransport == false)
    {
        // allocate port pairs for RTP/RTCP ports in UDP transport mode
        server.sin_family = AF_INET;   
        server.sin_addr.s_addr = INADDR_ANY;   
        for (uint16_t P = 6970; P < 0xFFFE ; P += 2)
        {
            _rtpSocket = socket(AF_INET, SOCK_DGRAM, 0);                     
            server.sin_port = htons(P);
            if (bind(_rtpSocket, (sockaddr*)&server, sizeof(server)) == 0)
            {
                // Rtp socket was bound successfully. Lets try to bind the consecutive Rtsp socket
                _rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0);
                server.sin_port = htons(P + 1);
                if (bind(_rtcpSocket, (sockaddr*)&server, sizeof(server)) == 0) 
                {
                    _rtpServerPort  = P;
                    _rtcpServerPort = P+1;
                    break; 
                }
                else
                {
                    close(_rtpSocket);
                    close(_rtcpSocket);
                };
            }
            else close(_rtpSocket);
        };
    };
    return RC_SUCCESS;
};

uint16_t    Streamer::getRtpServerPort() const
{
    return _rtpServerPort;
};

uint16_t    Streamer::getRtcpServerPort() const
{
    return _rtcpServerPort;
};

RetCode     Streamer::StreamImage(int StreamID)
{
    char*   Samples1[2] = {JpegScanDataCh1A, JpegScanDataCh1B};
    char*   Samples2[2] = { JpegScanDataCh2A, JpegScanDataCh2B };
    char**  JpegScanData;
    int     JpegScanDataLen;

    switch (StreamID > 0)
    {
        case 0:
        {
            JpegScanData = &Samples1[0]; 
            JpegScanDataLen = KJpegCh1ScanDataLen;
            break;
        };
        case 1: 
        {
            JpegScanData = &Samples2[0]; 
            JpegScanDataLen = KJpegCh2ScanDataLen;
            break;
        };
    };

    SendRtpPacket(JpegScanData[_sendIdx], JpegScanDataLen, StreamID);
    _sendIdx++;
    if (_sendIdx > 1)
        _sendIdx = 0;
    return RC_SUCCESS;

