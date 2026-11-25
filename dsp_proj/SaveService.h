#pragma once
#include <string>
#include "Map.h"
#include "ResourceStore.h"
#include "WaveManager.h"

class SaveService
{
public:
    static bool saveGame(const std::string& aFilename,
        const Map& aMap,
        const ResourceStore& aResources,
        const WaveManager& aWaves);

    static bool loadGame(const std::string& aFilename,
        Map& aMap,
        ResourceStore& aResources,
        WaveManager& aWaves);
};