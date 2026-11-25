#pragma once
#include "Queue.h"
#include "SpawnEvent.h"

// Wave roster is configured beside the queue that owns it.
namespace WaveConfig
{
    struct EnemyWave
    {
        int number;
        const char* name;
        int count;
        int hitPoints;
        int damage;
    };

    inline constexpr EnemyWave Enemies[] = {
        { 1, "Crawler", 2, 15, 10 },
        { 2, "Dagger", 3, 25, 15 },
        { 3, "Mace", 3, 140, 20 },
        { 4, "Guardian", 2, 180, 35 },
        { 5, "Crawler", 5, 235, 50 },
        { 6, "Dagger", 5, 250, 75 },
        { 7, "Mace", 7, 180, 100 },
        { 8, "Guardian", 5, 400, 125 }
    };
}

class WaveManager
{
private:
    Queue<SpawnEvent> fWaveQueue;
    int fCurrentWave;

public:
    WaveManager();

    // Populate the FIFO queue with the standard escalating enemy waves
    void loadDefaultWaves();

    // Check if there are still waves waiting in the queue
    bool hasNextWave() const;

    // Dequeue and return the next wave in FIFO order
    SpawnEvent spawnNextWave();

    // Peek at the upcoming wave without removing it
    const SpawnEvent& peekNextWave() const;

    int getCurrentWave() const;
    int getRemainingWaveCount() const;

    // Used by SaveService when loading a saved game at a later wave
    void syncToWave(int aSavedWave);
};
