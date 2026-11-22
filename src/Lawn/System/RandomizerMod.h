#pragma once

#include "../../ConstEnums.h"

class PlayerInfo;

// Adventure randomizer:
// - plant unlock order (seeds 0..47, imitater excluded), rolled once per profile
// - zombie type swap (boss/flag/imp/backup dancer stay), re-rolled when adventure clears
// - conveyor levels 10/20/30/40/50 use plants unlocked on that area
// - seed chooser layout follows obtain order
// - early levels without chooser fill bank from obtain order

void Randomizer_Ensure(PlayerInfo* thePlayer);

bool Randomizer_PlantUnlocked(PlayerInfo* thePlayer, SeedType theSeed, int theUnlockedCount, bool theAdventureFinished);

// vanillaAwardIndex is the value GetAwardSeedForLevel() already returns (0..39 for a real new plant)
SeedType Randomizer_AwardSeed(PlayerInfo* thePlayer, int theVanillaAwardIndex);

// Plant at obtain-order index (0 = first unlocked). Falls back to (SeedType)index if out of range.
SeedType Randomizer_PlantAtOrder(PlayerInfo* thePlayer, int theOrderIndex);

// Grid slot for this plant in the chooser (0..47). Returns theOrderIndex-style slot; -1 to use vanilla index.
int Randomizer_DisplaySlotForPlant(PlayerInfo* thePlayer, SeedType theSeed);

ZombieType Randomizer_MapZombie(PlayerInfo* thePlayer, ZombieType theOriginal);

// Writes seed ids for the conveyor of this adventure level. Returns 0 if this level is not one of 10/20/30/40/50.
int Randomizer_AreaSeeds(PlayerInfo* thePlayer, int theLevel, int* theOutSeeds, int theCap);
