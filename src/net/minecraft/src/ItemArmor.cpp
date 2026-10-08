#include "ItemArmor.h"

#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"

const int ItemArmor::maxDamageArray[4] = { 11, 16, 15, 13 };

ItemArmor::ItemArmor(int id, EnumArmorMaterial armorMaterial, int render, int type)
    : Item(id), material(armorMaterial), armorType(type),
      damageReduceAmount(EnumArmorMaterialHelper::getDamageReductionAmount(armorMaterial, type)),
      renderIndex(render)
{
    setMaxDamage(EnumArmorMaterialHelper::getDurability(armorMaterial, type));
    maxStackSize = 1;
}

ItemStack* ItemArmor::onItemRightClick(ItemStack* itemstack, World*, EntityPlayer* entityplayer)
{
    if (itemstack == nullptr || entityplayer == nullptr || armorType < 0 || armorType >= 4)
        return itemstack;

    InventoryPlayer* inventory = entityplayer->inventory;
    const int armorSlot = 3 - armorType;
    if (inventory == nullptr || inventory->armorItemInSlot(armorSlot) != nullptr)
        return itemstack;

    ItemStack* equippedStack = itemstack->splitStack(1);
    inventory->setInventorySlotContents(36 + armorSlot, equippedStack);
    inventory->onInventoryChanged();
    return itemstack;
}

int ItemArmor::getItemEnchantability()
{
    return EnumArmorMaterialHelper::getEnchantability(material);
}
