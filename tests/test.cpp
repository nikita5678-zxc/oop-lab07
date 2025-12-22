#include "../include/npc_system.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <string>



TEST(NPCTest, MoveRandomlyStaysInsideMapAndWithinStepForBear) {
    const int mapX = 100;
    const int mapY = 100;

    auto bear = std::make_shared<Bear>("Bear", 50, 50);
    int oldX = bear->getX();
    int oldY = bear->getY();

    bear->moveRandomly(mapX, mapY);

    int newX = bear->getX();
    int newY = bear->getY();

    EXPECT_GE(newX, 0);
    EXPECT_LT(newX, mapX);
    EXPECT_GE(newY, 0);
    EXPECT_LT(newY, mapY);

    EXPECT_LE(std::abs(newX - oldX), 5);
    EXPECT_LE(std::abs(newY - oldY), 5);
}

TEST(NPCTest, IsInRangeForKillUsesEuclideanDistance) {
    auto a = std::make_shared<Bear>("A", 0, 0);
    auto b = std::make_shared<Bear>("B", 3, 4); 
    auto c = std::make_shared<Bear>("C", 20, 0); 

    EXPECT_TRUE(a->isInRangeForKill(*b));
    EXPECT_FALSE(a->isInRangeForKill(*c));
}

TEST(NPCTest, RollDiceIsBetween1And6) {
    auto duck = std::make_shared<Duck>("D", 10, 10);
    for (int i = 0; i < 100; ++i) {
        int v = duck->rollDice();
        EXPECT_GE(v, 1);
        EXPECT_LE(v, 6);
    }
}


TEST(FightTest, BearKillsDuckAndDesman) {
    auto bear = std::make_shared<Bear>("Bear", 0, 0);
    auto duck = std::make_shared<Duck>("Duck", 0, 0);
    auto desman = std::make_shared<Desman>("Desman", 0, 0);

    EXPECT_TRUE(bear->fight(duck));
    EXPECT_FALSE(bear->isAlive() == false); 
    EXPECT_FALSE(duck->isAlive());          

    EXPECT_TRUE(bear->fight(desman));
    EXPECT_FALSE(bear->isAlive() == false);
    EXPECT_FALSE(desman->isAlive());
}

TEST(FightTest, BearVsBearBothDie) {
    auto b1 = std::make_shared<Bear>("B1", 0, 0);
    auto b2 = std::make_shared<Bear>("B2", 0, 0);

    EXPECT_TRUE(b1->fight(b2));
    EXPECT_FALSE(b1->isAlive());
    EXPECT_FALSE(b2->isAlive());
}

TEST(FightTest, DesmanKillsBear) {
    auto desman = std::make_shared<Desman>("Desman", 0, 0);
    auto bear = std::make_shared<Bear>("Bear", 0, 0);

    EXPECT_TRUE(desman->fight(bear));
    EXPECT_FALSE(bear->isAlive());
    EXPECT_TRUE(desman->isAlive());
}

TEST(FightTest, DuckDoesNotKillAnyone) {
    auto duck = std::make_shared<Duck>("Duck", 0, 0);
    auto bear = std::make_shared<Bear>("Bear", 0, 0);

    EXPECT_FALSE(duck->fight(bear));
    EXPECT_TRUE(bear->isAlive());
    EXPECT_TRUE(duck->isAlive());
}


class TestObserver : public Observer {
public:
    int calls = 0;
    std::string lastKiller;
    std::string lastVictim;

    void onKill(const std::string& killer, const std::string& victim) override {
        ++calls;
        lastKiller = killer;
        lastVictim = victim;
    }
};

TEST(DungeonTest, AddNPCReturnsTrueAndStoresNPC) {
    Dungeon d;
    auto bear = std::make_shared<Bear>("Bear", 0, 0);
    EXPECT_TRUE(d.addNPC(bear));
}

TEST(DungeonTest, BattleNotifiesObserverOnKill) {
    Dungeon d;
    auto obs = std::make_shared<TestObserver>();
    d.addObserver(obs);

    auto bear = std::make_shared<Bear>("Bear", 0, 0);
    auto duck = std::make_shared<Duck>("Duck", 1, 1); 

    d.addNPC(bear);
    d.addNPC(duck);

    d.battle(10.0); 

    EXPECT_GE(obs->calls, 1);
    EXPECT_EQ(obs->lastKiller, bear->getName());
    EXPECT_EQ(obs->lastVictim, duck->getName());
}


