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

// TODO: output, rate, progress, output-file, bit-rate,
//       channels, quality

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

// return value
enum RET
{
    RET_SUCCESS = 0,
    RET_INVALID_ARGUMENT
};

enum WINSAY_MODE
{
    WINSAY_NOTHING,
    WINSAY_SAY,
    WINSAY_GETVOICES,
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
    printf("--help              Show this help.\n");
    printf("--version           Show version info.\n");
    printf("string              The text to speak.\n");
    printf("\n");
    printf("-f file             \n");
    printf("--input-file=file   An input file to be spoken.\n");
    printf("                    If file is - or neither this parameter nor a message\n");
    printf("                    is specified, read from standard input.\n");
    printf("\n");
    printf("-o file             \n");
    printf("--output-file=file  An output file\n");
    printf("\n");
    printf("-v voice            \n");
    printf("--voice=voice       A voice to be used\n");
    printf("\n");
    printf("--quality=quality   The audio converter quality (ignored)\n");
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
    bool any_options = false;

    opterr = 0;  /* NOTE: opterr == 1 is not compatible to getopt_port */

    while ((opt = getopt_long(argc, argv, "hv:i:o:", opts, &opt_index)) != -1)
    {
        any_options = true;
        switch (opt)
        {
        case 0:
            arg = opts[opt_index].name;
            if (arg == "version")
            {
                show_version();
                exit(EXIT_SUCCESS);
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

    if (g_mode == WINSAY_SAY)
    {
        if (g_text.empty())
        {
            if (g_input_file != "-" && g_input_file.size())
            {
                if (FILE *fp = fopen(g_input_file.c_str(), "rb"))
                {
                    char buf[256];
                    while (fgets(buf, ARRAYSIZE(buf), fp))
                    {
                        g_text += buf;
                    }
                    fclose(fp);
                }
            }
        }

        if (g_text.empty() || g_input_file == "-" || g_input_file.empty())
        {
            char buf[256];
            while (fgets(buf, ARRAYSIZE(buf), stdin))
            {
                g_text += buf;
            }
        }
    }

    mstr_trim(g_text);

    return RET_SUCCESS;
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

bool
winsay_get_voices(const WCHAR *pszRequest, std::vector<VOICE_TOKEN>& tokens)
{
    ISpObjectTokenCategory *pCategory = NULL;
    HRESULT hr = SpGetCategoryFromId(SPCAT_VOICES, &pCategory);
    if (SUCCEEDED(hr) && pCategory)
    {
        IEnumSpObjectTokens *pTokens = NULL;
        hr = pCategory->EnumTokens(pszRequest, NULL, &pTokens);
        if (SUCCEEDED(hr) && pTokens)
        {
            ISpObjectToken *pToken = NULL;
            for (;;)
            {
                pToken = NULL;
                hr = pTokens->Next(1, &pToken, NULL);
                if (FAILED(hr) || !pToken)
                    break;

                LPWSTR pszID = NULL;
                pToken->GetId(&pszID);
                if (pszID)
                {
                    //printf("pszID: %ls\n", pszID);
                    static const WCHAR *pszHKLM = L"HKEY_LOCAL_MACHINE\\";
                    static const WCHAR *pszHKCU = L"HKEY_CURRENT_USER\\";
                    std::wstring key_path;
                    HKEY hKey = NULL;
                    if (memcmp(pszID, pszHKLM, lstrlenW(pszHKLM)) == 0)
                    {
                        hKey = HKEY_LOCAL_MACHINE;
                        key_path = &pszID[lstrlenW(pszHKLM)];
                    }
                    else if (memcmp(pszID, pszHKLM, lstrlenW(pszHKCU)) == 0)
                    {
                        hKey = HKEY_CURRENT_USER;
                        key_path = &pszID[lstrlenW(pszHKCU)];
                    }
                    if (hKey)
                    {
                        key_path += L"\\Attributes";
                        //printf("%ls\n", key_path.c_str());

                        HKEY hSubKey = NULL;
                        RegOpenKeyExW(hKey, key_path.c_str(), 0, KEY_READ, &hSubKey);
                        if (hSubKey)
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

    WinVoice voice;
    ISpObjectToken *pToken = NULL;

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

    HRESULT hr = voice.Speak(g_text, false);
    //printf("HRESULT: %08lX\n", hr);

    if (pToken)
    {
        pToken->Release();
        pToken = NULL;
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
