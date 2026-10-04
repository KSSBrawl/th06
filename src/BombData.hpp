#pragma once
#include "Player.hpp"

namespace th06
{
struct BombData
{
    void (*calc)(Player *p);
    void (*draw)(Player *p);
};

void BombData_BombReimuACalc(Player *);
void BombData_BombReimuBCalc(Player *);
void BombData_BombMarisaACalc(Player *);
void BombData_BombMarisaBCalc(Player *);
void BombData_BombReimuADraw(Player *);
void BombData_BombReimuBDraw(Player *);
void BombData_BombMarisaADraw(Player *);
void BombData_BombMarisaBDraw(Player *);

} // namespace th06
