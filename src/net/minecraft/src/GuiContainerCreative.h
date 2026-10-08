#pragma once

#include "GuiContainer.h"

class EntityPlayer;
class InventoryBasic;
class RenderItem;
class ItemStack;
class Slot;
class GuiButton;

// net.minecraft.src.GuiContainerCreative
class GuiContainerCreative : public GuiContainer
{
public:
    explicit GuiContainerCreative(EntityPlayer *player);

    void updateScreen() override;
    void initGui() override;
    void handleMouseInput() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    Slot *getControllerNavigationTarget(Slot *selected, int_t dirX, int_t dirY) override;

    void setCategory(int_t tabIndex);
    void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;
    void keyTyped(char_t c, int_t key) override;

    static InventoryBasic *getInventory();

    static ItemStack *s_tabIcons[6];
    static RenderItem *creativeItemRenderer;

protected:
    void handleMouseClick(Slot *slot, int_t slotId, int_t button, bool shift) override;
    void drawGuiContainerForegroundLayer() override;
    void drawGuiContainerBackgroundLayer(float_t partialTick) override;
    void actionPerformed(GuiButton *button) override;

    void drawCategoryTabs(int_t guiLeft, int_t guiTop);
    void drawCreativeTabTooltip(const char *text, int_t x, int_t y);

private:
    bool scrollRows(int_t direction);

    static InventoryBasic inventory;
    float_t currentScroll;
    bool isScrolling;
    bool wasClicking;
};
