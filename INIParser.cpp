#include "INIParser.h"
#include "Keywords.h"
#include "EditorIDMapper/EditorIDMapperAPI.h"

#include "string.hpp"

#include "obse/GameAPI.h"
#include "obse/GameForms.h"
#include "obse/GameData.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <ranges>

// ============================================================
//  Helpers
// ============================================================

void INILoader::Trim(std::string& s)
{
    // Leading
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
        }));
    // Trailing
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
        }).base(), s.end());
}

// Strip an inline comment (everything from the first ; or # onwards).
static void StripComment(std::string& s)
{
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == ';' || s[i] == '#')
        {
            s.erase(i);
            return;
        }
    }
}

// ============================================================
//  Form resolution
// ============================================================

UInt32 INILoader::ResolveForm(const std::string& token)
{
    if (token.empty()) return 0;

    constexpr auto lookup_formID = [](std::uint32_t a_refID, const std::string& modName) -> std::uint32_t {
        const auto modIdx = (*g_dataHandler)->GetModIndex(modName.c_str());
        return modIdx == 0xFF ? 0 : (a_refID & 0xFFFFFF) | modIdx << 24;
        };

    if (const auto splitID = clib_util::string::split(token, "~"); splitID.size() == 2)
    {
        const auto  formID = clib_util::string::to_num<std::uint32_t>(splitID[0], true);
        const auto& modName = splitID[1];
        return lookup_formID(formID, modName);
    }
    if (clib_util::string::is_only_hex(token, true))
    {
        if (const auto form = LookupFormByID(clib_util::string::to_num<std::uint32_t>(token, true)))
        {
            return form->refID;
        }
    }

    if (!EditorIDMapper::IsReady())
    {
        _WARNING("INILoader: EditorIDMapper not ready, cannot resolve '%s'", token.c_str());
        return 0;
    }

    UInt32 formID = EditorIDMapper::Lookup(token);
    if (formID == 0)
    {
        _WARNING("INILoader: could not resolve editor ID '%s'", token.c_str());
        return 0;
    }
    return formID;
}

// ============================================================
//  Line parser
// ============================================================

bool INILoader::ParseLine(const std::string& rawLine,
    std::string& outToken,
    std::vector<std::string>& outKeywords)
{
    outToken.clear();
    outKeywords.clear();

    std::string line = rawLine;
    StripComment(line);
    Trim(line);

    if (line.empty())            return false;   // blank / comment-only
    if (line.front() == '[')     return false;   // section header — skip

    // Expect exactly one '=' separator
    auto eq = line.find('=');
    if (eq == std::string::npos)
    {
        _WARNING("INILoader: no '=' found in line: '%s'", rawLine.c_str());
        return false;
    }

    outToken = line.substr(0, eq);
    Trim(outToken);
    if (outToken.empty())
    {
        _WARNING("INILoader: empty form token in line: '%s'", rawLine.c_str());
        return false;
    }

    std::string keywordPart = line.substr(eq + 1);

    // Split keyword list on commas
    std::stringstream ss(keywordPart);
    std::string kw;
    while (std::getline(ss, kw, ','))
    {
        Trim(kw);
        if (!kw.empty())
        {
            outKeywords.push_back(kw);
        }
    }

    if (outKeywords.empty())
    {
        _WARNING("INILoader: no keywords found in line: '%s'", rawLine.c_str());
        return false;
    }

    return true;
}

// ============================================================
//  Directory
// ============================================================

std::string INILoader::GetINIDirectory()
{
    // Relative to the Oblivion working directory
    return "Data\\OBSE\\Plugins\\OBSEKeywords\\";
}

// ============================================================
//  Load a single file
// ============================================================

INILoadResult INILoader::LoadFile(const std::string& path)
{
    INILoadResult result;
    result.filePath = path;

    std::ifstream file(path);
    if (!file.is_open())
    {
        _WARNING("INILoader: cannot open file '%s'", path.c_str());
        return result;
    }

    _MESSAGE("INILoader: reading '%s'", path.c_str());

    KeywordManager* mgr = KeywordManager::GetSingleton();

    std::string line;
    int lineNum = 0;

    while (std::getline(file, line))
    {
        ++lineNum;

        std::string token;
        std::vector<std::string> keywords;

        if (!ParseLine(line, token, keywords))
        {
            // ParseLine returns false for blanks, comments, and section headers
            // as well as genuine errors; it already logged warnings for errors.
            // We only count it as an error if the line had actual content.
            std::string trimmed = line;
            Trim(trimmed);
            StripComment(trimmed);
            Trim(trimmed);
            if (!trimmed.empty() && trimmed.front() != '[')
            {
                ++result.errorLines;
            }
            continue;
        }

        UInt32 formID = ResolveForm(token);
        if (formID == 0)
        {
            ++result.errorLines;
            continue;
        }

        for (const auto& kw : keywords)
        {
            if (mgr->AddKeyword(formID, kw))
            {
                ++result.keywordsAdded;
            }
        }
        ++result.formsProcessed;
    }

    _MESSAGE("INILoader: '%s' — %d forms, %d keywords, %d errors",
        path.c_str(), result.formsProcessed, result.keywordsAdded, result.errorLines);

    return result;
}

// ============================================================
//  Load all *.ini files in the plugin directory
// ============================================================

std::vector<INILoadResult> INILoader::LoadAll()
{
    std::vector<INILoadResult> results;

    std::string dir = GetINIDirectory();
    std::string pattern = dir + "*.ini";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        _MESSAGE("INILoader: no *.ini files found in '%s' (or directory missing)", dir.c_str());
        return results;
    }

    do
    {
        // Skip directories (shouldn't exist here, but be safe)
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        std::string fullPath = dir + findData.cFileName;
        results.push_back(LoadFile(fullPath));

    }
    while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // Summary
    int totalForms = 0, totalKeywords = 0, totalErrors = 0;
    for (const auto& r : results)
    {
        totalForms += r.formsProcessed;
        totalKeywords += r.keywordsAdded;
        totalErrors += r.errorLines;
    }
    _MESSAGE("INILoader: finished — %d file(s), %d forms, %d keywords, %d error line(s)",
        (int)results.size(), totalForms, totalKeywords, totalErrors);

    return results;
}

// ============================================================
//  Script commands
// ============================================================

// LoadKeywordsFromINI "path\to\file.ini"
// Returns number of keywords added, or -1 on failure to open.
bool Cmd_LoadKeywordsFromINI_Execute(COMMAND_ARGS)
{
    *result = -1;

    char path[512] = {};
    if (!ExtractArgs(PASS_EXTRACT_ARGS, &path)) return true;
    if (!path[0]) return true;

    INILoadResult r = INILoader::LoadFile(path);
    *result = (r.formsProcessed > 0 || r.keywordsAdded > 0)
        ? r.keywordsAdded
        : -1;

    Console_Print("INI load '%s': %d forms, %d keywords, %d errors",
        path, r.formsProcessed, r.keywordsAdded, r.errorLines);
    return true;
}

// ReloadKeywordINIs
// Scans the default directory and (re-)applies all *.ini files.
// Does NOT clear existing keywords first — use ClearAllKeywords first if you want a clean slate.
// Returns total number of keywords added.
bool Cmd_ReloadKeywordINIs_Execute(COMMAND_ARGS)
{
    *result = 0;

    Console_Print("INI reload from '%s'...", INILoader::GetINIDirectory().c_str());

    auto results = INILoader::LoadAll();

    int total = 0;
    for (const auto& r : results)
    {
        total += r.keywordsAdded;
        Console_Print("  %s — %d kw (%d err)",
            r.filePath.c_str(), r.keywordsAdded, r.errorLines);
    }

    Console_Print("INI reload complete: %d file(s), %d keywords total",
        (int)results.size(), total);

    *result = total;
    return true;
}

// ============================================================
//  Command info
// ============================================================

static ParamInfo kParams_LoadKeywordsFromINI[] = {
    { "path", kParamType_String, 0 },
};

DEFINE_COMMAND_PLUGIN(LoadKeywordsFromINI,
    "Loads keywords from a specific INI file. Returns keywords added or -1 on error.",
    0, 1, kParams_LoadKeywordsFromINI);

DEFINE_COMMAND_PLUGIN(ReloadKeywordINIs,
    "Reloads all *.ini files from the default OBSEKeywords directory.",
    0, 0, nullptr);