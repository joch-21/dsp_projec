#pragma once
#include <string>

struct SpawnEvent
{
    int waveNumber;
    std::string enemyName;
    int enemyCount;
    int enemyHP;
    int enemyDamage;

    SpawnEvent()
        : waveNumber(1), enemyName("Crawler"), enemyCount(1), enemyHP(20), enemyDamage(10)
    {
    }

    SpawnEvent(int aWaveNum, const std::string& aEnemyName, int aCount, int aHP, int aDamage)
        : waveNumber(aWaveNum),
        enemyName(aEnemyName),
        enemyCount(aCount),
        enemyHP(aHP),
        enemyDamage(aDamage)
    {
    }

    int getTotalWaveHP() const
    {
        return enemyCount * enemyHP;
    }

    int getTotalWaveDamage() const
    {
        return enemyCount * enemyDamage;
    }
};