# Oblivion Keyword Plugin

A keyword system for Oblivion using OBSE, allowing you to tag forms with keywords for easy categorization and script checks.

## Features

- Add/remove keywords to any form (items, NPCs, spells, etc.)
- Check if forms have specific keywords
- Query all keywords on a form
- Check for any/all keywords from a list
- Automatic save/load integration
- Case-insensitive keyword matching

## Building

### Requirements

- Visual Studio 2019 or later (Community Edition works)
- OBSE SDK (from https://github.com/llde/xOBSE)
- Oblivion installed

### Build Steps

1. Download and extract the OBSE SDK
2. Create a new DLL project in Visual Studio
3. Add the source files (KeywordPlugin.h, KeywordPlugin.cpp, main.cpp)
4. Configure project settings:
   - Platform: Win32 (x86)
   - Configuration Type: Dynamic Library (.dll)
   - C++ Language Standard: ISO C++17 or later
   
5. Add include directories:
   - Path to OBSE SDK (e.g., `C:\OBSE_SDK\obse`)
   - Path to OBSE common (e.g., `C:\OBSE_SDK\obse_common`)

6. Add library directories and link obse_loader.lib if needed

7. Build the project (Release configuration recommended)

8. Rename the output DLL to `KeywordPlugin.dll`

9. Copy to `Oblivion\Data\OBSE\Plugins\`

## Script Commands

### AddKeyword
```
AddKeyword form:ref keyword:string
```
Adds a keyword to a form. Returns 1 if successful.

**Example:**
```
AddKeyword WeapIronDagger "Weapon"
AddKeyword WeapIronDagger "Metal"
AddKeyword WeapIronDagger "OneHanded"
```

### RemoveKeyword
```
RemoveKeyword form:ref keyword:string
```
Removes a keyword from a form. Returns 1 if successful.

**Example:**
```
RemoveKeyword WeapIronDagger "Metal"
```

### HasKeyword
```
HasKeyword form:ref keyword:string
```
Checks if a form has a specific keyword. Returns 1 if true, 0 if false.

**Example:**
```
if HasKeyword player.GetEquippedObject 16 "Weapon"
    Message "You're wielding a weapon!"
endif
```

### GetKeywordCount
```
GetKeywordCount form:ref
```
Returns the number of keywords on a form.

**Example:**
```
set keywordCount to GetKeywordCount WeapIronDagger
```

### GetNthKeyword
```
GetNthKeyword form:ref index:int
```
Returns the keyword at the specified index (0-based). Returns empty string if index is out of range.

**Example:**
```
set keyword$ to GetNthKeyword WeapIronDagger 0
```

### HasAnyKeyword
```
HasAnyKeyword form:ref keyword1:string keyword2:string keyword3:string keyword4:string
```
Checks if form has ANY of the specified keywords (up to 4). Returns 1 if any match found.

**Example:**
```
if HasAnyKeyword targetItem "Weapon" "Armor" "Clothing"
    Message "This is wearable equipment!"
endif
```

### HasAllKeywords
```
HasAllKeywords form:ref keyword1:string keyword2:string keyword3:string keyword4:string
```
Checks if form has ALL of the specified keywords (up to 4). Returns 1 if all present.

**Example:**
```
if HasAllKeywords weapon "Weapon" "Metal" "TwoHanded"
    Message "Heavy weapon!"
endif
```

### ClearKeywords
```
ClearKeywords form:ref
```
Removes all keywords from a form.

**Example:**
```
ClearKeywords WeapIronDagger
```

### PrintKeywords
```
PrintKeywords form:ref
```
Prints all keywords for a form to the console. Returns the number of keywords.

**Example:**
```
PrintKeywords WeapIronDagger
```

## Usage Examples

### Example 1: Weapon Classification System

```
scn MyWeaponClassifierScript

ref weapon

begin GameMode
    ; Classify all iron weapons on first run
    if GetGameLoaded
        set weapon to GetFormFromMod "Oblivion.esm" "00000001"  ; Iron Dagger
        AddKeyword weapon "Weapon"
        AddKeyword weapon "Blade"
        AddKeyword weapon "OneHanded"
        AddKeyword weapon "Metal"
        AddKeyword weapon "Cheap"
    endif
end
```

### Example 2: Custom Merchant Filter

```
scn MerchantFilterScript

ref item
short index

begin GameMode
    set index to 0
    set item to player.GetInventoryObject index
    
    while item
        ; Only buy weapons and armor
        if HasAnyKeyword item "Weapon" "Armor"
            ; But not broken items
            if HasKeyword item "Broken" == 0
                ; Add to merchant's buying list
                ; (your custom logic here)
            endif
        endif
        
        set index to index + 1
        set item to player.GetInventoryObject index
    loop
end
```

### Example 3: Quest Item Protection

```
scn QuestItemScript

ref item

begin OnAdd
    set item to GetSelf
    
    if HasKeyword item "QuestItem"
        Message "This item cannot be removed."
        player.RemoveItem item 1 1
        AddItem item 1
    endif
end
```

### Example 4: Dynamic Loot Tables

```
scn LootTableScript

ref container
ref item
short random

begin OnActivate
    ; 50% chance for weapon or armor
    set random to GetRandomPercent
    
    if random < 50
        ; Add a random weapon
        ; (Search your forms for those with "Weapon" keyword)
    else
        ; Add random armor
        ; (Search your forms for those with "Armor" keyword)
    endif
end
```

## Notes

- Keywords are case-insensitive ("Weapon" = "weapon" = "WEAPON")
- Keywords persist across save/load
- Keywords are stored per-form, not per-instance
- Empty keywords are ignored
- Opcode base is 0x2700 (can be changed in main.cpp if conflicts occur)

## Integration Tips

1. **Initialization Script**: Create a quest script that runs once to tag all vanilla items with appropriate keywords.

2. **Mod Compatibility**: Other mods can use your keyword system! Just distribute the DLL and document your keyword conventions.

3. **Performance**: HasKeyword and HasAnyKeyword are fast O(log n) lookups. Safe for frequent use.

4. **Naming Convention**: Use PascalCase for consistency (e.g., "WeaponBlade", "ArmorHeavy", "QuestItem").

## Troubleshooting

- **Commands not found**: Check OBSE is loading the plugin. Look for "KeywordPlugin loaded successfully" in obse.log
- **Keywords not saving**: Ensure serialization interface is working. Check for errors in KeywordPlugin.log
- **Crashes**: Verify you're using the correct OBSE version and Oblivion version

## License

Free to use and modify for any purpose. Credit appreciated but not required.

## Version History

- v1.0: Initial release
  - Basic keyword add/remove/check functionality
  - Serialization support
  - 9 script commands
