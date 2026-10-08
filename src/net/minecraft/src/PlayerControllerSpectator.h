#pragma once

#include "PlayerController.h"

// Single-player spectator controller. Legacy multiplayer servers cannot
// authoritatively represent this game type.
class PlayerControllerSpectator : public PlayerController
{
public:
	explicit PlayerControllerSpectator(Minecraft *minecraft);

	void initializePlayer(EntityPlayer *player) override;
	void clickBlock(int_t x, int_t y, int_t z, int_t side) override;
	bool sendBlockRemoved(int_t x, int_t y, int_t z, int_t side) override;
	bool sendUseItem(EntityPlayer *player, World *world, ItemStack *stack) override;
	bool sendPlaceBlock(EntityPlayer *player, World *world, ItemStack *stack,
		int_t x, int_t y, int_t z, int_t side) override;
	void interactWithEntity(EntityPlayer *player, Entity *entity) override;
	void attackEntity(EntityPlayer *player, Entity *entity) override;
	bool isNotCreative() override;
	bool isInCreativeMode() override;
	bool isSpectatorMode() const override;
};