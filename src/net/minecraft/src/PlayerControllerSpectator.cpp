#include "PlayerControllerSpectator.h"

#include "EntityPlayer.h"
#include "PlayerCapabilities.h"

PlayerControllerSpectator::PlayerControllerSpectator(Minecraft *minecraft)
	: PlayerController(minecraft)
{
}

void PlayerControllerSpectator::initializePlayer(EntityPlayer *player)
{
	PlayerController::initializePlayer(player);
	if (player == nullptr)
		return;
	player->capabilities.allowFlying = true;
	player->capabilities.isFlying = true;
	player->capabilities.disableDamage = true;
	player->capabilities.isSpectator = true;
	player->noClip = true;
}

void PlayerControllerSpectator::clickBlock(int_t, int_t, int_t, int_t)
{
}

bool PlayerControllerSpectator::sendBlockRemoved(int_t, int_t, int_t, int_t)
{
	return false;
}

bool PlayerControllerSpectator::sendUseItem(EntityPlayer *, World *, ItemStack *)
{
	return false;
}

bool PlayerControllerSpectator::sendPlaceBlock(EntityPlayer *, World *, ItemStack *, int_t, int_t, int_t, int_t)
{
	return false;
}

void PlayerControllerSpectator::interactWithEntity(EntityPlayer *, Entity *)
{
}

void PlayerControllerSpectator::attackEntity(EntityPlayer *, Entity *)
{
}

bool PlayerControllerSpectator::isNotCreative()
{
	return true;
}

bool PlayerControllerSpectator::isInCreativeMode()
{
	return false;
}

bool PlayerControllerSpectator::isSpectatorMode() const
{
	return true;
}