#include "stdafx.h"
#include "ThemeCapability.h"

#include "Core/Utilities/StringUtils.h"
#include "UI/RmlBridge/RmlTheme.h"
#include "UI/RmlBridge/ThemeInheritance.h"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace UI::RmlBridge
{
namespace
{
constexpr const char* ThemesRoot = "Data/Interface/RmlUi/themes";

// Answers per key for the theme they were read under; some callers ask every frame, so a cached
// key is found without building a string (std::less<> compares the view in place).
std::string s_cachedTheme;
std::map<std::wstring, bool, std::less<>> s_cache;

bool ReadCapability(const wchar_t* key)
{
    const UI::RmlBridge::ThemeInheritance theme(std::filesystem::path(ThemesRoot) / UI::RmlBridge::GetActiveThemeName());
    const auto setting = theme.GetSetting("Capabilities", StringUtils::WideToNarrow(key));
    return setting && !setting->value.empty() && setting->value != "0";
}
} // namespace

bool HasThemeCapability(const wchar_t* key)
{
    const std::string& theme = UI::RmlBridge::GetActiveThemeName();
    if (theme != s_cachedTheme)
    {
        s_cachedTheme = theme;
        s_cache.clear();
    }
    const auto found = s_cache.find(std::wstring_view(key));
    if (found != s_cache.end())
        return found->second;
    const bool value = ReadCapability(key);
    s_cache.emplace(key, value);
    return value;
}
} // namespace UI::RmlBridge
