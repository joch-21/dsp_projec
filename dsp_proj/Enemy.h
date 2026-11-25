#pragma once
#include <string>

struct Enemy
{
    std::string name;
    int hp;
    int maxHP;
    int attackDmg;

    Enemy() : name("Crawler"), hp(20), maxHP(20), attackDmg(10) {}
    Enemy(const std::string& aName, int aMaxHP, int aAttackDmg)
        : name(aName), hp(aMaxHP), maxHP(aMaxHP), attackDmg(aAttackDmg)
    {
    }

    bool isAlive() const
    {
        return hp > 0;
    }

    void takeDamage(int aAmount)
    {
        if (aAmount > 0)
        {
            hp -= aAmount;
            if (hp < 0)
            {
                hp = 0;
            }
        }
    }
};