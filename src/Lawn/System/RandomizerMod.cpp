#include "RandomizerMod.h"

#include "PlayerInfo.h"
#include "../LawnCommon.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <random>
#include <vector>

namespace {

constexpr int kPlantCount = 48;          // SEED_PEASHOOTER .. SEED_COBCANNON
constexpr int kAdventureSlots = 40;      // plants actually handed out during adventure
constexpr int kZombieMapSize = 64;
constexpr uint32_t kMagic = 0x31444E52;  // 'RND1' little-endian

struct SaveData {
	uint32_t magic = kMagic;
	uint32_t finishedAdventure = 0;
	uint8_t plants[kPlantCount]{};
	uint8_t zombieMap[kZombieMapSize]{};
};

SaveData gData;
uint32_t gLoadedId = 0xFFFFFFFFu;
bool gLoaded = false;

bool IsShuffleZombie(ZombieType theType)
{
	switch (theType)
	{
	case ZombieType::ZOMBIE_NORMAL:
	case ZombieType::ZOMBIE_TRAFFIC_CONE:
	case ZombieType::ZOMBIE_POLEVAULTER:
	case ZombieType::ZOMBIE_PAIL:
	case ZombieType::ZOMBIE_NEWSPAPER:
	case ZombieType::ZOMBIE_DOOR:
	case ZombieType::ZOMBIE_FOOTBALL:
	case ZombieType::ZOMBIE_DANCER:
	case ZombieType::ZOMBIE_DUCKY_TUBE:
	case ZombieType::ZOMBIE_SNORKEL:
	case ZombieType::ZOMBIE_ZAMBONI:
	case ZombieType::ZOMBIE_BOBSLED:
	case ZombieType::ZOMBIE_DOLPHIN_RIDER:
	case ZombieType::ZOMBIE_JACK_IN_THE_BOX:
	case ZombieType::ZOMBIE_BALLOON:
	case ZombieType::ZOMBIE_DIGGER:
	case ZombieType::ZOMBIE_POGO:
	case ZombieType::ZOMBIE_YETI:
	case ZombieType::ZOMBIE_BUNGEE:
	case ZombieType::ZOMBIE_LADDER:
	case ZombieType::ZOMBIE_CATAPULT:
	case ZombieType::ZOMBIE_GARGANTUAR:
		return true;
	default:
		// boss, flag, imp, backup dancer, zombotany heads stay themselves
		return false;
	}
}

std::string SavePath(uint32_t theId)
{
	return Sexy::GetAppDataPath(std::format("userdata/randomizer_{}.dat", theId));
}

void ShuffleZombies(std::mt19937& theRng)
{
	for (int i = 0; i < kZombieMapSize; i++)
		gData.zombieMap[i] = static_cast<uint8_t>(i);

	std::vector<int> aPool;
	for (int i = 0; i < static_cast<int>(ZombieType::NUM_ZOMBIE_TYPES) && i < kZombieMapSize; i++)
	{
		if (IsShuffleZombie(static_cast<ZombieType>(i)))
			aPool.push_back(i);
	}
	for (int i = static_cast<int>(aPool.size()) - 1; i > 0; i--)
	{
		std::uniform_int_distribution<int> aDist(0, i);
		int j = aDist(theRng);
		std::swap(aPool[i], aPool[j]);
	}
	std::vector<int> aOriginal;
	for (int i = 0; i < static_cast<int>(ZombieType::NUM_ZOMBIE_TYPES) && i < kZombieMapSize; i++)
	{
		if (IsShuffleZombie(static_cast<ZombieType>(i)))
			aOriginal.push_back(i);
	}
	for (size_t i = 0; i < aOriginal.size(); i++)
		gData.zombieMap[aOriginal[i]] = static_cast<uint8_t>(aPool[i]);
}

void ShufflePlants(std::mt19937& theRng)
{
	for (int i = 0; i < kPlantCount; i++)
		gData.plants[i] = static_cast<uint8_t>(i);
	for (int i = kPlantCount - 1; i > 0; i--)
	{
		std::uniform_int_distribution<int> aDist(0, i);
		int j = aDist(theRng);
		std::swap(gData.plants[i], gData.plants[j]);
	}
}

bool ReadFile(uint32_t theId)
{
	FILE* aFile = fopen(SavePath(theId).c_str(), "rb");
	if (!aFile)
		return false;
	SaveData aData;
	const size_t aRead = fread(&aData, 1, sizeof(aData), aFile);
	fclose(aFile);
	if (aRead != sizeof(aData) || aData.magic != kMagic)
		return false;
	gData = aData;
	return true;
}

void WriteFile(uint32_t theId)
{
	Sexy::MkDir(Sexy::GetAppDataPath("userdata"));
	FILE* aFile = fopen(SavePath(theId).c_str(), "wb");
	if (!aFile)
		return;
	fwrite(&gData, 1, sizeof(gData), aFile);
	fclose(aFile);
}

} // namespace

void Randomizer_Ensure(PlayerInfo* thePlayer)
{
	if (!thePlayer)
		return;

	if (!gLoaded || gLoadedId != thePlayer->mId)
	{
		gLoaded = true;
		gLoadedId = thePlayer->mId;
		if (!ReadFile(thePlayer->mId))
		{
			std::mt19937 aRng(static_cast<uint32_t>(thePlayer->mId) ^ static_cast<uint32_t>(std::random_device{}()));
			std::memset(&gData, 0, sizeof(gData));
			gData.magic = kMagic;
			gData.finishedAdventure = static_cast<uint32_t>(thePlayer->mFinishedAdventure);
			ShufflePlants(aRng);
			ShuffleZombies(aRng);
			WriteFile(thePlayer->mId);
			return;
		}
	}

	if (gData.finishedAdventure != static_cast<uint32_t>(thePlayer->mFinishedAdventure))
	{
		std::mt19937 aRng(static_cast<uint32_t>(thePlayer->mId)
			^ (static_cast<uint32_t>(thePlayer->mFinishedAdventure) * 0x9E3779B9u)
			^ static_cast<uint32_t>(std::random_device{}()));
		ShuffleZombies(aRng);
		gData.finishedAdventure = static_cast<uint32_t>(thePlayer->mFinishedAdventure);
		WriteFile(thePlayer->mId);
	}
}

bool Randomizer_PlantUnlocked(PlayerInfo* thePlayer, SeedType theSeed, int theUnlockedCount, bool theAdventureFinished)
{
	if (!thePlayer)
		return false;
	Randomizer_Ensure(thePlayer);
	const int aSeed = static_cast<int>(theSeed);
	if (aSeed < 0 || aSeed >= kPlantCount)
		return false;
	if (theAdventureFinished)
		return true;
	if (theUnlockedCount < 0)
		theUnlockedCount = 0;
	if (theUnlockedCount > kAdventureSlots)
		theUnlockedCount = kAdventureSlots;
	for (int i = 0; i < theUnlockedCount; i++)
	{
		if (gData.plants[i] == static_cast<uint8_t>(aSeed))
			return true;
	}
	return false;
}

SeedType Randomizer_AwardSeed(PlayerInfo* thePlayer, int theVanillaAwardIndex)
{
	if (!thePlayer)
		return static_cast<SeedType>(theVanillaAwardIndex);
	Randomizer_Ensure(thePlayer);
	if (theVanillaAwardIndex < 0 || theVanillaAwardIndex >= kPlantCount)
		return static_cast<SeedType>(theVanillaAwardIndex);
	return static_cast<SeedType>(gData.plants[theVanillaAwardIndex]);
}

ZombieType Randomizer_MapZombie(PlayerInfo* thePlayer, ZombieType theOriginal)
{
	if (!thePlayer)
		return theOriginal;
	Randomizer_Ensure(thePlayer);
	const int aIndex = static_cast<int>(theOriginal);
	if (aIndex < 0 || aIndex >= kZombieMapSize)
		return theOriginal;
	if (!IsShuffleZombie(theOriginal))
		return theOriginal;
	return static_cast<ZombieType>(gData.zombieMap[aIndex]);
}

int Randomizer_AreaSeeds(PlayerInfo* thePlayer, int theLevel, int* theOutSeeds, int theCap)
{
	if (!thePlayer || !theOutSeeds || theCap <= 0)
		return 0;
	int aBegin = 0;
	int aEnd = 0;
	switch (theLevel)
	{
	case 10: aBegin = 0;  aEnd = 8;  break; // day
	case 20: aBegin = 8;  aEnd = 16; break; // night
	case 30: aBegin = 16; aEnd = 24; break; // pool
	case 40: aBegin = 24; aEnd = 32; break; // fog
	case 50: aBegin = 32; aEnd = 40; break; // roof
	default: return 0;
	}
	Randomizer_Ensure(thePlayer);
	int aCount = 0;
	for (int i = aBegin; i < aEnd && aCount < theCap; i++)
		theOutSeeds[aCount++] = gData.plants[i];
	return aCount;
}
