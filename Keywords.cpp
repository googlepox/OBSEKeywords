#include "Keywords.h"
#include "INIParser.h"
#include <algorithm>
#include <obse/StringVar.h>
#include <obse/GameObjects.h>

// Initialize static instance
KeywordManager* KeywordManager::instance = nullptr;

// ===== KeywordManager Implementation =====

KeywordManager* KeywordManager::GetSingleton()
{
    if (!instance)
    {
        instance = new KeywordManager();
    }
    return instance;
}

bool KeywordManager::AddKeyword(UInt32 formID, const std::string& keyword)
{
    if (keyword.empty()) return false;

    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    formKeywords[formID].insert(lowerKeyword);
    keywordForms[lowerKeyword].insert(formID);

    return true;
}

bool KeywordManager::RemoveKeyword(UInt32 formID, const std::string& keyword)
{
    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    auto formIt = formKeywords.find(formID);
    if (formIt != formKeywords.end())
    {
        formIt->second.erase(lowerKeyword);
        if (formIt->second.empty())
        {
            formKeywords.erase(formIt);
        }
    }

    auto keywordIt = keywordForms.find(lowerKeyword);
    if (keywordIt != keywordForms.end())
    {
        keywordIt->second.erase(formID);
        if (keywordIt->second.empty())
        {
            keywordForms.erase(keywordIt);
        }
    }

    return true;
}

bool KeywordManager::HasKeyword(UInt32 formID, const std::string& keyword)
{
    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    auto it = formKeywords.find(formID);
    if (it != formKeywords.end())
    {
        bool found = it->second.find(lowerKeyword) != it->second.end();
        return found;
    }

    return false;
}

std::vector<std::string> KeywordManager::GetKeywords(UInt32 formID)
{
    std::vector<std::string> result;
    auto it = formKeywords.find(formID);
    if (it != formKeywords.end())
    {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<UInt32> KeywordManager::GetFormsWithKeyword(const std::string& keyword)
{
    std::vector<UInt32> result;
    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    auto it = keywordForms.find(lowerKeyword);
    if (it != keywordForms.end())
    {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

int KeywordManager::GetKeywordCount(UInt32 formID)
{
    auto it = formKeywords.find(formID);
    if (it != formKeywords.end())
    {
        return it->second.size();
    }
    return 0;
}

void KeywordManager::ClearFormKeywords(UInt32 formID)
{
    auto it = formKeywords.find(formID);
    if (it != formKeywords.end())
    {
        // Remove from reverse index
        for (const auto& keyword : it->second)
        {
            auto keyIt = keywordForms.find(keyword);
            if (keyIt != keywordForms.end())
            {
                keyIt->second.erase(formID);
                if (keyIt->second.empty())
                {
                    keywordForms.erase(keyIt);
                }
            }
        }
        formKeywords.erase(it);
    }
}

void KeywordManager::ClearAllKeywords()
{
    formKeywords.clear();
    keywordForms.clear();
}

// ===== Serialization =====

void KeywordManager::Save(OBSESerializationInterface* intfc)
{
    // Write number of forms with keywords
    UInt32 numForms = formKeywords.size();
    intfc->WriteRecord('KWCT', 1, &numForms, sizeof(numForms));

    // Write each form's keywords
    for (const auto& pair : formKeywords)
    {
        UInt32 formID = pair.first;
        UInt32 numKeywords = pair.second.size();

        intfc->WriteRecord('KWFM', 1, &formID, sizeof(formID));
        intfc->WriteRecord('KWKC', 1, &numKeywords, sizeof(numKeywords));

        for (const auto& keyword : pair.second)
        {
            UInt32 keywordLen = keyword.length();
            intfc->WriteRecord('KWKL', 1, &keywordLen, sizeof(keywordLen));
            intfc->WriteRecord('KWKD', 1, keyword.c_str(), keywordLen);
        }
    }
}

void KeywordManager::Load(OBSESerializationInterface* intfc)
{
    ClearAllKeywords();

    UInt32 type, version, length;

    while (intfc->GetNextRecordInfo(&type, &version, &length))
    {
        switch (type)
        {
        case 'KWCT':
        {
            UInt32 numForms;
            intfc->ReadRecordData(&numForms, sizeof(numForms));
            break;
        }

        case 'KWFM':
        {
            UInt32 oldFormID, newFormID;
            intfc->ReadRecordData(&oldFormID, sizeof(oldFormID));

            // Resolve form ID in case of mod changes
            if (!intfc->ResolveRefID(oldFormID, &newFormID))
            {
                newFormID = oldFormID;
            }

            // Read keyword count
            if (intfc->GetNextRecordInfo(&type, &version, &length) && type == 'KWKC')
            {
                UInt32 numKeywords;
                intfc->ReadRecordData(&numKeywords, sizeof(numKeywords));

                // Read each keyword
                for (UInt32 i = 0; i < numKeywords; i++)
                {
                    if (intfc->GetNextRecordInfo(&type, &version, &length) && type == 'KWKL')
                    {
                        UInt32 keywordLen;
                        intfc->ReadRecordData(&keywordLen, sizeof(keywordLen));

                        if (intfc->GetNextRecordInfo(&type, &version, &length) && type == 'KWKD')
                        {
                            char* buffer = new char[keywordLen + 1];
                            intfc->ReadRecordData(buffer, keywordLen);
                            buffer[keywordLen] = '\0';

                            std::string keyword(buffer);
                            AddKeyword(newFormID, keyword);

                            delete[] buffer;
                        }
                    }
                }
            }
            break;
        }
        }
    }
}

void KeywordManager::NewGame()
{
    ClearAllKeywords();
}


bool Cmd_AddKeyword_Execute(COMMAND_ARGS)
{
    *result = 0;
    TESForm* form = nullptr;
    char keyword[512] = { 0 };

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, keyword))
        return true;

    if (!form)
        return true;

    if (!keyword[0])
        return true;

    if (KeywordManager::GetSingleton()->AddKeyword(form->refID, keyword))
        *result = 1;

    return true;
}

bool Cmd_AddKeywordRef_Execute(COMMAND_ARGS)
{
    *result = 0;
    TESObjectREFR* form = nullptr;
    char keyword[512] = { 0 };

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, keyword))
        return true;

    if (!form)
        return true;

    if (!keyword[0])
        return true;

    if (KeywordManager::GetSingleton()->AddKeyword(form->refID, keyword))
        *result = 1;

    return true;
}

bool Cmd_RemoveKeyword_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESForm* form = nullptr;
    char keyword[512];

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &keyword))
    {
        return true;
    }

    if (!form || !keyword[0])
    {
        return true;
    }

    if (KeywordManager::GetSingleton()->RemoveKeyword(form->refID, keyword))
    {
        *result = 1;
    }

    return true;
}

bool Cmd_RemoveKeywordRef_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESObjectREFR* form = nullptr;
    char keyword[512];

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &keyword))
    {
        return true;
    }

    if (!form || !keyword[0])
    {
        return true;
    }

    if (KeywordManager::GetSingleton()->RemoveKeyword(form->refID, keyword))
    {
        *result = 1;
    }

    return true;
}

bool Cmd_HasKeyword_Execute(COMMAND_ARGS)
{
    *result = 0;
    TESForm* form = nullptr;
    char keyword[512] = { 0 };

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, keyword))
        return true;

    if (!form)
        return true;

    if (KeywordManager::GetSingleton()->HasKeyword(form->refID, keyword))
    {
        *result = 1;
    }

    return true;
}

bool Cmd_HasKeywordRef_Execute(COMMAND_ARGS)
{
    *result = 0;
    TESObjectREFR* form = nullptr;
    char keyword[512] = { 0 };

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, keyword))
        return true;

    if (!form)
        return true;

    if (KeywordManager::GetSingleton()->HasKeyword(form->refID, keyword))
        *result = 1;

    return true;
}

bool Cmd_GetKeywordCount_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESForm* form = nullptr;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    *result = KeywordManager::GetSingleton()->GetKeywordCount(form->refID);

    return true;
}

bool Cmd_GetKeywordCountRef_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESObjectREFR* form = nullptr;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    *result = KeywordManager::GetSingleton()->GetKeywordCount(form->refID);

    return true;
}

bool Cmd_ClearKeywords_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESForm* form = nullptr;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    KeywordManager::GetSingleton()->ClearFormKeywords(form->refID);
    *result = 1;

    return true;
}

bool Cmd_ClearKeywordsRef_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESObjectREFR* form = nullptr;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    KeywordManager::GetSingleton()->ClearFormKeywords(form->refID);
    *result = 1;

    return true;
}

bool Cmd_GetNthKeyword_Execute(COMMAND_ARGS)
{
    const char* resultStr = "";

    TESForm* form = nullptr;
    UInt32 index = 0;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &index))
    {
        AssignToStringVar(PASS_COMMAND_ARGS, resultStr);
        return true;
    }

    if (!form)
    {
        AssignToStringVar(PASS_COMMAND_ARGS, resultStr);
        return true;
    }

    auto keywords = KeywordManager::GetSingleton()->GetKeywords(form->refID);
    if (index < keywords.size())
    {
        resultStr = keywords[index].c_str();
    }

    AssignToStringVar(PASS_COMMAND_ARGS, resultStr);
    return true;
}

bool Cmd_GetNthKeywordRef_Execute(COMMAND_ARGS)
{
    const char* resultStr = "";

    TESForm* form = nullptr;
    UInt32 index = 0;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &index))
    {
        AssignToStringVar(PASS_COMMAND_ARGS, resultStr);
        return true;
    }

    if (!form)
    {
        AssignToStringVar(PASS_COMMAND_ARGS, resultStr);
        return true;
    }

    auto keywords = KeywordManager::GetSingleton()->GetKeywords(form->refID);
    if (index < keywords.size())
    {
        resultStr = keywords[index].c_str();
    }

    AssignToStringVar(PASS_COMMAND_ARGS, resultStr);
    return true;
}

bool Cmd_HasAnyKeyword_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESForm* form = nullptr;
    char keyword1[512] = "";
    char keyword2[512] = "";
    char keyword3[512] = "";
    char keyword4[512] = "";

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &keyword1, &keyword2, &keyword3, &keyword4))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    KeywordManager* mgr = KeywordManager::GetSingleton();

    if (keyword1[0] && mgr->HasKeyword(form->refID, keyword1))
    {
        *result = 1;
        return true;
    }
    if (keyword2[0] && mgr->HasKeyword(form->refID, keyword2))
    {
        *result = 1;
        return true;
    }
    if (keyword3[0] && mgr->HasKeyword(form->refID, keyword3))
    {
        *result = 1;
        return true;
    }
    if (keyword4[0] && mgr->HasKeyword(form->refID, keyword4))
    {
        *result = 1;
        return true;
    }

    return true;
}

bool Cmd_HasAnyKeywordRef_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESObjectREFR* form = nullptr;
    char keyword1[512] = "";
    char keyword2[512] = "";
    char keyword3[512] = "";
    char keyword4[512] = "";

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &keyword1, &keyword2, &keyword3, &keyword4))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    KeywordManager* mgr = KeywordManager::GetSingleton();

    if (keyword1[0] && mgr->HasKeyword(form->refID, keyword1))
    {
        *result = 1;
        return true;
    }
    if (keyword2[0] && mgr->HasKeyword(form->refID, keyword2))
    {
        *result = 1;
        return true;
    }
    if (keyword3[0] && mgr->HasKeyword(form->refID, keyword3))
    {
        *result = 1;
        return true;
    }
    if (keyword4[0] && mgr->HasKeyword(form->refID, keyword4))
    {
        *result = 1;
        return true;
    }

    return true;
}

bool Cmd_HasAllKeywords_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESForm* form = nullptr;
    char keyword1[512] = "";
    char keyword2[512] = "";
    char keyword3[512] = "";
    char keyword4[512] = "";

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &keyword1, &keyword2, &keyword3, &keyword4))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    KeywordManager* mgr = KeywordManager::GetSingleton();

    if (keyword1[0] && !mgr->HasKeyword(form->refID, keyword1))
    {
        return true;
    }
    if (keyword2[0] && !mgr->HasKeyword(form->refID, keyword2))
    {
        return true;
    }
    if (keyword3[0] && !mgr->HasKeyword(form->refID, keyword3))
    {
        return true;
    }
    if (keyword4[0] && !mgr->HasKeyword(form->refID, keyword4))
    {
        return true;
    }

    *result = 1;
    return true;
}

bool Cmd_HasAllKeywordsRef_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESObjectREFR* form = nullptr;
    char keyword1[512] = "";
    char keyword2[512] = "";
    char keyword3[512] = "";
    char keyword4[512] = "";

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form, &keyword1, &keyword2, &keyword3, &keyword4))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    KeywordManager* mgr = KeywordManager::GetSingleton();

    if (keyword1[0] && !mgr->HasKeyword(form->refID, keyword1))
    {
        return true;
    }
    if (keyword2[0] && !mgr->HasKeyword(form->refID, keyword2))
    {
        return true;
    }
    if (keyword3[0] && !mgr->HasKeyword(form->refID, keyword3))
    {
        return true;
    }
    if (keyword4[0] && !mgr->HasKeyword(form->refID, keyword4))
    {
        return true;
    }

    *result = 1;
    return true;
}

bool Cmd_PrintKeywords_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESForm* form = nullptr;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    auto keywords = KeywordManager::GetSingleton()->GetKeywords(form->refID);

    Console_Print("Keywords for form %08X:", form->refID);
    if (keywords.empty())
    {
        Console_Print("  (none)");
    }
    else
    {
        for (const auto& keyword : keywords)
        {
            Console_Print("  %s", keyword.c_str());
        }
    }

    *result = keywords.size();
    return true;
}

bool Cmd_PrintKeywordsRef_Execute(COMMAND_ARGS)
{
    *result = 0;

    TESObjectREFR* form = nullptr;

    if (!ExtractArgs(PASS_EXTRACT_ARGS, &form))
    {
        return true;
    }

    if (!form)
    {
        return true;
    }

    auto keywords = KeywordManager::GetSingleton()->GetKeywords(form->refID);

    Console_Print("Keywords for form %08X:", form->refID);
    if (keywords.empty())
    {
        Console_Print("  (none)");
    }
    else
    {
        for (const auto& keyword : keywords)
        {
            Console_Print("  %s", keyword.c_str());
        }
    }

    *result = keywords.size();
    return true;
}

static ParamInfo kParams_OneForm[] = {
    { "form", kParamType_TESObject, 0 },
};

static ParamInfo kParams_OneRef[] = {
    { "form", kParamType_ObjectRef, 0 },
};

static ParamInfo kParams_OneForm_OneString[] = {
    { "form",    kParamType_TESObject, 0 },
    { "keyword", kParamType_String,  0 },
};

static ParamInfo kParams_OneRef_OneString[] = {
    { "form",    kParamType_ObjectRef, 0 },
    { "keyword", kParamType_String,  0 },
};

static ParamInfo kParams_GetNthKeyword[] = {
    { "form",  kParamType_TESObject, 0 },
    { "index", kParamType_Integer, 0 },
};

static ParamInfo kParams_GetNthKeywordRef[] = {
    { "form",  kParamType_ObjectRef, 0 },
    { "index", kParamType_Integer, 0 },
};

static ParamInfo kParams_FormAndFourKeywords[] = {
    { "form",     kParamType_TESObject, 0 },
    { "keyword1", kParamType_String,  0 },
    { "keyword2", kParamType_String,  1 },
    { "keyword3", kParamType_String,  1 },
    { "keyword4", kParamType_String,  1 },
};

static ParamInfo kParams_RefAndFourKeywords[] = {
    { "form",     kParamType_ObjectRef, 0 },
    { "keyword1", kParamType_String,  0 },
    { "keyword2", kParamType_String,  1 },
    { "keyword3", kParamType_String,  1 },
    { "keyword4", kParamType_String,  1 },
};

// ===== Command Info Definitions =====

DEFINE_COMMAND_PLUGIN(AddKeyword, "Adds a keyword to a form", 0, 2, kParams_OneForm_OneString);
DEFINE_COMMAND_PLUGIN(AddKeywordRef, "Adds a keyword to a ref", 0, 2, kParams_OneRef_OneString);
DEFINE_COMMAND_PLUGIN(RemoveKeyword, "Removes a keyword from a form", 0, 2, kParams_OneForm_OneString);
DEFINE_COMMAND_PLUGIN(RemoveKeywordRef, "Removes a keyword from a ref", 0, 2, kParams_OneRef_OneString);
DEFINE_COMMAND_PLUGIN(HasKeyword, "Returns 1 if a form has the given keyword", 0, 2, kParams_OneForm_OneString);
DEFINE_COMMAND_PLUGIN(HasKeywordRef, "Returns 1 if a ref has the given keyword", 0, 2, kParams_OneRef_OneString);
DEFINE_COMMAND_PLUGIN(GetKeywordCount, "Returns the number of keywords on a form", 0, 1, kParams_OneForm);
DEFINE_COMMAND_PLUGIN(GetKeywordCountRef, "Returns the number of keywords on a ref", 0, 1, kParams_OneRef);
DEFINE_COMMAND_PLUGIN(ClearKeywords, "Removes all keywords from a form", 0, 1, kParams_OneForm);
DEFINE_COMMAND_PLUGIN(ClearKeywordsRef, "Removes all keywords from a ref", 0, 1, kParams_OneRef);
DEFINE_COMMAND_PLUGIN(GetNthKeyword, "Returns the Nth keyword string from a form", 0, 2, kParams_GetNthKeyword);
DEFINE_COMMAND_PLUGIN(GetNthKeywordRef, "Returns the Nth keyword string from a ref", 0, 2, kParams_GetNthKeywordRef);
DEFINE_COMMAND_PLUGIN(HasAnyKeyword, "Returns 1 if form has any of up to 4 keywords", 0, 5, kParams_FormAndFourKeywords);
DEFINE_COMMAND_PLUGIN(HasAnyKeywordRef, "Returns 1 if ref has any of up to 4 keywords", 0, 5, kParams_RefAndFourKeywords);
DEFINE_COMMAND_PLUGIN(HasAllKeywords, "Returns 1 if form has all of up to 4 keywords", 0, 5, kParams_FormAndFourKeywords);
DEFINE_COMMAND_PLUGIN(HasAllKeywordsRef, "Returns 1 if ref has all of up to 4 keywords", 0, 5, kParams_RefAndFourKeywords);
DEFINE_COMMAND_PLUGIN(PrintKeywords, "Prints all keywords for a form to the console", 0, 1, kParams_OneForm);
DEFINE_COMMAND_PLUGIN(PrintKeywordsRef, "Prints all keywords for a ref to the console", 0, 1, kParams_OneRef);