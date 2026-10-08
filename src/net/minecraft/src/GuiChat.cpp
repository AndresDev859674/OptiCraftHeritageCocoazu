#include "GuiChat.h"

#include <algorithm>

#include "ChatClickData.h"
#include "EntityClientPlayerMP.h"
#include "EntityPlayerSP.h"
#include "GuiChatConfirmLink.h"
#include "GuiIngame.h"
#include "GuiPlayerInfo.h"
#include "GuiTextField.h"
#include "Minecraft.h"
#include "PlayerControllerCreative.h"
#include "PlayerControllerSP.h"
#include "NetClientHandler.h"
#include "World.h"
#include "WorldInfo.h"
#include "java/String.h"
#include "java/System.h"
#include "pc/lwjgl/Keyboard.h"
#include "pc/lwjgl/Mouse.h"
#include <cctype>
#include <sstream>

namespace
{
    std::string lowerChatToken(std::string token)
    {
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char value)
        {
            return static_cast<char>(std::tolower(value));
        });
        return token;
    }

    bool executeSingleplayerCommand(Minecraft *mc, const std::string &message)
    {
        if (mc == nullptr || mc->theWorld == nullptr || mc->thePlayer == nullptr)
            return false;

        std::istringstream input(message.substr(1));
        std::string command;
        if (!(input >> command))
        {
            mc->ingameGUI->addChatMessage("Usage: /time set <ticks> or /gamemode <survival|creative|spectator> [username]");
            return true;
        }
        command = lowerChatToken(command);

        if (command == "time")
        {
            std::string action;
            std::string value;
            std::string extra;
            if (!(input >> action >> value) || lowerChatToken(action) != "set" || (input >> extra))
            {
                mc->ingameGUI->addChatMessage("Usage: /time set <ticks>");
                return true;
            }

            try
            {
                std::size_t consumed = 0;
                const long_t ticks = static_cast<long_t>(std::stoll(value, &consumed));
                if (consumed != value.size())
                    throw std::invalid_argument("invalid ticks");
                mc->theWorld->setWorldTime(ticks);
                mc->ingameGUI->addChatMessage("Set time to " + std::to_string(ticks));
            }
            catch (const std::exception &)
            {
                mc->ingameGUI->addChatMessage("Time must be an integer number of ticks");
            }
            return true;
        }

        if (command == "gamemode")
        {
            std::string mode;
            std::string target;
            std::string extra;
            const bool hasMode = static_cast<bool>(input >> mode);
            WorldInfo *worldInfo = mc->theWorld->getWorldInfo();
            const int_t currentGameType = worldInfo != nullptr ? worldInfo->getGameType() :
                (mc->playerController->isSpectatorMode() ? 3 :
                    (mc->playerController->isInCreativeMode() ? 1 : 0));
            int_t gameType = currentGameType;
            if (hasMode)
            {
                const std::string normalizedMode = lowerChatToken(mode);
                if (normalizedMode == "survival" || mode == "0")
                    gameType = 0;
                else if (normalizedMode == "creative" || mode == "1")
                    gameType = 1;
                else if (normalizedMode == "spectator" || mode == "3")
                    gameType = 3;
                else
                {
                    mc->ingameGUI->addChatMessage("Usage: /gamemode <survival|creative|spectator> [username]");
                    return true;
                }
            }
            else
                gameType = currentGameType == 0 ? 1 : (currentGameType == 1 ? 3 : 0);

            if (input >> target)
            {
                if ((input >> extra) || !String::equalsIgnoreCaseJava(target, mc->thePlayer->username))
                {
                    mc->ingameGUI->addChatMessage("That username is not in this single-player world");
                    return true;
                }
            }

            if (gameType != currentGameType)
            {
                PlayerController *oldController = mc->playerController;
                mc->playerController = PlayerController::createForGameType(mc, gameType);
                delete oldController;
                if (gameType != 3)
                    mc->thePlayer->noClip = false;
                mc->playerController->onWorldChanged(mc->theWorld);
                mc->playerController->initializePlayer(mc->thePlayer);
            }
            if (worldInfo != nullptr)
                worldInfo->setGameType(gameType);
            const char *modeName = gameType == 3 ? "spectator" : (gameType == 1 ? "creative" : "survival");
            mc->ingameGUI->addChatMessage(std::string("Set game mode to ") + modeName);
            return true;
        }

        mc->ingameGUI->addChatMessage("Unknown command: /" + command);
        return true;
    }
}

GuiChat::GuiChat()
    : historyBuffer()
    , sentHistoryCursor(-1)
    , playerNamesFound(false)
    , autocompleteIndex(0)
    , autocompleteNames()
    , pendingUrl()
    , messageField(nullptr)
    , initialMessage()
{
}

GuiChat::GuiChat(const std::string &message)
    : historyBuffer()
    , sentHistoryCursor(-1)
    , playerNamesFound(false)
    , autocompleteIndex(0)
    , autocompleteNames()
    , pendingUrl()
    , messageField(nullptr)
    , initialMessage(message)
{
}

GuiChat::~GuiChat()
{
    delete messageField;
}

void GuiChat::initGui()
{
    lwjgl::Keyboard::enableRepeatEvents(true);
    sentHistoryCursor = (int_t)mc->ingameGUI->getSentMessages().size();
    delete messageField;
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)
    const int_t fieldY = height - 124;
#else
    const int_t fieldY = height - 12;
#endif
    messageField = new GuiTextField(this, fontRenderer, 4, fieldY, width - 4, 12, "");
    messageField->setMaxStringLength(100);
    messageField->setEnableBackgroundDrawing(false);
    messageField->setFocused(true);
    messageField->setText(initialMessage);
    messageField->setCanLoseFocus(false);
}

void GuiChat::onGuiClosed()
{
    lwjgl::Keyboard::enableRepeatEvents(false);
    mc->ingameGUI->resetChatScroll();
    if (messageField != nullptr)
        messageField->setFocused(false);
}

void GuiChat::updateScreen()
{
    if (messageField != nullptr)
        messageField->updateCursorCounter();
}

void GuiChat::keyTyped(char_t c, int_t key)
{
    if (key == lwjgl::Keyboard::KEY_TAB)
        completePlayerName();
    else
        playerNamesFound = false;

    if (key == lwjgl::Keyboard::KEY_ESCAPE)
    {
        mc->displayGuiScreen(nullptr);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_RETURN)
    {
        std::string message = String::trimJava(messageField != nullptr ? messageField->getText() : "");
        if (!message.empty())
        {
            if (!mc->isMultiplayerWorld() && mc->lineIsCommand(message))
            {
                std::vector<std::string> &history = mc->ingameGUI->getSentMessages();
                if (history.empty() || history.back() != message)
                    history.push_back(message);
                executeSingleplayerCommand(mc, message);
            }
            else
            {
                mc->thePlayer->sendChatMessage(message);
            }
        }
        mc->displayGuiScreen(nullptr);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_UP)
    {
        getSentHistory(-1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_DOWN)
    {
        getSentHistory(1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_PRIOR)
    {
        mc->ingameGUI->scrollChat(19);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_NEXT)
    {
        mc->ingameGUI->scrollChat(-19);
        return;
    }
    if (messageField != nullptr)
        messageField->textboxKeyTyped(c, key);
}

void GuiChat::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    int_t wheel = lwjgl::Mouse::getEventDWheel();
    if (wheel == 0)
        return;
    if (wheel > 1) wheel = 1;
    if (wheel < -1) wheel = -1;
    if (!GuiScreen::isShiftKeyDown())
        wheel *= 7;
    mc->ingameGUI->scrollChat(wheel);
}

void GuiChat::mouseClicked(int_t x, int_t y, int_t button)
{
    if (button == 0)
    {
        ChatClickData *click = mc->ingameGUI->getChatClickData(lwjgl::Mouse::getX(), lwjgl::Mouse::getY());
        if (click != nullptr)
        {
            std::string url = click->getUrl();
            if (!url.empty())
            {
                pendingUrl = url;
                mc->displayGuiScreen(new GuiChatConfirmLink(this, this, click->getClickedText(), 0, click));
                return;
            }
            delete click;
        }
    }

    if (messageField != nullptr)
        messageField->mouseClicked(x, y, button);
    GuiScreen::mouseClicked(x, y, button);
}

void GuiChat::confirmClicked(bool confirmed, int_t id)
{
    if (id != 0)
        return;
    if (confirmed && !pendingUrl.empty())
        System::openURL(pendingUrl);
    pendingUrl.clear();
    mc->displayGuiScreen(this);
}

void GuiChat::completePlayerName()
{
    if (messageField == nullptr)
        return;

    if (playerNamesFound)
    {
        messageField->deleteWords(-1);
        if (autocompleteIndex >= (int_t)autocompleteNames.size())
            autocompleteIndex = 0;
    }
    else
    {
        int_t wordStart = messageField->getNthWordFromCursor(-1);
        if (messageField->getCursorPosition() - wordStart < 1)
            return;

        autocompleteNames.clear();
        std::string typed = String::substringUtf16(
            messageField->getText(), wordStart, String::utf16Length(messageField->getText()));
        std::string lower = String::toLowerCaseJava(typed);
        EntityClientPlayerMP *player = dynamic_cast<EntityClientPlayerMP *>(mc->thePlayer);
        if (player == nullptr || player->sendQueue == nullptr)
            return;

        for (GuiPlayerInfo *info : player->sendQueue->getPlayerNames())
        {
            if (info != nullptr && info->nameStartsWith(lower))
                autocompleteNames.push_back(info->name);
        }
        if (autocompleteNames.empty())
            return;

        playerNamesFound = true;
        autocompleteIndex = 0;
        messageField->deleteFromCursor(wordStart - messageField->getCursorPosition());
    }

    if (autocompleteNames.size() > 1)
    {
        std::string names;
        for (const std::string &name : autocompleteNames)
        {
            if (!names.empty()) names += ", ";
            names += name;
        }
        mc->ingameGUI->addChatMessage(names);
    }

    messageField->writeText(autocompleteNames[(std::size_t)autocompleteIndex++]);
}

void GuiChat::getSentHistory(int_t direction)
{
    if (messageField == nullptr)
        return;
    const std::vector<std::string> &history = mc->ingameGUI->getSentMessages();
    int_t next = sentHistoryCursor + direction;
    next = std::max(0, std::min(next, (int_t)history.size()));
    if (next == sentHistoryCursor)
        return;

    if (next == (int_t)history.size())
    {
        sentHistoryCursor = next;
        messageField->setText(historyBuffer);
    }
    else
    {
        if (sentHistoryCursor == (int_t)history.size())
            historyBuffer = messageField->getText();
        messageField->setText(history[(std::size_t)next]);
        sentHistoryCursor = next;
    }
}

void GuiChat::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)
    const int_t fieldY = height - 124;
#else
    const int_t fieldY = height - 14;
#endif
    drawRect(2, fieldY, width - 2, fieldY + 12, 0x80000000);
    if (messageField != nullptr)
        messageField->drawTextBox();
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}

std::string GuiChat::getMessage() const
{
    if (messageField != nullptr)
        return messageField->getText();

    return std::string();
}

void GuiChat::setMessage(const std::string &message)
{
    if (messageField != nullptr)
        messageField->setText(message);
    else
        initialMessage = message;
}
