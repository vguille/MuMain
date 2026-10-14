#pragma once

#include "UI/Chat/MessageType.h"

namespace mu::ui::window
{
class CChatLogWindow;
}

// The classic (0.97) chat's behaviour over MuMain's chat log. 0.97 has one message list
// that is either six frameless lines at the top-left corner or a framed window ("nis" frame) above
// the input box, and its keys differ from S6's:
//   F2  hides / shows the normal chat (whispers, party, guild and system stay), S6 shows whispers only;
//   F4  cycles frameless -> framed 3 lines -> 6 -> big -> frameless, S6 only resizes the frame (F5 shows it).
// Three buttons above the bar (chat, size, transparency) fade in under the mouse. The look is the
// classic theme's (chat_log.rcss, workspace.rcss, main_frame.rml/.rcss); this module only sets
// classes and drives CChatLogWindow. Everything is a no-op unless the theme turns it on with
// theme.ini [Capabilities] ClassicChat=1.
namespace UI::Chat::ClassicChat
{
// True while the active theme declares ClassicChat=1.
bool IsActive();

// What the chat log lists while F2 hides the normal chat (CChatInputBox owns that state, in every
// theme): S6 lists only the whispers; with ClassicChat it keeps listing every message and the
// theme hides the normal lines (the classic-chat-silenced class).
constexpr mu::ui::window::MESSAGE_TYPE ListedMessages(bool normalChatHidden, bool classicChat)
{
    return normalChatHidden && !classicChat ? mu::ui::window::TYPE_WHISPER_MESSAGE : mu::ui::window::TYPE_ALL_MESSAGE;
}

// From the top of CChatInputBox::UpdateKeyEvent (which runs whether or not the box is open):
// handles F4 the 0.97 way. True when it used the key, so the S6 handling is skipped.
bool UpdateKeys(mu::ui::window::CChatLogWindow* chatLog);

// The buttons above the bar. The first one is F2's.
void ToggleChat();
void StepSize();
void StepTransparency();

bool IsChatShown();

// Once a frame (from UI::Hud::MainFrameExtras::SyncModel): the frame / silence classes on the
// workspace and the chat log, and whether the mouse is where the theme shows the buttons.
void Sync();
bool ButtonsShown();
} // namespace UI::Chat::ClassicChat
