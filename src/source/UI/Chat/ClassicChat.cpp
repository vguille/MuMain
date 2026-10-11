#include "stdafx.h"
#include "ClassicChat.h"

#include "Audio/DSPlaySound.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include "UI/Core/WindowSystem.h"
#include "UI/HUD/ChatLogWindow.h"
#include "UI/Placement/WindowPlacement.h"
#include "UI/RmlBridge/ThemeCapability.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

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

// 0.97's frame is 640x480 and its bar (#hud_layout) the bottom 48 units of it.
constexpr float OriginalWidth = 640.f;
constexpr float BarTop = 432.f;
constexpr float BarHeight = 48.f;
// 0040e230: the buttons fade in over the bar's middle, from 4 units under its top, and fade out
// once the pointer goes above them.
constexpr float ButtonsHoverLeft = 160.f;
constexpr float ButtonsHoverRight = 480.f;
constexpr float ButtonsHoverTop = BarTop + 4.f;
constexpr float ButtonsHideAbove = 416.f;
// The chat input's depth: over the HUD (main_frame, 10.6), under the tooltips (10.69).
constexpr float InputOverHudDepth = 10.65f;

bool s_chatShown = true; // 0.97's DAT_00559bf1: normal chat shown
bool s_buttonsShown = false;
bool s_framedApplied = false;
bool s_silencedApplied = false;
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

// Sets `className` on the first element matching `selector` in `file`; false when it is not there.
bool SetClass(std::string_view file, const char* selector, const char* className, bool on)
{
    Rml::ElementDocument* document = FindDocument(file);
    if (document == nullptr)
        return false;
    Rml::Element* element = document->QuerySelector(selector);
    if (element == nullptr)
        return false;
    element->SetClass(className, on);
    return true;
}

} // namespace

bool IsActive()
{
    return UI::RmlBridge::HasThemeCapability(L"ClassicChat");
}

void ToggleChat()
{
    s_chatShown = !s_chatShown;
    PlayBuffer(SOUND_CLICK01);
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
    return s_chatShown;
}

bool UpdateKeys(mu::ui::window::CChatLogWindow* chatLog)
{
    if (!IsActive() || chatLog == nullptr)
        return false;
    if (mu::ui::window::IsPress(VK_F2))
    {
        ToggleChat();
        return true;
    }
    if (mu::ui::window::IsPress(VK_F4))
    {
        StepSize();
        return true;
    }
    return false;
}

void Sync()
{
    const bool active = IsActive();
    mu::ui::window::CChatLogWindow* chatLog = ChatLog();

    // The framed window stands above the input box (workspace.rcss); the frameless list stays in
    // the corner. Re-place on a change.
    const bool framed = active && chatLog != nullptr && chatLog->IsShowFrame();
    if (framed != s_framedApplied && SetClass("workspace.rml", ".chat-stack", "classic-chat-framed", framed))
    {
        s_framedApplied = framed;
        UI::Placement::Invalidate();
    }
    if (s_scrollFrames > 0 && --s_scrollFrames == 0 && chatLog != nullptr)
        chatLog->Scrolling(static_cast<int>(chatLog->GetNumberOfLines(chatLog->GetCurrentMsgType())));

    const bool silenced = active && !s_chatShown;
    if (silenced != s_silencedApplied && SetClass("chat_log.rml", "#panel", "classic-chat-silenced", silenced))
        s_silencedApplied = silenced;

    // 0.97 draws the input box over the bar (y 415-444, its foot on the bar's top); MuMain stacks
    // the HUD (main_frame, 10.6) over it (RmlStackingOrder, set on load), so it is raised just above
    // the HUD and under the tooltips (10.69). A theme reload restores the default, hence every frame.
    if (Rml::ElementDocument* input = active ? FindDocument("chat_input.rml") : nullptr)
    {
        const Rml::Property* zIndex = input->GetLocalProperty(Rml::PropertyId::ZIndex);
        if (zIndex == nullptr || zIndex->Get<float>() != InputOverHudDepth)
            input->SetProperty(Rml::PropertyId::ZIndex, Rml::Property(InputOverHudDepth, Rml::Unit::NUMBER));
    }

    // 0.97's 0040e230: the buttons show while the mouse is over the bar's middle (x 160..480 from
    // 4 units under its top) and hide once it leaves upwards past them (y < 416), wherever x is.
    Rml::ElementDocument* hud = active ? FindDocument("main_frame.rml") : nullptr;
    Rml::Element* bar = hud != nullptr ? hud->GetElementById("hud_layout") : nullptr;
    const Rml::Vector2f size = bar != nullptr ? bar->GetBox().GetSize(Rml::BoxArea::Border) : Rml::Vector2f(0.f, 0.f);
    if (size.x <= 0.f || size.y <= 0.f)
    {
        s_buttonsShown = false;
        return;
    }
    // #hud_layout is the 640x48 bar; Project() undoes the workspace's scale on it.
    Rml::Vector2f point(g_fWindowMouseX, g_fWindowMouseY);
    if (!bar->Project(point))
        return;
    const Rml::Vector2f offset = bar->GetAbsoluteOffset(Rml::BoxArea::Border);
    const float x = (point.x - offset.x) * OriginalWidth / size.x;
    const float y = BarTop + (point.y - offset.y) * BarHeight / size.y;
    if (x >= ButtonsHoverLeft && x < ButtonsHoverRight && y >= ButtonsHoverTop)
        s_buttonsShown = true;
    else if (y < ButtonsHideAbove)
        s_buttonsShown = false;
}

bool ButtonsShown()
{
    return s_buttonsShown;
}
} // namespace UI::Chat::ClassicChat
