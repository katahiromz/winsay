// winsay.cpp --- Windows says things.
// Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software.

#include <cstdio>       // standard C I/O
#include <string>       // for std::string
#include <vector>       // for std::vector
#ifdef USE_GETOPT_PORT
    #include "getopt.h" // for portable getopt_long
#else
    #include <getopt.h> // for GNU getopt_long
#endif

// TODO: rate, progress, bit-rate,
//       channels, quality, data-format

#include "WinVoice.hpp"
#include "MString.hpp"
#include "MTextToText.hpp"
#include <sphelper.h>

using std::printf;
using std::fprintf;
using std::exit;

// global variables
std::string g_input_file = "-";
std::string g_output_file;
std::string g_voice;
std::string g_text;
std::string g_file_format = ".wav";

// return value
enum RET
{
    RET_SUCCESS = 0,
    RET_INVALID_ARGUMENT
};

enum WINSAY_MODE
{
    WINSAY_SAY,
    WINSAY_OUTPUT,
    WINSAY_GETVOICES,
    WINSAY_GETFILEFORMATS
};
WINSAY_MODE g_mode = WINSAY_SAY;

// show version info
void show_version(void)
{
    printf("winsay version 0.4\n");
}

// show help
void show_help(void)
{
    printf("winsay -- Windows says things\n");
    printf("Usage: sample [options] string...\n");
    printf("\n");
    printf("Options:\n");
    printf("--help                  Show this help.\n");
    printf("--version               Show version info.\n");
    printf("string                  The text to speak.\n");
    printf("\n");
    printf("-f file                 \n");
    printf("--input-file=file       An input file to be spoken.\n");
    printf("                        If file is - or neither this parameter nor a message\n");
    printf("                        is specified, read from standard input.\n");
    printf("\n");
    printf("-o file                 \n");
    printf("--output-file=file      An output file.\n");
    printf("\n");
    printf("-v voice                \n");
    printf("--voice=voice           A voice to be used.\n");
    printf("--voice=?               List all available voices.\n");
    printf("\n");
    printf("--file-format=format    The format of the output file to write.\n");
    printf("--quality=quality       The audio converter quality (ignored).\n");
}

// option info for getopt_long
struct option opts[] =
{
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 0 },
    { "input-file", optional_argument, NULL, 'f' },
    { "output-file", required_argument, NULL, 'o' },
    { "voice", required_argument, NULL, 'v' },
    { "quality", required_argument, NULL, 0 },
    { "file-format", required_argument, NULL, 0 },
    { NULL, 0, NULL, 0 },
};

// external symbols for getopt_long
extern "C"
{
    extern char *optarg;
    extern int optind, opterr, optopt;
}

// parse the command line
int parse_command_line(int argc, char **argv)
{
    int opt, opt_index;
    std::string arg;

    opterr = 0;  /* NOTE: opterr == 1 is not compatible to getopt_port */

    while ((opt = getopt_long(argc, argv, "hv:i:o:", opts, &opt_index)) != -1)
    {
        switch (opt)
        {
        case 0:
            arg = opts[opt_index].name;
            if (arg == "version")
            {
                show_version();
                exit(EXIT_SUCCESS);
            }
            if (arg == "file-format")
            {
                g_file_format = optarg;
                if (strcmp(optarg, "?") == 0)
                {
                    g_mode = WINSAY_GETFILEFORMATS;
                }
            }
            if (arg == "quality")
            {
                // simply ignored
            }
            break;
        case 'h':
            show_help();
            exit(EXIT_SUCCESS);
            break;
        case 'f':
            if (optarg)
                g_input_file = optarg;
            else
                g_input_file = "-";
            break;
        case 'o':
            g_output_file = optarg;
            g_mode = WINSAY_OUTPUT;
            break;
        case 'v':
            if (strcmp(optarg, "?") == 0)
                g_mode = WINSAY_GETVOICES;
            g_voice = optarg;
            break;
        case '?':
        default:
            switch (optopt)
            {
            case 'f':
                if (!optarg)
                {
                    fprintf(stderr, "ERROR: option '-%c' requires a parameter.\n", optopt);
                }
                break;
            case 'o':
                if (!optarg)
                {
                    fprintf(stderr, "ERROR: option '-%c' requires a parameter.\n", optopt);
                }
                break;
            case 'v':
                if (optarg)
                {
                    g_voice = "?";
                    g_mode = WINSAY_GETVOICES;
                }
                else
                {
                    fprintf(stderr, "ERROR: option '-%c' requires a parameter.\n", optopt);
                }
                break;
            case 0:
                fprintf(stderr, "ERROR: invalid option.\n");
                break;
            default:
                fprintf(stderr, "ERROR: invalid option '-%c'.\n", optopt);
                break;
            }
            return RET_INVALID_ARGUMENT;
        }
    }

    for (int i = optind; i < argc; ++i)
    {
        g_text += ' ';
        g_text += argv[i];
    }

    switch (g_mode)
    {
    case WINSAY_SAY:
    case WINSAY_OUTPUT:
        if (g_text.empty())
        {
            FILE *fp;
            if (g_input_file != "-" && g_input_file.size())
            {
                fp = fopen(g_input_file.c_str(), "rb");
            }
            else
            {
                fp = stdin;
            }

            if (fp)
            {
                char buf[256];
                while (fgets(buf, ARRAYSIZE(buf), fp))
                {
                    g_text += buf;
                }

                if (fp != stdin)
                    fclose(fp);
            }
        }
        break;
    case WINSAY_GETVOICES:
        break;
    case WINSAY_GETFILEFORMATS:
        break;
    }

    mstr_trim(g_text);

    return RET_SUCCESS;
}

struct AUDIO_TOKEN
{
    std::wstring id;
};

std::wstring
GetKeyPathFromTokenID(HKEY& hKeyBase, const WCHAR *pszID)
{
    static const WCHAR *pszHKLM = L"HKEY_LOCAL_MACHINE\\";
    static const WCHAR *pszHKCU = L"HKEY_CURRENT_USER\\";
    std::wstring key_path;
    hKeyBase = NULL;
    if (memcmp(pszID, pszHKLM, lstrlenW(pszHKLM)) == 0)
    {
        hKeyBase = HKEY_LOCAL_MACHINE;
        key_path = &pszID[lstrlenW(pszHKLM)];
    }
    else if (memcmp(pszID, pszHKLM, lstrlenW(pszHKCU)) == 0)
    {
        hKeyBase = HKEY_CURRENT_USER;
        key_path = &pszID[lstrlenW(pszHKCU)];
    }
    key_path += L"\\Attributes";
    return key_path;
}

struct VOICE_TOKEN
{
    std::wstring id;
    std::wstring name;
    std::wstring full_name;
    std::wstring age;
    std::wstring gender;
    std::wstring language;
};

VOICE_TOKEN GetVoiceTokenInfo(LPCWSTR pszID, HKEY hSubKey)
{
    WCHAR szAge[MAX_PATH] = {};
    WCHAR szGender[MAX_PATH] = {};
    WCHAR szLanguage[MAX_PATH] = {};
    WCHAR szName[MAX_PATH] = {};
    DWORD cbValue;

    cbValue = sizeof(szAge);
    RegQueryValueExW(hSubKey, L"Age", NULL, NULL, LPBYTE(szAge), &cbValue);
    cbValue = sizeof(szGender);
    RegQueryValueExW(hSubKey, L"Gender", NULL, NULL, LPBYTE(szGender), &cbValue);
    cbValue = sizeof(szLanguage);
    RegQueryValueExW(hSubKey, L"Language", NULL, NULL, LPBYTE(szLanguage), &cbValue);
    cbValue = sizeof(szName);
    RegQueryValueExW(hSubKey, L"Name", NULL, NULL, LPBYTE(szName), &cbValue);

    std::wstring name = szName;

    static const WCHAR szMicrosoftSp[] = L"Microsoft ";
    INT cchMicrosoftSp = lstrlenW(szMicrosoftSp);
    if (name.size() > cchMicrosoftSp &&
        name.substr(0, cchMicrosoftSp) == szMicrosoftSp)
    {
        name.erase(0, cchMicrosoftSp);
    }

    static const WCHAR szSpDesktop[] = L" Desktop";
    INT cchSpDesktop = lstrlenW(szSpDesktop);
    if (name.size() > cchSpDesktop &&
        name.substr(name.size() - cchSpDesktop, cchSpDesktop) == szSpDesktop)
    {
        name = name.substr(0, name.size() - cchSpDesktop);
    }

    if (0)
    {
        printf("age: %ls\n", szAge);
        printf("gender: %ls\n", szGender);
        printf("language: %ls\n", szLanguage);
        printf("name: %ls\n", name.c_str());
    }

    VOICE_TOKEN token =
    {
        pszID,
        name,
        szName,
        szAge,
        szGender,
        szLanguage
    };
    return token;
}

bool
winsay_get_voices(const WCHAR *pszRequest, std::vector<VOICE_TOKEN>& tokens)
{
    // get voice category
    ISpObjectTokenCategory *pCategory = NULL;
    HRESULT hr = SpGetCategoryFromId(SPCAT_VOICES, &pCategory);
    if (SUCCEEDED(hr) && pCategory)
    {
        // get object tokens
        IEnumSpObjectTokens *pTokens = NULL;
        hr = pCategory->EnumTokens(pszRequest, NULL, &pTokens);
        if (SUCCEEDED(hr) && pTokens)
        {
            for (;;)
            {
                // get token
                ISpObjectToken *pToken = NULL;
                hr = pTokens->Next(1, &pToken, NULL);
                if (FAILED(hr) || !pToken)
                    break;

                // get token id
                LPWSTR pszID = NULL;
                pToken->GetId(&pszID);
                if (pszID)
                {
                    //printf("pszID: %ls\n", pszID);

                    // read voice info from registry
                    HKEY hKeyBase = NULL;
                    std::wstring key_path = GetKeyPathFromTokenID(hKeyBase, pszID);
                    if (hKeyBase)
                    {
                        //printf("%ls\n", key_path.c_str());
                        HKEY hSubKey = NULL;
                        RegOpenKeyExW(hKeyBase, key_path.c_str(), 0, KEY_READ, &hSubKey);
                        if (hSubKey)
                        {
                            VOICE_TOKEN token = GetVoiceTokenInfo(pszID, hSubKey);
                            tokens.push_back(token);

                            RegCloseKey(hSubKey);
                        }
                    }
                    CoTaskMemFree(pszID);
                    pszID = NULL;
                }
                pToken->Release();
                pToken = NULL;
            }
            pTokens->Release();
            pTokens = NULL;
        }
        pCategory->Release();
        pCategory = NULL;
    }

    return !tokens.empty();
}

// make windows say
int winsay(void)
{
    if (0)
    {
        printf("input-file: %s\n", g_input_file.c_str());
        printf("output-file: %s\n", g_output_file.c_str());
        printf("text: %s\n", g_text.c_str());
        printf("voice: %s\n", g_voice.c_str());
    }

    std::vector<VOICE_TOKEN> tokens;
    if (!winsay_get_voices(NULL, tokens))
    {
        fprintf(stderr, "ERROR: unable to enumerate voices.\n");
        return EXIT_FAILURE;
    }

    if (g_file_format == "?")
    {
        printf("wav      WAVE format\n");
        return EXIT_SUCCESS;
    }

    WinVoice voice;
    ISpObjectToken *pToken = NULL;
    ISpStream *pStream = NULL;

    if (g_voice == "?")
    {
        for (auto& token : tokens)
        {
            printf("%-20ls%ls\n", token.name.c_str(), token.language.c_str());
        }
        return EXIT_SUCCESS;
    }
    else if (g_voice.size())
    {
        for (auto& token : tokens)
        {
            MAnsiToWide wVoice(CP_ACP, g_voice.c_str());

            if (lstrcmpiW(wVoice.c_str(), token.name.c_str()) == 0 ||
                lstrcmpiW(wVoice.c_str(), token.full_name.c_str()) == 0)
            {
                ::CoCreateInstance(CLSID_SpObjectToken, NULL, CLSCTX_ALL,
                                   IID_ISpObjectToken, (void **)&pToken);
                if (pToken)
                    pToken->SetId(NULL, token.id.c_str(), FALSE);
            }
        }
    }

    if (pToken)
        voice.SetVoice(pToken);

    if (g_output_file.size())
    {
        if (g_file_format.size() && g_file_format[0] != '.')
        {
            g_file_format = "." + g_file_format;
        }

        std::string dotext;
        if (g_output_file.size() >= 4)
        {
            dotext = g_output_file.substr(g_output_file.size() - 4, 4);
        }
        CharLowerA(&dotext[0]);

        if (dotext != ".wav")
        {
            g_output_file += g_file_format;
        }

        ::CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_ALL,
                           IID_ISpStream, (void **)&pStream);
        if (pStream)
        {
            MAnsiToWide wOutputFile(CP_ACP, g_output_file.c_str());

            WAVEFORMATEX fmt;
            fmt.wFormatTag = WAVE_FORMAT_PCM;
            fmt.nChannels = 1; 
            fmt.wBitsPerSample = 16;
            fmt.nSamplesPerSec = 44100;
            fmt.nBlockAlign = 2; 
            fmt.nAvgBytesPerSec = 88200;
            fmt.cbSize = 0;

            GUID GUID_SPDFID_WaveFormatEx;
            IIDFromString(L"{C31ADBAE-527F-4ff5-A230-F62BB61FF70C}", &GUID_SPDFID_WaveFormatEx);

            pStream->BindToFile(wOutputFile.c_str(), SPFM_CREATE_ALWAYS,
                                &GUID_SPDFID_WaveFormatEx, &fmt, 0);
            if (voice.SpVoice())
                voice.SpVoice()->SetOutput(pStream, TRUE);
        }
    }
    else
    {
        if (voice.SpVoice())
            voice.SpVoice()->SetOutput(NULL, TRUE);
    }

    HRESULT hr = voice.Speak(g_text, false);
    //printf("HRESULT: %08lX\n", hr);

    if (pToken)
    {
        pToken->Release();
        pToken = NULL;
    }
    if (pStream)
    {
        pStream->Release();
        pStream = NULL;
    }

    return EXIT_SUCCESS;
}

// automatically calls CoInitialize and CoUninitialize functions
class CAutoCoInitialize
{
public:
    HRESULT m_hResult;
    CAutoCoInitialize() : m_hResult(S_FALSE)
    {
        m_hResult = CoInitialize(NULL);
    }
    ~CAutoCoInitialize()
    {
        CoUninitialize();
        m_hResult = S_FALSE;
    }
};

// the main function
int main(int argc, char **argv)
{
    CAutoCoInitialize co_init;

    if (int ret = parse_command_line(argc, argv))
        return EXIT_FAILURE;

    return winsay();
}
