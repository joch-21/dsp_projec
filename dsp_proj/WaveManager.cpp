#include "WaveManager.h"

WaveManager::WaveManager() : fWaveQueue(20), fCurrentWave(0)
{
    loadDefaultWaves();
}

void WaveManager::loadDefaultWaves()
{
    while (!fWaveQueue.isEmpty())
    {
        fWaveQueue.dequeue();
    }
    fCurrentWave = 0;

    // Wave entity counts, health, and attack damage are configured in WaveManager.h.
    for (const WaveConfig::EnemyWave& wave : WaveConfig::Enemies)
    {
        fWaveQueue.enqueue(SpawnEvent(wave.number, wave.name, wave.count, wave.hitPoints, wave.damage));
    }
}

bool WaveManager::hasNextWave() const
{
    return !fWaveQueue.isEmpty();
}

SpawnEvent WaveManager::spawnNextWave()
{
    SpawnEvent next = fWaveQueue.dequeue();
    fCurrentWave = next.waveNumber;
    return next;
}

const SpawnEvent& WaveManager::peekNextWave() const
{
    return fWaveQueue.front();
}

int WaveManager::getCurrentWave() const
{
    return fCurrentWave;
}

int WaveManager::getRemainingWaveCount() const
{
    return fWaveQueue.size();
}

void WaveManager::syncToWave(int aSavedWave)
{
    loadDefaultWaves();
    while (hasNextWave() && peekNextWave().waveNumber <= aSavedWave)
    {
        spawnNextWave();
    }
}
