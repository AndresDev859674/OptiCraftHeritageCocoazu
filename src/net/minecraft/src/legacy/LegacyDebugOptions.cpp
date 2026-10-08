#include "LegacyDebugOptions.h"

#include "LegacyGuiButton.h"
#include "LegacyOptionCheckbox.h"
#include "LegacyOptionLabel.h"
#include "net/minecraft/src/Entity.h"
#include "net/minecraft/src/EntityDragon.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/PlayerController.h"
#include "net/minecraft/src/PlayerControllerCreative.h"
#include "net/minecraft/src/PlayerControllerSP.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldInfo.h"
#include "pc/lwjgl/Keyboard.h"

namespace
{
enum LegacyDebugButtonId
{
    BUTTON_SHOW_FPS = 700,
    BUTTON_EXTENDED_INFO = 701,
    BUTTON_SET_DAY = 702,
    BUTTON_GAME_MODE = 703,
    BUTTON_KEEP_INVENTORY = 704,
    BUTTON_KILL_ENTITIES = 705,
    BUTTON_PREVIOUS = 706,
    BUTTON_NEXT = 707,
    BUTTON_NOCLIP = 708,
    BUTTON_DONE = 799
};

constexpr long_t TICKS_PER_DAY = 24000LL;
constexpr long_t DAY_TIME = 1000LL;
}

LegacyDebugOptions::LegacyDebugOptions(GuiScreen *parent, GameSettings *settingsValue,
    LegacyOptionsBackgroundMode backgroundModeValue)
    : LegacyOptionsScreen(parent, settingsValue, backgroundModeValue),
      showFpsCheckbox(nullptr), extendedInfoCheckbox(nullptr), noClipCheckbox(nullptr),
      keepInventoryCheckbox(nullptr), setDayButton(nullptr), gameModeButton(nullptr),
      killEntitiesButton(nullptr), multiplayer(false), currentPage(0)
{
}

void LegacyDebugOptions::initGui()
{
    configureLegacyLayout(6, true, LegacyOptionsLayoutPreset::Compact);
    multiplayer = mc != nullptr && mc->isMultiplayerWorld();
    currentPage = 0;
    rebuildPage();
}

void LegacyDebugOptions::rebuildPage()
{
    for (GuiButton *button : controlList)
        delete button;
    controlList.clear();
	hoveredControlIndex = -1;
	selectedControlIndex = -1;
	showFpsCheckbox = nullptr;
	extendedInfoCheckbox = nullptr;
	noClipCheckbox = nullptr;
	keepInventoryCheckbox = nullptr;
	setDayButton = nullptr;
	gameModeButton = nullptr;
	killEntitiesButton = nullptr;

    const int_t x = legacyLayout.contentX;
    const int_t w = legacyLayout.contentWidth;
    const int_t h = legacyLayout.rowHeight;

    if (currentPage == 0)
    {
        showFpsCheckbox = new LegacyOptionCheckbox(BUTTON_SHOW_FPS, x, legacyLayout.rowY(0), w, h,
            "Show FPS", settings->showFps);
        extendedInfoCheckbox = new LegacyOptionCheckbox(BUTTON_EXTENDED_INFO, x, legacyLayout.rowY(1), w, h,
            "F3 Extended Info", settings->showDebugInfo);
        gameModeButton = new LegacyGuiButton(BUTTON_GAME_MODE, x, legacyLayout.rowY(2), w, h, "");
        noClipCheckbox = new LegacyOptionCheckbox(BUTTON_NOCLIP, x, legacyLayout.rowY(3), w, h,
            "Noclip", mc != nullptr && mc->thePlayer != nullptr && mc->thePlayer->noClip);
        controlList.push_back(showFpsCheckbox);
        controlList.push_back(extendedInfoCheckbox);
        controlList.push_back(gameModeButton);
        controlList.push_back(noClipCheckbox);
    }
    else
    {
        setDayButton = new LegacyGuiButton(BUTTON_SET_DAY, x, legacyLayout.rowY(0), w, h, "Set Day");
        keepInventoryCheckbox = new LegacyOptionCheckbox(BUTTON_KEEP_INVENTORY, x, legacyLayout.rowY(1), w, h,
            "Keep Inventory", settings->debugKeepInventory);
        killEntitiesButton = new LegacyGuiButton(BUTTON_KILL_ENTITIES, x, legacyLayout.rowY(2), w, h, "Kill Entities");
        controlList.push_back(setDayButton);
        controlList.push_back(keepInventoryCheckbox);
        controlList.push_back(killEntitiesButton);
    }

    const int_t navY = legacyLayout.rowY(4);
    const int_t navGap = 2;
    const int_t navWidth = (w - navGap) / 2;
    LegacyGuiButton *previous = new LegacyGuiButton(BUTTON_PREVIOUS, x, navY, navWidth, h, "Previous");
    LegacyGuiButton *next = new LegacyGuiButton(BUTTON_NEXT, x + navWidth + navGap,
        navY, w - navWidth - navGap, h, "Next");
    previous->enabled = currentPage > 0;
    next->enabled = currentPage < 1;
    controlList.push_back(previous);
    controlList.push_back(next);
    controlList.push_back(new LegacyGuiButton(BUTTON_DONE, x, legacyLayout.rowY(5), w, h, "Done"));

    syncControls();
}

void LegacyDebugOptions::syncControls()
{
    if (showFpsCheckbox != nullptr)
        showFpsCheckbox->setChecked(settings->showFps);
    if (extendedInfoCheckbox != nullptr)
        extendedInfoCheckbox->setChecked(settings->showDebugInfo);
    if (keepInventoryCheckbox != nullptr)
        keepInventoryCheckbox->setChecked(settings->debugKeepInventory);
    // if (noClipCheckbox != nullptr)
    // {
    //     const bool spectator = mc != nullptr && mc->playerController != nullptr &&
    //         mc->playerController->isSpectatorMode();
    //     noClipCheckbox->setChecked(mc != nullptr && mc->thePlayer != nullptr && mc->thePlayer->noClip);
    //     noClipCheckbox->enabled = !multiplayer && !spectator && mc != nullptr &&
    //         mc->playerController != nullptr && mc->playerController->isInCreativeMode();
    // }
    if (gameModeButton != nullptr)
    {
        const bool creative = mc != nullptr && mc->playerController != nullptr &&
            mc->playerController->isInCreativeMode();
        const bool spectator = mc != nullptr && mc->playerController != nullptr &&
            mc->playerController->isSpectatorMode();
        gameModeButton->displayString = std::string("Game Mode: ") +
            (spectator ? "Spectator" : (creative ? "Creative" : "Survival"));
    }
}

void LegacyDebugOptions::setDay()
{
    if (multiplayer || mc == nullptr || mc->theWorld == nullptr)
        return;

    const long_t time = mc->theWorld->getWorldTime();
    const long_t dayStart = time - time % TICKS_PER_DAY;
    mc->theWorld->setWorldTime(dayStart + DAY_TIME);
}

void LegacyDebugOptions::killEntities()
{
    if (multiplayer || mc == nullptr || mc->theWorld == nullptr)
        return;

    std::vector<Entity *> entities = mc->theWorld->getLoadedEntityList();
    for (Entity *entity : entities)
    {
        if (entity == nullptr || entity->isPlayer() || entity->isDead)
            continue;

        EntityDragon *dragon = dynamic_cast<EntityDragon *>(entity);
        if (dragon != nullptr)
        {
            dragon->setHealth(0);
            continue;
        }

        mc->theWorld->setEntityDead(entity);
    }
}

void LegacyDebugOptions::setGameMode(int_t gameType)
{
    if (multiplayer || mc == nullptr || mc->theWorld == nullptr || mc->thePlayer == nullptr)
        return;

    const int_t currentGameType = mc->playerController != nullptr && mc->playerController->isSpectatorMode()
        ? 3 : (mc->playerController != nullptr && mc->playerController->isInCreativeMode() ? 1 : 0);
    if (currentGameType == gameType)
        return;

    PlayerController *oldController = mc->playerController;
    mc->playerController = PlayerController::createForGameType(mc, gameType);
    delete oldController;
	if (gameType != 3)
		mc->thePlayer->noClip = false;

    mc->playerController->onWorldChanged(mc->theWorld);
    mc->playerController->initializePlayer(mc->thePlayer);

    WorldInfo *worldInfo = mc->theWorld->getWorldInfo();
    if (worldInfo != nullptr)
        worldInfo->setGameType(gameType);
}

void LegacyDebugOptions::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == BUTTON_PREVIOUS && currentPage > 0)
    {
        --currentPage;
        rebuildPage();
        return;
    }
    if (button->id == BUTTON_NEXT && currentPage < 1)
    {
        ++currentPage;
        rebuildPage();
        return;
    }

    switch (button->id)
    {
    case BUTTON_SHOW_FPS:
        settings->showFps = !settings->showFps;
        syncControls();
        return;
    case BUTTON_EXTENDED_INFO:
        settings->showDebugInfo = !settings->showDebugInfo;
        syncControls();
        return;
    case BUTTON_SET_DAY:
        setDay();
        return;
    case BUTTON_GAME_MODE:
        {
		const int_t currentGameType = mc != nullptr && mc->playerController != nullptr &&
			mc->playerController->isSpectatorMode() ? 3 :
			(mc != nullptr && mc->playerController != nullptr && mc->playerController->isInCreativeMode() ? 1 : 0);
		setGameMode(currentGameType == 0 ? 1 : (currentGameType == 1 ? 3 : 0));
        syncControls();
        return;
        }
        case BUTTON_NOCLIP:
		if (!multiplayer && mc != nullptr && mc->thePlayer != nullptr && mc->playerController != nullptr &&
			mc->playerController->isInCreativeMode())
			mc->thePlayer->noClip = !mc->thePlayer->noClip;
		syncControls();
		return;
    case BUTTON_KEEP_INVENTORY:
        settings->debugKeepInventory = !settings->debugKeepInventory;
        syncControls();
        return;
    case BUTTON_KILL_ENTITIES:
        killEntities();
        return;
    case BUTTON_DONE:
        returnToParent();
        return;
    default:
        return;
    }
}

void LegacyDebugOptions::keyTyped(char_t c, int_t key)
{
    if (key == lwjgl::Keyboard::KEY_F3)
    {
        mc->displayGuiScreen(nullptr);
        return;
    }
    LegacyOptionsScreen::keyTyped(c, key);
}

void LegacyDebugOptions::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawLegacyBackground(partialTick);
    legacyDrawOptionLabel(fontRenderer, "Debug Cheats", legacyLayout.contentX,
        legacyLayout.rowY(2) + (legacyLayout.rowHeight - 8) / 2);
    updateLegacyPointerHover(mouseX, mouseY);
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
