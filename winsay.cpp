// winsay.cpp --- Windows says things.
// Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software.

#include <cstdio>       // standard C I/O
#include <vector>       // for std::vector
#ifdef USE_GETOPT_PORT
    #include "getopt.h" // for portable getopt_long
#else
    #include <getopt.h> // for GNU getopt_long
#endif

#include "WinVoice.hpp"
#include "MString.hpp"
#include "MTextToText.hpp"
#include <sphelper.h>
#include "winsay.hpp"

// TODO: rate, progress, bit-rate, mp3,
//       channels, quality, data-format

using std::printf;
using std::fprintf;
using std::exit;

// return value
enum RET
{
    RET_SUCCESS = 0,
    RET_INVALID_ARGUMENT
};

// show version info
void winsay_show_version(void)
{
    printf("winsay version 0.6 by katahiromz\n");
}

// show help
void winsay_show_help(void)
{
    printf("winsay -- Windows says things\n");
    printf("Usage: winsay [options] string...\n");
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
static struct option winsay_opts[] =
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
int winsay_command_line(WINSAY_DATA& data, int argc, char **argv)
{
    int opt, opt_index;
    std::string arg;

    opterr = 0;  /* NOTE: opterr == 1 is not compatible to getopt_port */

    data.mode = WINSAY_SAY;

    // for each command line option
    while ((opt = getopt_long(argc, argv, "hv:i:o:", winsay_opts, &opt_index)) != -1)
    {
        switch (opt)
        {
        case 0:     // no short option
            arg = winsay_opts[opt_index].name;
            if (arg == "version")
            {
                winsay_show_version();
                exit(EXIT_SUCCESS);
            }
            if (arg == "file-format")
            {
                data.file_format = optarg;
                if (strcmp(optarg, "?") == 0)
                {
                    data.mode = WINSAY_GETFILEFORMATS;
                }
            }
            if (arg == "quality")
            {
                // simply ignored
            }
            break;
        case 'h':
            winsay_show_help();
            exit(EXIT_SUCCESS);
            break;
        case 'f':
            if (optarg)
                data.input_file = optarg;
            else
                data.input_file = "-";
            break;
        case 'o':
            data.output_file = optarg;
            data.mode = WINSAY_OUTPUT;
            break;
        case 'v':
            if (strcmp(optarg, "?") == 0)
                data.mode = WINSAY_GETVOICES;
            data.voice = optarg;
            break;
        case '?':
        default:
            // error
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
                    data.voice = "?";
                    data.mode = WINSAY_GETVOICES;
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
        data.text += ' ';
        data.text += argv[i];
    }

    switch (data.mode)
    {
    case WINSAY_SAY:
    case WINSAY_OUTPUT:
        if (data.text.empty())
        {
            // no text. input now
            FILE *fp;
            if (data.input_file != "-" && data.input_file.size())
            {
                fp = fopen(data.input_file.c_str(), "rb");
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
                    data.text += buf;
                }

                if (fp != stdin)
                    fclose(fp);
            }
        }
        break;
    case WINSAY_GETVOICES:
    case WINSAY_GETFILEFORMATS:
        break;
    }

    mstr_trim(data.text);

    return RET_SUCCESS;
}

struct AUDIO_TOKEN
{
    std::wstring id;
};

static std::wstring
winsay_get_reg_path_from_id(HKEY& hKeyBase, const WCHAR *pszID)
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

static VOICE_TOKEN
winsay_get_voice_token_info(LPCWSTR pszID, HKEY hSubKey)
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
    size_t cchMicrosoftSp = wcslen(szMicrosoftSp);
    if (name.size() > cchMicrosoftSp &&
        name.substr(0, cchMicrosoftSp) == szMicrosoftSp)
    {
        name.erase(0, cchMicrosoftSp);
    }

    static const WCHAR szSpDesktop[] = L" Desktop";
    size_t cchSpDesktop = wcslen(szSpDesktop);
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

static bool
winsay_get_voices(const WCHAR *pszRequest,
                  std::vector<VOICE_TOKEN>& tokens)
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
            // for each token
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
                    std::wstring key_path = winsay_get_reg_path_from_id(hKeyBase, pszID);
                    if (hKeyBase)
                    {
                        //printf("%ls\n", key_path.c_str());
                        HKEY hSubKey = NULL;
                        RegOpenKeyExW(hKeyBase, key_path.c_str(), 0, KEY_READ, &hSubKey);
                        if (hSubKey)
                        {
                            VOICE_TOKEN token = winsay_get_voice_token_info(pszID, hSubKey);
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
int winsay_main(WINSAY_DATA& data)
{
    if (0)
    {
        printf("input-file: %s\n", data.input_file.c_str());
        printf("output-file: %s\n", data.output_file.c_str());
        printf("text: %s\n", data.text.c_str());
        printf("voice: %s\n", data.voice.c_str());
    }

    // get available voices
    std::vector<VOICE_TOKEN> voice_tokens;
    if (!winsay_get_voices(NULL, voice_tokens))
    {
        fprintf(stderr, "ERROR: unable to enumerate voices.\n");
        return EXIT_FAILURE;
    }

    if (data.file_format == "?")
    {
        // dump available file formats
        printf("wav      WAVE format\n");
        return EXIT_SUCCESS;
    }

    // the working objects
    WinVoice voice;
    ISpObjectToken *pVoiceToken = NULL;
    ISpStream *pStream = NULL;

    if (data.voice == "?")
    {
        // dump voices
        for (size_t i = 0; i < voice_tokens.size(); ++i)
        {
            VOICE_TOKEN& token = voice_tokens[i];
            WCHAR *endptr;
            WORD wLangID = (WORD)wcstoul(token.language.c_str(), &endptr, 16);
            if (*endptr == 0)
            {
                LCID lcid = MAKELCID(wLangID, SORT_DEFAULT);
                char szData[64] = {};
                GetLocaleInfoA(lcid, LOCALE_SISO639LANGNAME, szData, ARRAYSIZE(szData));
                printf("%-20ls%s\n", token.name.c_str(), szData);
            }
        }
        return EXIT_SUCCESS;
    }

    if (data.voice.size())
    {
        // select a voice
        for (size_t i = 0; i < voice_tokens.size(); ++i)
        {
            VOICE_TOKEN& token = voice_tokens[i];
            MAnsiToWide wVoice(CP_ACP, data.voice.c_str());

            if (lstrcmpiW(wVoice.c_str(), token.name.c_str()) == 0 ||
                lstrcmpiW(wVoice.c_str(), token.full_name.c_str()) == 0)
            {
                ::CoCreateInstance(CLSID_SpObjectToken, NULL, CLSCTX_ALL,
                                   IID_ISpObjectToken, (void **)&pVoiceToken);
                if (pVoiceToken)
                    pVoiceToken->SetId(NULL, token.id.c_str(), FALSE);
            }
        }
    }

    // set the voice
    if (pVoiceToken)
        voice.SetVoice(pVoiceToken);

    // take care of output file
    if (data.output_file.size())
    {
        // add dot
        if (data.file_format.size() && data.file_format[0] != '.')
        {
            data.file_format = "." + data.file_format;
        }

        // get last 4 characters
        std::string fourchars;
        if (data.output_file.size() >= 4)
        {
            fourchars = data.output_file.substr(data.output_file.size() - 4, 4);
        }
        CharLowerA(&fourchars[0]);

        if (fourchars != ".wav")
        {
            data.output_file += data.file_format;
        }

        ::CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_ALL,
                           IID_ISpStream, (void **)&pStream);
        if (pStream)
        {
            MAnsiToWide wOutputFile(CP_ACP, data.output_file.c_str());

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

    // speak now
    voice.Speak(data.text, false);

    // clean up
    if (pVoiceToken)
    {
        pVoiceToken->Release();
        pVoiceToken = NULL;
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
    HRESULT m_hr;
    CAutoCoInitialize() : m_hr(S_FALSE)
    {
        m_hr = CoInitialize(NULL);
    }
    ~CAutoCoInitialize()
    {
        CoUninitialize();
        m_hr = S_FALSE;
    }
};

// the main function
int main(int argc, char **argv)
{
    CAutoCoInitialize co_init;
    WINSAY_DATA data;

    if (0 != winsay_command_line(data, argc, argv))
        return EXIT_FAILURE;

    return winsay_main(data);
}
