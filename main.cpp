#include "Keywords.h"
#include "INIParser.h"
#include "EditorIDMapper/EditorIDMapperAPI.h"
#include "obse/PluginAPI.h"
#include "obse_common/SafeWrite.h"
#include <KeywordAPI.h>

IDebugLog gLog("OBSEKeywords.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

OBSESerializationInterface* g_serialization = nullptr;
OBSEMessagingInterface* g_messaging = nullptr;

void KeywordMessageHandler(OBSEMessagingInterface::Message* msg)
{
    if (!msg || !msg->data) return;

    KeywordManager* mgr = KeywordManager::GetSingleton();

    switch (msg->type)
    {
    case OBSEMessagingInterface::kMessage_PostPostLoad:
        _MESSAGE("OBSEKeywords: broadcasting ready signal");
        g_messaging->Dispatch(g_pluginHandle, KeywordAPI::kMessage_Ready,
            nullptr, 0, nullptr);
        break;

    case KeywordAPI::kMessage_AddKeyword:
    {
        auto* data = static_cast<KeywordAPI::BasicData*>(msg->data);
        data->result = mgr->AddKeyword(data->formID, data->keyword);
        break;
    }

    case KeywordAPI::kMessage_RemoveKeyword:
    {
        auto* data = static_cast<KeywordAPI::BasicData*>(msg->data);
        data->result = mgr->RemoveKeyword(data->formID, data->keyword);
        break;
    }

    case KeywordAPI::kMessage_HasKeyword:
    {
        auto* data = static_cast<KeywordAPI::BasicData*>(msg->data);
        data->result = mgr->HasKeyword(data->formID, data->keyword);
        break;
    }

    case KeywordAPI::kMessage_GetCount:
    {
        auto* data = static_cast<KeywordAPI::BasicData*>(msg->data);
        data->count = mgr->GetKeywordCount(data->formID);
        break;
    }

    case KeywordAPI::kMessage_Clear:
    {
        auto* data = static_cast<KeywordAPI::BasicData*>(msg->data);
        mgr->ClearFormKeywords(data->formID);
        break;
    }

    case KeywordAPI::kMessage_GetNth:
    {
        auto* data = static_cast<KeywordAPI::GetNthData*>(msg->data);
        data->keyword[0] = '\0';  // Default to empty

        auto keywords = mgr->GetKeywords(data->formID);
        if (data->index < keywords.size())
        {
            strncpy_s(data->keyword, sizeof(data->keyword),
                keywords[data->index].c_str(), _TRUNCATE);
        }
        break;
    }

    case KeywordAPI::kMessage_HasAny:
    {
        auto* data = static_cast<KeywordAPI::MultiKeywordData*>(msg->data);
        data->result = false;

        for (int i = 0; i < 4 && data->keywords[i]; ++i)
        {
            if (mgr->HasKeyword(data->formID, data->keywords[i]))
            {
                data->result = true;
                break;
            }
        }
        break;
    }

    case KeywordAPI::kMessage_HasAll:
    {
        auto* data = static_cast<KeywordAPI::MultiKeywordData*>(msg->data);
        data->result = true;  // Assume true until proven otherwise

        for (int i = 0; i < 4 && data->keywords[i]; ++i)
        {
            if (!mgr->HasKeyword(data->formID, data->keywords[i]))
            {
                data->result = false;
                break;
            }
        }
        break;
    }

    default:
        break;
    }
}

void SaveCallback(void* reserved)
{
    _MESSAGE("Saving keyword data...");
    KeywordManager::GetSingleton()->Save(g_serialization);
    _MESSAGE("Save complete");
}

void LoadCallback(void* reserved)
{
    _MESSAGE("Loading keyword data...");
    KeywordManager::GetSingleton()->Load(g_serialization);

    _MESSAGE("Applying INI keywords...");
    if (!EditorIDMapper::IsReady())
        _WARNING("EditorIDMapper not ready — editor ID lookups will fail");

    INILoader::LoadAll();
    _MESSAGE("Load complete");
}

void NewGameCallback(void* reserved)
{
    _MESSAGE("New game started - clearing runtime keywords");
    KeywordManager::GetSingleton()->NewGame();

    _MESSAGE("Applying INI keywords...");
    INILoader::LoadAll();
}

void MessageHandler()
{
    
}

extern "C" {

    bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
    {
        _MESSAGE("OBSEKeywords Query");

        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = "OBSEKeywords";
        info->version = PLUGIN_VERSION;

        if (obse->obseVersion < OBSE_VERSION_INTEGER)
        {
            _MESSAGE("OBSE version too old (got %08X, expected %08X)",
                obse->obseVersion, OBSE_VERSION_INTEGER);
            return false;
        }

        return true;
    }

    bool OBSEPlugin_Load(const OBSEInterface* obse)
    {
        _MESSAGE("OBSEKeywords Load");

        g_pluginHandle = obse->GetPluginHandle();

        obse->SetOpcodeBase(0x2760);
        obse->RegisterCommand(&kCommandInfo_AddKeyword);
        obse->RegisterCommand(&kCommandInfo_RemoveKeyword);
        obse->RegisterCommand(&kCommandInfo_HasKeyword);
        obse->RegisterCommand(&kCommandInfo_GetKeywordCount);
        obse->RegisterCommand(&kCommandInfo_ClearKeywords);
        obse->RegisterCommand(&kCommandInfo_GetNthKeyword);
        obse->RegisterCommand(&kCommandInfo_HasAnyKeyword);
        obse->RegisterCommand(&kCommandInfo_HasAllKeywords);
        obse->RegisterCommand(&kCommandInfo_PrintKeywords);
        obse->RegisterCommand(&kCommandInfo_LoadKeywordsFromINI);
        obse->RegisterCommand(&kCommandInfo_ReloadKeywordINIs);
        _MESSAGE("Commands registered with opcode base 0x2760");

        if (obse->isEditor)
        {
            _MESSAGE("Loaded in editor");
            return true;
        }

        g_serialization = (OBSESerializationInterface*)obse->QueryInterface(kInterface_Serialization);
        if (!g_serialization)
        {
            _ERROR("Serialization interface not found");
            return false;
        }
        if (g_serialization->version < OBSESerializationInterface::kVersion)
        {
            _ERROR("Serialization interface too old (%d, expected %d)",
                g_serialization->version, OBSESerializationInterface::kVersion);
            return false;
        }
        g_serialization->SetSaveCallback(g_pluginHandle, SaveCallback);
        g_serialization->SetLoadCallback(g_pluginHandle, LoadCallback);
        g_serialization->SetNewGameCallback(g_pluginHandle, NewGameCallback);

        g_messaging = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);

        EditorIDMapper::Init(g_messaging, g_pluginHandle);

        g_messaging->RegisterListener(g_pluginHandle, "OBSE", KeywordMessageHandler);
        g_messaging->RegisterListener(g_pluginHandle, nullptr, KeywordMessageHandler);

        _MESSAGE("OBSEKeywords loaded successfully");
        return true;
    }

};