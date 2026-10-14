#include <doctest.h>

#include "UI/Chat/ClassicChat.h"

using UI::Chat::ClassicChat::ListedMessages;
using mu::ui::window::TYPE_ALL_MESSAGE;
using mu::ui::window::TYPE_WHISPER_MESSAGE;

TEST_CASE("the normal chat shown lists every message in every theme [ui][chat]")
{
    CHECK(ListedMessages(false, false) == TYPE_ALL_MESSAGE);
    CHECK(ListedMessages(false, true) == TYPE_ALL_MESSAGE);
}

TEST_CASE("F2's one state follows the theme it is listed under [ui][chat]")
{
    // F2 pressed under legacy: whispers only. Switching to classic keeps the state but lists every
    // message (the theme hides the normal lines); switching back lists whispers only again.
    CHECK(ListedMessages(true, false) == TYPE_WHISPER_MESSAGE);
    CHECK(ListedMessages(true, true) == TYPE_ALL_MESSAGE);
    CHECK(ListedMessages(true, false) == TYPE_WHISPER_MESSAGE);
}
