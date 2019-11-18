#pragma once

enum    RetCode
{
        RC_SUCCESS = 0,
        RC_ERR = 1,
        RC_ERR_RTSP_BAD_PARSING = 2,
        RC_ERR_RTSP_BAD_URL_SUFFIX = 3,
        RC_ERR_RTSP_BAD_URL_PRESUFFIX = 4,
        RC_ERR_BAD_TRANSMIT_TO_SOCKET = 5,
        RC_ERR_STREAM_NOT_FOUND = 6,
        RC_TOTAL = 7,
};