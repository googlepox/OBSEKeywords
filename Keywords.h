#pragma once

#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse/GameAPI.h"
#include "obse/GameForms.h"
#include "obse/ParamInfos.h"
#include <string>
#include <map>
#include <set>
#include <vector>

// Plugin version
#define PLUGIN_VERSION 1

// Keyword system class
class KeywordManager
{
private:
    // Map of form ID -> set of keyword strings
    std::map<UInt32, std::set<std::string>> formKeywords;

    // Map of keyword -> set of form IDs (reverse index for fast lookup)
    std::map<std::string, std::set<UInt32>> keywordForms;

    static KeywordManager* instance;

    KeywordManager() {}

public:
    static KeywordManager* GetSingleton();

    // Core keyword functions
    bool AddKeyword(UInt32 formID, const std::string& keyword);
    bool RemoveKeyword(UInt32 formID, const std::string& keyword);
    bool HasKeyword(UInt32 formID, const std::string& keyword);

    // Query functions
    std::vector<std::string> GetKeywords(UInt32 formID);
    std::vector<UInt32> GetFormsWithKeyword(const std::string& keyword);
    int GetKeywordCount(UInt32 formID);

    // Utility
    void ClearFormKeywords(UInt32 formID);
    void ClearAllKeywords();

    // Serialization
    void Save(OBSESerializationInterface* intfc);
    void Load(OBSESerializationInterface* intfc);
    void NewGame();
};

// Script command declarations
bool Cmd_AddKeyword_Execute(COMMAND_ARGS);
bool Cmd_RemoveKeyword_Execute(COMMAND_ARGS);
bool Cmd_HasKeyword_Execute(COMMAND_ARGS);
bool Cmd_GetKeywordCount_Execute(COMMAND_ARGS);
bool Cmd_ClearKeywords_Execute(COMMAND_ARGS);
bool Cmd_GetNthKeyword_Execute(COMMAND_ARGS);
bool Cmd_HasAnyKeyword_Execute(COMMAND_ARGS);
bool Cmd_HasAllKeywords_Execute(COMMAND_ARGS);
bool Cmd_PrintKeywords_Execute(COMMAND_ARGS);

// Command info structures
extern CommandInfo kCommandInfo_AddKeyword;
extern CommandInfo kCommandInfo_AddKeywordRef;
extern CommandInfo kCommandInfo_RemoveKeyword;
extern CommandInfo kCommandInfo_RemoveKeywordRef;
extern CommandInfo kCommandInfo_HasKeyword;
extern CommandInfo kCommandInfo_HasKeywordRef;
extern CommandInfo kCommandInfo_GetKeywordCount;
extern CommandInfo kCommandInfo_GetKeywordCountRef;
extern CommandInfo kCommandInfo_ClearKeywords;
extern CommandInfo kCommandInfo_ClearKeywordsRef;
extern CommandInfo kCommandInfo_GetNthKeyword;
extern CommandInfo kCommandInfo_GetNthKeywordRef;
extern CommandInfo kCommandInfo_HasAnyKeyword;
extern CommandInfo kCommandInfo_HasAnyKeywordRef;
extern CommandInfo kCommandInfo_HasAllKeywords;
extern CommandInfo kCommandInfo_HasAllKeywordsRef;
extern CommandInfo kCommandInfo_PrintKeywords;
extern CommandInfo kCommandInfo_PrintKeywordsRef;

extern OBSEScriptInterface* g_scriptInterface;
#define ExtractArgsEx(...) g_scriptInterface->ExtractArgsEx(__VA_ARGS__)
#define ExtractFormatStringArgs(...) g_scriptInterface->ExtractFormatStringArgs(__VA_ARGS__)