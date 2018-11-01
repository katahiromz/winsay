// winsay.hpp --- Windows says things.
// Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software.

#ifndef WINSAY_HPP_
#define WINSAY_HPP_

#ifndef __cplusplus
    #error You have to use C++ compiler.
#endif

#include <string>       // for std::string

enum WINSAY_MODE
{
    WINSAY_SAY,
    WINSAY_OUTPUT,
    WINSAY_GETVOICES,
    WINSAY_GETFILEFORMATS,
    WINSAY_GETBITRATES,
    WINSAY_GETCHANNELS
};

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

// show version info
void winsay_show_version(void);
// show help
void winsay_show_help(void);
// parse the command line
int winsay_command_line(WINSAY_DATA& data, int argc, char **argv);
// make windows say
int winsay_main(WINSAY_DATA& data);

#endif  // ndef WINSAY_HPP_
