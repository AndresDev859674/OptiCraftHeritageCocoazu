#include "LegacyPlayGameScreen.h"

#include <algorithm>

#include "LegacyCreateWorldScreen.h"
#include "LegacyGuiButton.h"
#include "LegacyMainMenuLayout.h"
#include "LegacyMenuHints.h"
#include "LegacyMenuNavigation.h"
#include "LegacyOptionsLayout.h"
#include "LegacyPanorama.h"
#include "LegacySceneLayout.h"
#include "LegacySceneState.h"
#include "LegacyUiAssets.h"
#include "LegacyUiTexture.h"
#include "LegacyTutorialWorld.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/Tessellator.h"
#include "platform/RenderAPI.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "pc/lwjgl/Mouse.h"

namespace
{
enum LegacyPlayButtonId
{
    BUTTON_CREATE_WORLD = 100,
    BUTTON_TUTORIAL = 101,
    BUTTON_WORLD_BASE = 200
};

constexpr int_t PLAY_PANEL_TARGET_HEIGHT = 240;
constexpr int_t PLAY_CONTENT_INSET = 10;
constexpr int_t PLAY_PANEL_MAX_WIDTH = 720;
constexpr int_t PLAY_PANEL_SCREEN_MARGIN = 24;
constexpr int_t PLAY_PANEL_MIN_WIDTH_FOR_SPLIT = 240;
constexpr int_t PLAY_PANE_GAP = 10;
constexpr int_t PLAY_PANE_CONTENT_GAP = 8;
constexpr int_t PLAY_ROW_HEIGHT = 30;
constexpr int_t PLAY_ROW_SPACING = 4;
constexpr int_t PLAY_HEADER_HEIGHT = 28;
constexpr int_t PLAY_FOOTER_HEIGHT = 28;
constexpr int_t PLAY_SCROLL_ARROW_WIDTH = 13;
constexpr int_t PLAY_SCROLL_ARROW_HEIGHT = 7;
constexpr int_t PLAY_SCROLL_ARROW_BOTTOM_INSET = 8;
constexpr int_t PLAY_PANEL_FOOTER_GAP = 14;

LegacyUiTexture g_scrollDown("/legacy/scroll_down.png");

struct LegacyPlayPaneLayout
{
    bool split;
    int_t leftX;
    int_t leftY;
    int_t leftWidth;
    int_t leftHeight;
    int_t rightX;
    int_t rightY;
    int_t rightWidth;
    int_t rightHeight;
};

// The constants above describe the desktop composition. A console screen is 240 px
// tall and already only had room for a single world row; with the logo now taking
// its Legacy share of the height it would have room for none, so the world list
// scales with the screen like the rest of the Legacy layout.
int_t playRowHeight(int_t screenHeight)
{
    return legacyScaleToScreen(screenHeight, PLAY_ROW_HEIGHT, 18, PLAY_ROW_HEIGHT);
}

int_t playHeaderHeight(int_t screenHeight)
{
    return legacyScaleToScreen(screenHeight, PLAY_HEADER_HEIGHT, 17, PLAY_HEADER_HEIGHT);
}

int_t playFooterHeight(int_t screenHeight)
{
    return legacyScaleToScreen(screenHeight, PLAY_FOOTER_HEIGHT, 14, PLAY_FOOTER_HEIGHT);
}

LegacyOptionsLayout buildPlayLayout(int_t screenWidth, int_t screenHeight, int_t rows)
{
    const LegacySceneLayout scene = legacySceneLayout(screenWidth, screenHeight);
    LegacyOptionsLayout result{};
    const int_t screenMargin = std::min<int_t>(PLAY_PANEL_SCREEN_MARGIN,
        std::max<int_t>(8, screenWidth / 20));
    result.panelWidth = std::min<int_t>(PLAY_PANEL_MAX_WIDTH,
        std::max<int_t>(1, screenWidth - screenMargin * 2));
    result.panelX = (screenWidth - result.panelWidth) / 2;
    result.rowHeight = playRowHeight(screenHeight);
    result.rowSpacing = PLAY_ROW_SPACING;
    result.contentX = result.panelX + PLAY_CONTENT_INSET;
    result.contentWidth = result.panelWidth - PLAY_CONTENT_INSET * 2;

    const int_t headerHeight = playHeaderHeight(screenHeight);
    const int_t footerHeight = playFooterHeight(screenHeight);
    const int_t actionHeight = 2 * result.rowHeight + result.rowSpacing;
    const int_t worldRows = std::max<int_t>(1, rows);
    const int_t worldHeight = worldRows * result.rowHeight +
        std::max<int_t>(0, worldRows - 1) * result.rowSpacing;
    const bool split = result.panelWidth >= PLAY_PANEL_MIN_WIDTH_FOR_SPLIT;
    const int_t minimumHeight = split
        ? 2 * PLAY_CONTENT_INSET + headerHeight + PLAY_PANE_CONTENT_GAP +
            std::max<int_t>(actionHeight, worldHeight) + PLAY_PANE_CONTENT_GAP + footerHeight
        : 2 * PLAY_CONTENT_INSET + headerHeight + PLAY_PANE_CONTENT_GAP + actionHeight +
            PLAY_PANE_GAP + headerHeight + PLAY_PANE_CONTENT_GAP + worldHeight +
            PLAY_PANE_CONTENT_GAP + footerHeight;
    const int_t footerTop = legacyHintRowY(screenHeight) - PLAY_PANEL_FOOTER_GAP;
    const int_t availableHeight = std::max<int_t>(96, footerTop - scene.contentTop);
    result.panelHeight = std::min<int_t>(PLAY_PANEL_TARGET_HEIGHT,
        std::max<int_t>(minimumHeight, std::min<int_t>(PLAY_PANEL_TARGET_HEIGHT, availableHeight)));
    result.panelY = legacyCenteredPanelY(screenWidth, screenHeight, result.panelHeight, PLAY_PANEL_FOOTER_GAP);
    result.firstRowY = result.panelY + playHeaderHeight(screenHeight);

    result.titleY = scene.titleY;
    result.titleMaxWidth = scene.titleMaxWidth;
    result.titleMaxHeight = scene.titleMaxHeight;
    return result;
}

LegacyPlayPaneLayout buildPlayPaneLayout(const LegacyOptionsLayout &layout, int_t screenHeight)
{
    LegacyPlayPaneLayout panes{};
    const int_t innerX = layout.panelX + PLAY_CONTENT_INSET;
    const int_t innerY = layout.panelY + PLAY_CONTENT_INSET;
    const int_t innerWidth = std::max<int_t>(1, layout.panelWidth - PLAY_CONTENT_INSET * 2);
    const int_t innerBottom = layout.panelY + layout.panelHeight - PLAY_CONTENT_INSET;
    panes.split = layout.panelWidth >= PLAY_PANEL_MIN_WIDTH_FOR_SPLIT;
    panes.leftX = innerX;
    panes.leftY = innerY;

    if (panes.split)
    {
        const int_t availableWidth = std::max<int_t>(1, innerWidth - PLAY_PANE_GAP);
        const int_t leftPanePercent = layout.panelWidth < 520 ? 48 : 43;
        panes.leftWidth = availableWidth * leftPanePercent / 100;
        panes.rightX = panes.leftX + panes.leftWidth + PLAY_PANE_GAP;
        panes.rightWidth = availableWidth - panes.leftWidth;
        panes.leftHeight = panes.rightHeight = std::max<int_t>(1, innerBottom - innerY);
        panes.rightY = innerY;
    }
    else
    {
        panes.leftWidth = innerWidth;
        panes.leftHeight = playHeaderHeight(screenHeight) + PLAY_PANE_CONTENT_GAP +
            2 * layout.rowHeight + layout.rowSpacing + PLAY_PANE_CONTENT_GAP;
        panes.rightX = innerX;
        panes.rightY = innerY + panes.leftHeight + PLAY_PANE_GAP;
        panes.rightWidth = innerWidth;
        panes.rightHeight = std::max<int_t>(1, innerBottom - panes.rightY);
    }
    return panes;
}

void drawPlayPaneFrames(LegacyOptionsPanel &panelRenderer, const LegacyPlayPaneLayout &panes)
{
    LegacyOptionsLayout pane{};
    pane.panelX = panes.leftX;
    pane.panelY = panes.leftY;
    pane.panelWidth = panes.leftWidth;
    pane.panelHeight = panes.leftHeight;
    panelRenderer.draw(pane);

    pane.panelX = panes.rightX;
    pane.panelY = panes.rightY;
    pane.panelWidth = panes.rightWidth;
    pane.panelHeight = panes.rightHeight;
    panelRenderer.draw(pane);
}

Block *entryIconBlock(int_t buttonId)
{
    if (buttonId == BUTTON_CREATE_WORLD)
        return Block::grass;
    if (buttonId == BUTTON_TUTORIAL)
        return Block::workbench;
    if (buttonId < BUTTON_WORLD_BASE)
        return nullptr;

    switch ((buttonId - BUTTON_WORLD_BASE) % 4)
    {
    case 0: return Block::grass;
    case 1: return Block::bookShelf;
    case 2: return Block::stone;
    default: return Block::cobblestone;
    }
}

int_t entryIconTextureSide(int_t buttonId)
{
    if (buttonId == BUTTON_CREATE_WORLD)
        return 1;
    if (buttonId == BUTTON_TUTORIAL)
        return 1;
    return 2;
}

void drawEntryIconTile(Minecraft *mc, Block *block, int_t side, int_t x, int_t y, float_t zLevel)
{
    if (mc == nullptr || mc->renderEngine == nullptr || block == nullptr)
        return;

    constexpr int_t ICON_SIZE = 16;
    constexpr float_t ATLAS_SIZE = 256.0f;
    constexpr float_t TILE_SIZE = 16.0f;
    constexpr float_t TEXEL_INSET = 0.01f;

    const int_t textureIndex = block->getBlockTextureFromSide(side);
    const int_t tileX = textureIndex & 15;
    const int_t tileY = textureIndex >> 4;
    const float_t u0 = (tileX * TILE_SIZE + TEXEL_INSET) / ATLAS_SIZE;
    const float_t v0 = (tileY * TILE_SIZE + TEXEL_INSET) / ATLAS_SIZE;
    const float_t u1 = ((tileX + 1) * TILE_SIZE - TEXEL_INSET) / ATLAS_SIZE;
    const float_t v1 = ((tileY + 1) * TILE_SIZE - TEXEL_INSET) / ATLAS_SIZE;

    const int_t tint = block->getRenderColor(0);
    const float_t red = static_cast<float_t>((tint >> 16) & 255) / 255.0f;
    const float_t green = static_cast<float_t>((tint >> 8) & 255) / 255.0f;
    const float_t blue = static_cast<float_t>(tint & 255) / 255.0f;

    renderEnable(RenderCapability::Texture2D);
    renderBindTexture(mc->renderEngine->getTexture("/terrain.png"));
    renderColor4f(red, green, blue, 1.0f);

    Tessellator *tess = &Tessellator::instance;
    tess->startDrawingQuads();
    tess->addVertexWithUV(x, y + ICON_SIZE, zLevel, u0, v1);
    tess->addVertexWithUV(x + ICON_SIZE, y + ICON_SIZE, zLevel, u1, v1);
    tess->addVertexWithUV(x + ICON_SIZE, y, zLevel, u1, v0);
    tess->addVertexWithUV(x, y, zLevel, u0, v0);
    tess->draw();

    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void drawPanelTitle(FontRenderer *font, const std::string &text, int_t centerX, int_t y)
{
    if (font == nullptr)
        return;

    const int_t x = centerX - font->getStringWidth(text) / 2;
    font->drawString(text, x + 1, y + 1, 0xd0d0d0);
    font->drawString(text, x, y, 0x303030);
}

}

LegacyPlayGameScreen::LegacyPlayGameScreen(GuiScreen *parent)
    : GuiSelectWorld(parent), page(0), firstWorldIndex(0), visibleWorldCount(0), selectedControlIndex(0),
      hoveredControlIndex(-1), tutorialMessageTicks(0), lastMouseX(-1), lastMouseY(-1), panoramaAvailable(false)
{
}

int_t LegacyPlayGameScreen::maxVisibleWorlds() const
{
    const LegacyOptionsLayout base = buildPlayLayout(width, height, 1);
    const LegacyPlayPaneLayout panes = buildPlayPaneLayout(base, height);
    const int_t firstRowY = panes.rightY + playHeaderHeight(height) + PLAY_PANE_CONTENT_GAP;
    const int_t scrollMarkerSpace = PLAY_SCROLL_ARROW_BOTTOM_INSET + PLAY_SCROLL_ARROW_HEIGHT +
        PLAY_PANE_CONTENT_GAP;
    const int_t available = panes.rightY + panes.rightHeight - scrollMarkerSpace - firstRowY;
    return std::max<int_t>(1, std::min<int_t>(8, (available + base.rowSpacing) /
        (base.rowHeight + base.rowSpacing)));
}

int_t LegacyPlayGameScreen::maxPage() const
{
    const int_t pageSize = std::max<int_t>(1, maxVisibleWorlds());
    return saveList.empty() ? 0 : static_cast<int_t>((saveList.size() - 1) / pageSize);
}

void LegacyPlayGameScreen::initGui()
{
    loadSaves();
    panoramaAvailable = mc != nullptr && mc->renderEngine != nullptr &&
        mc->renderEngine->hasResource(legacyPanoramaResourcePath());
    rebuildButtons();
}

void LegacyPlayGameScreen::rebuildButtons()
{
    hoveredControlIndex = -1;
    for (GuiButton *button : controlList)
        delete button;
    controlList.clear();

    const int_t pageSize = std::max<int_t>(1, maxVisibleWorlds());
    const int_t lastPage = maxPage();
    page = std::max<int_t>(0, std::min<int_t>(page, lastPage));
    firstWorldIndex = page * pageSize;
    visibleWorldCount = std::min<int_t>(pageSize,
        std::max<int_t>(0, static_cast<int_t>(saveList.size()) - firstWorldIndex));

    layout = buildPlayLayout(width, height, visibleWorldCount);
    const LegacyPlayPaneLayout panes = buildPlayPaneLayout(layout, height);
    const int_t actionX = panes.leftX + PLAY_PANE_CONTENT_GAP;
    const int_t actionWidth = std::max<int_t>(1, panes.leftWidth - PLAY_PANE_CONTENT_GAP * 2);
    const int_t actionY = panes.leftY + playHeaderHeight(height) + PLAY_PANE_CONTENT_GAP;
    controlList.push_back(new LegacyGuiButton(BUTTON_CREATE_WORLD, actionX, actionY,
        actionWidth, layout.rowHeight, "Create New World"));
    controlList.push_back(new LegacyGuiButton(BUTTON_TUTORIAL, actionX,
        actionY + layout.rowHeight + layout.rowSpacing, actionWidth, layout.rowHeight,
        "Play Tutorial"));

    const int_t worldX = panes.rightX + PLAY_PANE_CONTENT_GAP;
    const int_t worldWidth = std::max<int_t>(1, panes.rightWidth - PLAY_PANE_CONTENT_GAP * 2);
    const int_t worldY = panes.rightY + playHeaderHeight(height) + PLAY_PANE_CONTENT_GAP;
    for (int_t i = 0; i < visibleWorldCount; ++i)
    {
        const int_t saveIndex = firstWorldIndex + i;
        controlList.push_back(new LegacyGuiButton(BUTTON_WORLD_BASE + saveIndex,
            worldX, worldY + i * (layout.rowHeight + layout.rowSpacing), worldWidth, layout.rowHeight,
            getSaveName(saveIndex)));
    }

    if (controlList.empty())
        selectedControlIndex = -1;
    else
        selectedControlIndex = std::max<int_t>(0,
            std::min<int_t>(selectedControlIndex, static_cast<int_t>(controlList.size()) - 1));
    syncSelectedButton();
}

void LegacyPlayGameScreen::syncSelectedButton()
{
    for (int_t i = 0; i < static_cast<int_t>(controlList.size()); ++i)
    {
        LegacyGuiButton *button = static_cast<LegacyGuiButton *>(controlList[i]);
        if (button != nullptr)
            button->setSelected(hoveredControlIndex < 0 && i == selectedControlIndex);
    }
}

void LegacyPlayGameScreen::activateSelection()
{
    const int_t targetIndex = hoveredControlIndex >= 0 ? hoveredControlIndex : selectedControlIndex;
    if (targetIndex < 0 || targetIndex >= static_cast<int_t>(controlList.size()))
        return;
    mc->sndManager->playSoundFX("random.action", 1.0f, 1.0f);
    actionPerformed(controlList[targetIndex]);
}

void LegacyPlayGameScreen::selectControl(int_t index)
{
    if (controlList.empty())
    {
        selectedControlIndex = -1;
        return;
    }
    selectedControlIndex = std::max<int_t>(0,
        std::min<int_t>(index, static_cast<int_t>(controlList.size()) - 1));
    syncSelectedButton();
}

void LegacyPlayGameScreen::moveSelection(int_t direction)
{
    if (hoveredControlIndex >= 0)
        return;
    if (controlList.empty() || direction == 0)
        return;

    if (direction < 0)
    {
        if (selectedControlIndex > 0)
        {
            selectControl(selectedControlIndex - 1);
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
        else if (page > 0)
        {
            --page;
            rebuildButtons();
            selectControl(static_cast<int_t>(controlList.size()) - 1);
            mc->sndManager->playSoundFX("random.scroll", 1.0f, 1.0f);
        }
        return;
    }

    if (selectedControlIndex + 1 < static_cast<int_t>(controlList.size()))
    {
        selectControl(selectedControlIndex + 1);
        mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (page < maxPage())
    {
        ++page;
        rebuildButtons();
        selectControl(std::min<int_t>(2, static_cast<int_t>(controlList.size()) - 1));
        mc->sndManager->playSoundFX("random.scroll", 1.0f, 1.0f);
    }
}

void LegacyPlayGameScreen::updateScreen()
{
    GuiSelectWorld::updateScreen();
    if (tutorialMessageTicks > 0)
        --tutorialMessageTicks;
#if PLATFORM_PS2 || PLATFORM_WII
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
#if PLATFORM_PS2
    if ((pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_SHIFT)) != 0)
    {
        if (mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        mc->displayGuiScreen(parentScreen);
        return;
    }
#endif
    if ((pad.pressed & PLATFORM_TEXT_UP) != 0)
        moveSelection(-1);
    else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0)
        moveSelection(1);
#if PLATFORM_PS2
    if ((pad.pressed & PLATFORM_TEXT_TYPE) != 0)
        activateSelection();
#elif PLATFORM_WII
    if (!platformMenuPointerActive() && (pad.pressed & PLATFORM_TEXT_TYPE) != 0)
        activateSelection();
#endif
#if PLATFORM_WII
    if ((pad.pressed & PLATFORM_TEXT_BACK) != 0)
    {
        mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        mc->displayGuiScreen(parentScreen);
    }
#endif
#endif
}

void LegacyPlayGameScreen::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == BUTTON_CREATE_WORLD)
    {
        mc->displayGuiScreen(new LegacyCreateWorldScreen(this));
        return;
    }
    if (button->id == BUTTON_TUTORIAL)
    {
        std::string errorMessage;
        if (!LegacyTutorialWorld::play(mc, errorMessage))
        {
            tutorialMessage = errorMessage.empty() ? "Could not load Tutorial World" : errorMessage;
            tutorialMessageTicks = 120;
        }
        return;
    }
    if (button->id >= BUTTON_WORLD_BASE)
    {
        const int_t index = button->id - BUTTON_WORLD_BASE;
        if (index >= 0 && index < static_cast<int_t>(saveList.size()))
            selectWorld(index);
    }
}

void LegacyPlayGameScreen::keyTyped(char_t c, int_t key)
{
    (void)c;
    if (key == 1)
    {
        mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        mc->displayGuiScreen(parentScreen);
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII
    if (key == 200)
    {
        moveSelection(-1);
        return;
    }
    if (key == 208)
    {
        moveSelection(1);
        return;
    }
    if (key == 28)
        activateSelection();
#endif
}

void LegacyPlayGameScreen::handleMouseInput()
{
    GuiSelectWorld::handleMouseInput();
    int_t wheel = lwjgl::Mouse::getEventDWheel();
    if (wheel == 0)
        return;

    const LegacyPlayPaneLayout panes = buildPlayPaneLayout(layout, height);
    if (lastMouseX < panes.rightX || lastMouseX >= panes.rightX + panes.rightWidth ||
        lastMouseY < panes.rightY || lastMouseY >= panes.rightY + panes.rightHeight)
        return;

    const int_t nextPage = wheel > 0 ? page - 1 : page + 1;
    if (nextPage < 0 || nextPage > maxPage())
        return;
    page = nextPage;
    rebuildButtons();
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.scroll", 1.0f, 1.0f);
}

void LegacyPlayGameScreen::drawLegacyScene(float_t partialTick)
{
    const bool drewPanorama = panoramaAvailable &&
        legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel);
    if (!drewPanorama)
        drawDefaultBackground();
    else
        drawGradientRect(0, 0, width, height, static_cast<int_t>(0x14000000u), static_cast<int_t>(0x3c000000u));

    LegacyMainMenuLayout titleLayout{};
    titleLayout.titleY = layout.titleY;
    titleLayout.titleMaxWidth = layout.titleMaxWidth;
    titleLayout.titleMaxHeight = layout.titleMaxHeight;
    legacyDrawTitleTexture(mc, titleLayout, width, zLevel, nullptr);
    panelRenderer.draw(layout);
}

void LegacyPlayGameScreen::drawEntryIcons()
{
    if (mc == nullptr || mc->renderEngine == nullptr)
        return;

    for (GuiButton *button : controlList)
    {
        Block *block = button != nullptr ? entryIconBlock(button->id) : nullptr;
        if (block == nullptr)
            continue;
        const int_t iconX = button->xPosition + 8;
        const int_t iconY = button->yPosition + (button->getButtonHeight() - 16) / 2;
        drawEntryIconTile(mc, block, entryIconTextureSide(button->id), iconX, iconY, zLevel + 2.0f);
    }
}

void LegacyPlayGameScreen::drawScrollIndicators()
{
    if (firstWorldIndex + visibleWorldCount >= static_cast<int_t>(saveList.size()))
        return;
    if (mc == nullptr || mc->renderEngine == nullptr)
        return;

    const int_t texture = g_scrollDown.resolve(mc->renderEngine);
    if (texture < 0)
        return;

    const LegacyPlayPaneLayout panes = buildPlayPaneLayout(layout, height);
    const int_t x = panes.rightX + (panes.rightWidth - PLAY_SCROLL_ARROW_WIDTH) / 2;
    const int_t y = panes.rightY + panes.rightHeight - PLAY_SCROLL_ARROW_BOTTOM_INSET - PLAY_SCROLL_ARROW_HEIGHT;
    legacyDrawUiTexture(texture, x, y, PLAY_SCROLL_ARROW_WIDTH, PLAY_SCROLL_ARROW_HEIGHT, zLevel + 2.0f);
}

void LegacyPlayGameScreen::drawMenuControlHints()
{
    drawLegacyMenuHints(fontRenderer, width, height, true);
}

void LegacyPlayGameScreen::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    const int_t previousHover = hoveredControlIndex;
    hoveredControlIndex = legacyHoveredSelectableButton(controlList, mouseX, mouseY);
    if (hoveredControlIndex >= 0)
        selectedControlIndex = hoveredControlIndex;
    if (hoveredControlIndex != previousHover)
    {
        syncSelectedButton();
        if (hoveredControlIndex >= 0 && mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }

    drawLegacyScene(partialTick);
    const LegacyPlayPaneLayout panes = buildPlayPaneLayout(layout, height);
    drawPlayPaneFrames(panelRenderer, panes);
    drawPanelTitle(fontRenderer, "Start Game", panes.leftX + panes.leftWidth / 2, panes.leftY + 8);
    drawPanelTitle(fontRenderer, "Join Game", panes.rightX + panes.rightWidth / 2, panes.rightY + 8);
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
    drawEntryIcons();
    drawScrollIndicators();
    drawMenuControlHints();

    if (saveList.empty())
        drawPanelTitle(fontRenderer, "No Games Found", panes.rightX + panes.rightWidth / 2,
            panes.rightY + panes.rightHeight / 2 - 4);
    if (tutorialMessageTicks > 0 && !tutorialMessage.empty())
        drawCenteredString(fontRenderer, tutorialMessage, width / 2,
            layout.panelY + layout.panelHeight - 15, 0xffff00);
}
