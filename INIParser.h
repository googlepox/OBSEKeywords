#pragma once

#include <string>
#include <vector>
#include <Keywords.h>

// ============================================================
//  INI format reference
//
//  Data/OBSE/Plugins/OBSEKeywords/*.ini
//
//  ; Semicolons are comments (rest of line ignored)
//  # Hash comments are also supported
//
//  ; Section headers are optional — they are treated as
//  ; human-readable grouping only and do not affect parsing.
//  [Weapons]
//
//  ; EditorID  =  Keyword, Keyword, ...
//  WeapIronDagger     = Weapon, Blade, OneHanded, Metal, Iron
//  WeapIronLongsword  = Weapon, Blade, OneHanded, Metal, Iron
//
//  ; Hex FormID (with leading 0x) is also accepted.
//  ; Use this for forms that lack an editor ID at runtime.
//  0x00000001 = Weapon, Blade
//
//  ; Blank lines are ignored.
// ============================================================

struct INILoadResult
{
    std::string filePath;
    int         formsProcessed = 0;
    int         keywordsAdded = 0;
    int         errorLines = 0;
};

class INILoader
{
public:
    
    // Load all *.ini files found under
    //   Data/OBSE/Plugins/OBSEKeywords/
    // Returns one result entry per file parsed.
    static std::vector<INILoadResult> LoadAll();

    // Load a single named file (absolute or relative to working dir).
    static INILoadResult LoadFile(const std::string& path);

    // Return the canonical directory that LoadAll() scans.
    static std::string GetINIDirectory();

private:
    // Parse one logical line.  Returns false on bad format.
    static bool ParseLine(const std::string& line,
        std::string& outEditorIDOrFormID,
        std::vector<std::string>& outKeywords);

    // Resolve an editorID or "0x…" hex string to a form ID.
    // Returns 0 if the form cannot be found.
    static UInt32 ResolveForm(const std::string& token);

    // Trim leading/trailing whitespace in-place.
    static void Trim(std::string& s);
};

// Script command
bool Cmd_LoadKeywordsFromINI_Execute(COMMAND_ARGS);
extern CommandInfo kCommandInfo_LoadKeywordsFromINI;

// Optional: command that prints every loaded INI file to the console
bool Cmd_ReloadKeywordINIs_Execute(COMMAND_ARGS);
extern CommandInfo kCommandInfo_ReloadKeywordINIs;