#include "stdafx.h"
#include "ClassicChat.h"

#include "Audio/DSPlaySound.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include "UI/Core/WindowSystem.h"
#include "UI/HUD/ChatInputBox.h"
#include "UI/HUD/ChatLogWindow.h"
#include "UI/Placement/WindowPlacement.h"
#include "UI/RmlBridge/ThemeCapability.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <optional>
#include <string>
#include <string_view>

namespace UI::Chat::ClassicChat
{
namespace
{
// F4's sizes: 0.97 goes 3, 6, 30 lines; MuMain's log stops at 15 (CChatLogWindow::SetNumberOfShowingLines).
constexpr int SmallLines = 3;
constexpr int MediumLines = 6;
constexpr int BigLines = 15;
// 0.97's frameless list.
constexpr int FramelessLines = 6;

// The state the theme styles: on the workspace's and the chat log's <body>, so a reloaded document
// gets it back on the next frame. Rml::String, so the per-frame checks build no string.
const Rml::String FramedClass = "classic-chat-framed";
const Rml::String SilencedClass = "classic-chat-silenced";
// The chat's buttons and the area that shows them, in main_frame.rml.
const Rml::String ButtonsId = "chat_btns";
const Rml::String ButtonsZoneId = "chat_btns_zone";

bool s_buttonsShown = false;
// Frames until the log is scrolled to its newest line, once a resize has been laid out.
int s_scrollFrames = 0;

mu::ui::window::CChatLogWindow* ChatLog()
{
    return g_pChatListBox;
}

// True when `url`'s file name is `file` (".../file"). Runs every frame, so it compares in place.
bool IsDocumentFile(const std::string& url, std::string_view file)
{
    if (url.size() <= file.size())
        return false;
    const std::size_t start = url.size() - file.size();
    return url[start - 1] == '/' && std::string_view(url).substr(start) == file;
}

Rml::ElementDocument* FindDocument(std::string_view file)
{
    Rml::Context* context = RmlUiRuntime::Instance().GetContext();
    if (context == nullptr)
        return nullptr;
    for (int i = 0; i < context->GetNumDocuments(); ++i)
    {
        Rml::ElementDocument* document = context->GetDocument(i);
        if (IsDocumentFile(document->GetSourceURL(), file))
            return document;
    }
    return nullptr;
}

// Sets or clears `className` on `file`'s <body> when it differs. True when it changed.
bool SyncDocumentClass(std::string_view file, const Rml::String& className, bool on)
{
    Rml::ElementDocument* document = FindDocument(file);
    if (document == nullptr || document->IsClassSet(className) == on)
        return false;
    document->SetClass(className, on);
    return true;
}

// The pointer relative to `element`'s border box, in the element's layout units (Project() undoes
// the HUD's scale); none when it cannot be projected.
std::optional<Rml::Vector2f> PointerOver(Rml::Element* element)
{
    Rml::Vector2f point(g_fWindowMouseX, g_fWindowMouseY);
    if (!element->Project(point))
        return std::nullopt;
    return point - element->GetAbsoluteOffset(Rml::BoxArea::Border);
}

bool IsPointerInside(Rml::Element* element)
{
    const std::optional<Rml::Vector2f> point = PointerOver(element);
    const Rml::Vector2f size = element->GetBox().GetSize(Rml::BoxArea::Border);
    return point && point->x >= 0.f && point->y >= 0.f && point->x < size.x && point->y < size.y;
}

bool IsPointerAbove(Rml::Element* element)
{
    const std::optional<Rml::Vector2f> point = PointerOver(element);
    return point && point->y < 0.f;
}

// 0.97's 0040e230: the buttons show once the pointer is over #chat_btns_zone (0.97: the bar's
// middle) and hide once it goes above #chat_btns, wherever it is sideways. The theme places both;
// without a zone, the buttons' own box shows them.
void SyncButtons()
{
    Rml::ElementDocument* hud = FindDocument("main_frame.rml");
    Rml::Element* buttons = hud != nullptr ? hud->GetElementById(ButtonsId) : nullptr;
    if (buttons == nullptr)
    {
        s_buttonsShown = false;
        return;
    }
    Rml::Element* zone = hud->GetElementById(ButtonsZoneId);
    if (IsPointerInside(zone != nullptr ? zone : buttons))
        s_buttonsShown = true;
    else if (IsPointerAbove(buttons))
        s_buttonsShown = false;
}
} // namespace

bool IsActive()
{
    return UI::RmlBridge::HasThemeCapability(L"ClassicChat");
}

void ToggleChat()
{
    if (mu::ui::window::CChatInputBox* input = g_pChatInputBox)
        input->ToggleNormalChat();
}

// 0.97's 0040e330: frameless (6) -> framed 3 -> 6 -> big -> frameless.
void StepSize()
{
    mu::ui::window::CChatLogWindow* chatLog = ChatLog();
    if (chatLog == nullptr)
        return;
    const int lines = static_cast<int>(chatLog->GetNumberOfShowingLines());
    if (!chatLog->IsShowFrame())
    {
        chatLog->ShowFrame();
        chatLog->SetNumberOfShowingLines(SmallLines);
    }
    else if (lines < MediumLines)
        chatLog->SetNumberOfShowingLines(MediumLines);
    else if (lines < BigLines)
        chatLog->SetNumberOfShowingLines(BigLines);
    else
    {
        chatLog->HideFrame();
        chatLog->SetNumberOfShowingLines(FramelessLines);
    }
    // 0.97 shows the newest lines after a resize; the new height is laid out over the next frames.
    s_scrollFrames = 2;
    PlayBuffer(SOUND_CLICK01);
}

void StepTransparency()
{
    if (mu::ui::window::CChatLogWindow* chatLog = ChatLog())
        chatLog->SetBackAlphaAuto();
    PlayBuffer(SOUND_CLICK01);
}

bool IsChatShown()
{
    const mu::ui::window::CChatInputBox* input = g_pChatInputBox;
    return input == nullptr || input->IsNormalChatShown();
}

bool UpdateKeys(mu::ui::window::CChatLogWindow* chatLog)
{
    if (!IsActive() || chatLog == nullptr)
        return false;
    if (mu::ui::window::IsPress(VK_F4))
    {
        StepSize();
        return true;
    }
    return false;
}

void Sync()
{
    // A theme switch rebuilds every document, so the classes go with the theme that set them.
    if (!IsActive())
    {
        s_buttonsShown = false;
        return;
    }
    mu::ui::window::CChatLogWindow* chatLog = ChatLog();

    // The framed window stands above the input box (workspace.rcss); the frameless list stays in
    // the corner. Re-place on a change.
    const bool framed = chatLog != nullptr && chatLog->IsShowFrame();
    if (SyncDocumentClass("workspace.rml", FramedClass, framed))
        UI::Placement::Invalidate();
    if (s_scrollFrames > 0 && --s_scrollFrames == 0 && chatLog != nullptr)
        chatLog->Scrolling(static_cast<int>(chatLog->GetNumberOfLines(chatLog->GetCurrentMsgType())));

    SyncDocumentClass("chat_log.rml", SilencedClass, !IsChatShown());
    SyncButtons();
}

bool ButtonsShown()
{
    return s_buttonsShown;
}
} // namespace UI::Chat::ClassicChat
