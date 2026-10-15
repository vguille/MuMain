#include "stdafx.h"
#include "MainFrameExtras.h"

#include "Audio/DSPlaySound.h"
#include "Core/Utilities/StringUtils.h"
#include "Engine/Object/ZzzCharacter.h"
#include "I18N/All.h"
#include "UI/Chat/ClassicChat.h"
#include "UI/Core/WindowSystem.h"

#include <RmlUi/Core/DataModelHandle.h>

#include <cwchar>
#include <string>

namespace UI::Hud::MainFrameExtras
{
namespace
{
// 0.97's hint texts for the chat's three buttons: chat on/off (F2), size (F4), transparency.
constexpr int ChatButtonCount = 3;
constexpr int ChatButtonFirstHintText = 750;

// A bound text and the source it was made from, so a frame whose source has not changed costs a
// comparison instead of a conversion.
struct WideText
{
    std::wstring source;
    Rml::String text;
};

struct Model
{
    Rml::String coordX;
    Rml::String coordY;
    bool partyOpen = false;
    bool guildOpen = false;
    WideText partyTooltip;
    WideText guildTooltip;
    bool chatButtons = false;
    bool chatShown = true;
    WideText chatHints[ChatButtonCount];
    int heroX = -1;
    int heroY = -1;
};

Model s_model;
Rml::DataModelHandle s_handle;

void Toggle(mu::ui::window::INTERFACE_LIST window)
{
    g_pNewUISystem->Toggle(window);
    PlayBuffer(SOUND_CLICK01);
}

void SyncText(Rml::String& field, const char* name, Rml::String value)
{
    if (field == value)
        return;
    field = std::move(value);
    s_handle.DirtyVariable(name);
}

void SyncWide(WideText& field, const char* name, const wchar_t* source)
{
    if (source == nullptr || std::wcscmp(field.source.c_str(), source) == 0)
        return;
    field.source = source;
    field.text = StringUtils::WideToNarrow(source);
    s_handle.DirtyVariable(name);
}

void SyncBool(bool& field, const char* name, bool value)
{
    if (field == value)
        return;
    field = value;
    s_handle.DirtyVariable(name);
}
} // namespace

void BindModel(Rml::DataModelConstructor& constructor)
{
    constructor.Bind("coord_x", &s_model.coordX);
    constructor.Bind("coord_y", &s_model.coordY);
    constructor.Bind("party_open", &s_model.partyOpen);
    constructor.Bind("guild_open", &s_model.guildOpen);
    constructor.Bind("party_tooltip", &s_model.partyTooltip.text);
    constructor.Bind("guild_tooltip", &s_model.guildTooltip.text);
    constructor.BindEventCallback("mainframe_party_click",
        [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Toggle(mu::ui::window::INTERFACE_PARTY); });
    constructor.BindEventCallback("mainframe_guild_click",
        [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Toggle(mu::ui::window::INTERFACE_GUILDINFO); });
    constructor.Bind("chat_buttons_shown", &s_model.chatButtons);
    constructor.Bind("chat_shown", &s_model.chatShown);
    constructor.Bind("chat_hint_0", &s_model.chatHints[0].text);
    constructor.Bind("chat_hint_1", &s_model.chatHints[1].text);
    constructor.Bind("chat_hint_2", &s_model.chatHints[2].text);
    constructor.BindEventCallback("mainframe_chat_toggle_click",
        [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { UI::Chat::ClassicChat::ToggleChat(); });
    constructor.BindEventCallback("mainframe_chat_size_click",
        [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { UI::Chat::ClassicChat::StepSize(); });
    constructor.BindEventCallback("mainframe_chat_alpha_click",
        [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { UI::Chat::ClassicChat::StepTransparency(); });
    s_handle = constructor.GetModelHandle();
}

void SyncModel()
{
    if (!s_handle)
        return;

    // The coordinates are formatted only when the hero moves to another tile.
    if (Hero != nullptr && (Hero->PositionX != s_model.heroX || Hero->PositionY != s_model.heroY))
    {
        s_model.heroX = Hero->PositionX;
        s_model.heroY = Hero->PositionY;
        SyncText(s_model.coordX, "coord_x", std::to_string(s_model.heroX));
        SyncText(s_model.coordY, "coord_y", std::to_string(s_model.heroY));
    }
    SyncBool(s_model.partyOpen, "party_open", g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_PARTY));
    SyncBool(s_model.guildOpen, "guild_open", g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_GUILDINFO));
    SyncWide(s_model.partyTooltip, "party_tooltip", I18N::Game::PartyP);
    SyncWide(s_model.guildTooltip, "guild_tooltip", I18N::Game::GuildG);

    UI::Chat::ClassicChat::Sync();
    SyncBool(s_model.chatButtons, "chat_buttons_shown", UI::Chat::ClassicChat::ButtonsShown());
    SyncBool(s_model.chatShown, "chat_shown", UI::Chat::ClassicChat::IsChatShown());
    static const char* const hintNames[ChatButtonCount] = {"chat_hint_0", "chat_hint_1", "chat_hint_2"};
    for (int i = 0; i < ChatButtonCount; ++i)
        SyncWide(s_model.chatHints[i], hintNames[i], I18N::Game::Lookup(ChatButtonFirstHintText + i));
}
} // namespace UI::Hud::MainFrameExtras
