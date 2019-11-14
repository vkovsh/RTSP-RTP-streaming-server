#pragma once

// supported command types
enum RTSP_Code
{
    RTSP_UNDEFINED = -1,
    RTSP_OPTIONS = 0,
    RTSP_DESCRIBE = 1,
    RTSP_SETUP = 2,
    RTSP_PLAY = 3,
    RTSP_TEARDOWN = 4,
    RTSP_TOTAL = 5,
};