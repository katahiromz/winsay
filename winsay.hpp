// winsay.hpp --- Windows says things.
// Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software.
///////////////////////////////////////////////////////////////////////////////

#ifndef WINSAY_HPP_
#define WINSAY_HPP_         8   // 0.8

#ifndef _INC_WINDOWS
    #include <windows.h>    // for Windows API
#endif
#ifndef _OBJBASE_H_
    #include <objbase.h>
#endif

///////////////////////////////////////////////////////////////////////////////

enum WINSAY_MODE
{
    WINSAY_SAY,
    WINSAY_OUTPUT,
    WINSAY_GETVOICES,
    WINSAY_GETFILEFORMATS,
    WINSAY_GETBITRATES,
    WINSAY_GETCHANNELS
};

///////////////////////////////////////////////////////////////////////////////
// WINSAY_DATA

#ifdef __cplusplus
    #include <string>       // for std::string
    struct WINSAY_DATA
    {
        std::string input_file;
        std::string output_file;
        std::string voice;
        std::string text;
        std::string file_format;
        WINSAY_MODE mode;
        int bit_rate;
        int channels;

        WINSAY_DATA()
        {
            clear();
        }

        void clear()
        {
            input_file = "-";
            output_file.clear();
            voice.clear();
            text.clear();
            file_format = ".wav";
            mode = WINSAY_SAY;
            bit_rate = 44100;
            channels = 2;
        }
    };
#else
    typedef struct WINSAY_DATA
    {
        char dummy;
    } WINSAY_DATA;
#endif

///////////////////////////////////////////////////////////////////////////////
// C++ functions and classes

#ifdef __cplusplus
extern "C" {
#endif

// create WINSAY_DATA structure
WINSAY_DATA *winsay_create(void);
// show version info
void winsay_show_version(void);
// show help
void winsay_show_help(void);
// parse the command line
int winsay_command_line(WINSAY_DATA *data, int argc, char **argv);
// make windows say
int winsay_say(WINSAY_DATA *data);
// destroy WINSAY_DATA structure
void winsay_destroy(WINSAY_DATA *data);

// automatically calls CoInitialize and CoUninitialize functions
class winsay_co_init
{
public:
    HRESULT m_hr;

    winsay_co_init() : m_hr(S_FALSE)
    {
        m_hr = CoInitialize(NULL);
    }

    ~winsay_co_init()
    {
        if (SUCCEEDED(m_hr))
        {
            CoUninitialize();
        }
        m_hr = S_FALSE;
    }
};

#ifdef __cplusplus
} // extern "C"
#endif

///////////////////////////////////////////////////////////////////////////////

#endif  // ndef WINSAY_HPP_
