#pragma once

// ============================================================
//  KeywordAPI.h
//
//  Drop-in header for any OBSE plugin that wants to use
//  OBSEKeywords's keyword system at runtime.
//
//  USAGE
//  -----
//  1. Copy this header into your project.
//  2. In OBSEPlugin_Load:
//
//       KeywordAPI::Init(g_msgIntfc, g_pluginHandle);
//
//  3. Wait for IsReady() before calling keyword functions, or
//     register a listener for kMessage_Ready.
//
//  4. Use keyword functions:
//
//       KeywordAPI::AddKeyword(formID, "Weapon");
//       if (KeywordAPI::HasKeyword(formID, "Blade")) { ... }
//
//  REQUIREMENTS
//  ------------
//  OBSEKeywords.dll (or OBSEKeywords.dll) must be loaded.
// ============================================================

#include "obse/PluginAPI.h"
#include <cstring>

namespace KeywordAPI
{
    // ---- Message types ----

    static const UInt32 kMessage_Ready = 'KWRD';  // Broadcast when ready
    static const UInt32 kMessage_AddKeyword = 'KWAD';
    static const UInt32 kMessage_RemoveKeyword = 'KWRM';
    static const UInt32 kMessage_HasKeyword = 'KWHS';
    static const UInt32 kMessage_GetCount = 'KWCT';
    static const UInt32 kMessage_Clear = 'KWCL';
    static const UInt32 kMessage_GetNth = 'KWGN';
    static const UInt32 kMessage_HasAny = 'KWAN';
    static const UInt32 kMessage_HasAll = 'KWAL';

    // ---- Data structs ----

    struct BasicData
    {
        UInt32      formID;
        const char* keyword;    // in (ignored for Clear/GetCount)
        bool        result;     // out (for Has queries)
        UInt32      count;      // out (for GetCount)
    };

    struct GetNthData
    {
        UInt32      formID;
        UInt32      index;      // in
        char        keyword[256]; // out (buffer filled by OBSEKeywords)
    };

    struct MultiKeywordData
    {
        UInt32      formID;
        const char* keywords[4]; // in (null-terminate early if < 4)
        bool        result;      // out
    };

    // ---- Client state ----

    inline bool                       s_ready = false;
    inline OBSEMessagingInterface* s_msgIntfc = nullptr;
    inline PluginHandle               s_pluginHandle = kPluginHandle_Invalid;

    // ---- MessageHandler ----

    inline void MessageHandler(OBSEMessagingInterface::Message* msg)
    {
        if (msg && msg->type == kMessage_Ready)
        {
            s_ready = true;
            _MESSAGE("OBSEKeywords: received ready signal");
        }
    }

    // ---- Init ----

    inline void Init(OBSEMessagingInterface* msgIntfc, PluginHandle pluginHandle)
    {
        s_msgIntfc = msgIntfc;
        s_pluginHandle = pluginHandle;
    }

    // ---- IsReady ----

    inline bool IsReady()
    {

        if (!s_msgIntfc)
        {
            _MESSAGE("OBSEKeywords: messaging interface missing");
        }

        if (!s_ready)
        {
            _MESSAGE("OBSEKeywords: Lookup before ready");
            return 0;
        }

        return s_ready && s_msgIntfc != nullptr;
    }

    // ---- AddKeyword ----

    inline bool AddKeyword(UInt32 formID, const char* keyword)
    {
        BasicData data = { formID, keyword, false, 0 };
        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_AddKeyword,
            &data, sizeof(data), nullptr);
        return data.result;
    }

    // ---- RemoveKeyword ----

    inline bool RemoveKeyword(UInt32 formID, const char* keyword)
    {
        if (!IsReady() || !keyword) return false;

        BasicData data = { formID, keyword, false, 0 };
        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_RemoveKeyword,
            &data, sizeof(data), nullptr);
        return data.result;
    }

    // ---- HasKeyword ----

    inline bool HasKeyword(UInt32 formID, const char* keyword)
    {
        if (!IsReady() || !keyword) return false;

        BasicData data = { formID, keyword, false, 0 };
        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_HasKeyword,
            &data, sizeof(data), nullptr);
        return data.result;
    }

    // ---- GetKeywordCount ----

    inline UInt32 GetKeywordCount(UInt32 formID)
    {
        if (!IsReady()) return 0;

        BasicData data = { formID, nullptr, false, 0 };
        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_GetCount,
            &data, sizeof(data), nullptr);
        return data.count;
    }

    // ---- ClearKeywords ----

    inline void ClearKeywords(UInt32 formID)
    {
        if (!IsReady()) return;

        BasicData data = { formID, nullptr, false, 0 };
        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_Clear,
            &data, sizeof(data), nullptr);
    }

    // ---- GetNthKeyword ----
    // Returns empty string if index out of range.

    inline std::string GetNthKeyword(UInt32 formID, UInt32 index)
    {
        if (!IsReady()) return "";

        GetNthData data = {};
        data.formID = formID;
        data.index = index;

        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_GetNth,
            &data, sizeof(data), nullptr);
        return data.keyword;
    }

    // ---- HasAnyKeyword ----
    // Check if form has any of the provided keywords (up to 4).
    // Null-terminate the array early if you have fewer than 4.

    inline bool HasAnyKeyword(UInt32 formID, const char* kw1,
        const char* kw2 = nullptr,
        const char* kw3 = nullptr,
        const char* kw4 = nullptr)
    {
        if (!IsReady()) return false;

        MultiKeywordData data = {};
        data.formID = formID;
        data.keywords[0] = kw1;
        data.keywords[1] = kw2;
        data.keywords[2] = kw3;
        data.keywords[3] = kw4;

        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_HasAny,
            &data, sizeof(data), nullptr);
        return data.result;
    }

    // ---- HasAllKeywords ----
    // Check if form has all of the provided keywords (up to 4).

    inline bool HasAllKeywords(UInt32 formID, const char* kw1,
        const char* kw2 = nullptr,
        const char* kw3 = nullptr,
        const char* kw4 = nullptr)
    {
        if (!IsReady()) return false;

        MultiKeywordData data = {};
        data.formID = formID;
        data.keywords[0] = kw1;
        data.keywords[1] = kw2;
        data.keywords[2] = kw3;
        data.keywords[3] = kw4;

        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_HasAll,
            &data, sizeof(data), nullptr);
        return data.result;
    }
}