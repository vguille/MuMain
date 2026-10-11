#pragma once

// Capabilities the active RmlUi theme declares in its theme.ini, e.g.
//
//   [Capabilities]
//   ClassicChat=1
//
// The nearest theme in the chain (UI/RmlBridge/ThemeInheritance.h) that declares the key decides.
namespace UI::RmlBridge
{
// True when the active theme (or a theme it builds on) declares `key` with a non-zero value. Cached
// per theme, so it is cheap enough to ask every frame.
bool HasThemeCapability(const wchar_t* key);
} // namespace UI::RmlBridge
