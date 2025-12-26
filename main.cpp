#include "include/npc_system.hpp"
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <memory>
#include <atomic>

std::vector<std::shared_ptr<NPC>> npcs;
std::shared_mutex npcsMutex;        
std::mutex coutMutex;               

struct BattleTask {
    std::shared_ptr<NPC> attacker;
    std::shared_ptr<NPC> target;
};

std::queue<BattleTask> battleQueue;
std::mutex battleQueueMutex;
std::condition_variable battleQueueCV;
std::atomic<bool> gameRunning{true};
const int MAP_SIZE_X = 100;
const int MAP_SIZE_Y = 100;
const int GAME_DURATION_SECONDS = 30;

void movementThread() {
    while (gameRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        {
            std::lock_guard<std::shared_mutex> lock(npcsMutex);
            for (auto& npc : npcs) {
                if (npc->isAlive()) {
                    npc->moveRandomly(MAP_SIZE_X, MAP_SIZE_Y);
                }
            }
        }

        std::vector<std::shared_ptr<NPC>> localNpcs;
        {
            std::shared_lock<std::shared_mutex> lock(npcsMutex);
            localNpcs = npcs;
            for (size_t i = 0; i < localNpcs.size(); ++i) {
                if (!localNpcs[i]->isAlive()) continue;
                for (size_t j = i + 1; j < localNpcs.size(); ++j) {
                    if (!localNpcs[j]->isAlive()) continue;
                    if (localNpcs[i]->isInRangeForKill(*localNpcs[j])) {
                        BattleTask task{localNpcs[i], localNpcs[j]};
                        {
                            std::lock_guard<std::mutex> qLock(battleQueueMutex);
                            battleQueue.push(task);
                        }
                        battleQueueCV.notify_one();
                    }
                }
            }
        }
    }
}

void battleThread() {
    while (gameRunning.load()) {
        std::unique_lock<std::mutex> lock(battleQueueMutex);
        battleQueueCV.wait(lock, [] { return !battleQueue.empty() || !gameRunning.load(); });

        if (!gameRunning.load() && battleQueue.empty()) break;

        if (!battleQueue.empty()) {
            BattleTask task = battleQueue.front();
            battleQueue.pop();
            lock.unlock();

            if (!task.attacker || !task.target || !task.attacker->isAlive() || !task.target->isAlive()) {
                continue;
            }

            BattleVisitor visitor(task.attacker);
            task.target->accept(visitor);

            if (visitor.wasKill()) {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[BATTLE] " << visitor.getKiller() << " killed " << visitor.getVictim() << std::endl;
            }
        }
    }
}

void printMap() {
    std::vector<std::shared_ptr<NPC>> localNpcs;
    {
        std::shared_lock<std::shared_mutex> lock(npcsMutex);
        localNpcs = npcs;
    }

    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "\n=== MAP (" << MAP_SIZE_X << "x" << MAP_SIZE_Y << ") ===" << std::endl;
    for (int y = 0; y < MAP_SIZE_Y; y += 10) { 
        for (int x = 0; x < MAP_SIZE_X; x += 10) {
            char c = '.';
            for (const auto& npc : localNpcs) {
                if (npc->isAlive() && npc->getX() / 10 == x / 10 && npc->getY() / 10 == y / 10) {
                    if (npc->getType() == "Bear") c = 'B';
                    else if (npc->getType() == "Duck") c = 'D';
                    else if (npc->getType() == "Desman") c = 'S';
                    break;
                }
            }
            std::cout << c;
        }
        std::cout << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

int main() {
    auto textObserver = TextObserver::get();
    auto fileObserver = FileObserver::get();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> coordX(0, MAP_SIZE_X - 1);
    std::uniform_int_distribution<> coordY(0, MAP_SIZE_Y - 1);
    std::uniform_int_distribution<> npcType(0, 2);

    for (int i = 0; i < 50; ++i) {
        int x = coordX(gen);
        int y = coordY(gen);
        std::shared_ptr<NPC> npc;
        int type = npcType(gen);
        switch (type) {
            case 0: npc = std::make_shared<Bear>("Bear" + std::to_string(i), x, y); break;
            case 1: npc = std::make_shared<Duck>("Duck" + std::to_string(i), x, y); break;
            case 2: npc = std::make_shared<Desman>("Desman" + std::to_string(i), x, y); break;
        }
        npc->subscribe(textObserver);
        npc->subscribe(fileObserver);
        {
            std::lock_guard<std::shared_mutex> lock(npcsMutex);
            npcs.push_back(npc);
        }
    }

    std::cout << "Starting game with 50 NPCs" << std::endl;

    std::thread moveThread(movementThread);
    std::thread battleTh(battleThread);

    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

        if (elapsed >= GAME_DURATION_SECONDS) {
            gameRunning.store(false);
            break;
        }

        printMap();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    battleQueueCV.notify_all();
    if (moveThread.joinable()) moveThread.join();
    if (battleTh.joinable()) battleTh.join();

    std::vector<std::shared_ptr<NPC>> survivors;
    {
        std::shared_lock<std::shared_mutex> lock(npcsMutex);
        for (const auto& npc : npcs) {
            if (npc->isAlive()) {
                survivors.push_back(npc);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\n=== GAME OVER ===" << std::endl;
        std::cout << "Survivors: " << survivors.size() << std::endl;
        for (const auto& npc : survivors) {
            std::cout << "[" << npc->getType() << "] " << npc->getName() << " @ (" << npc->getX() << ", " << npc->getY() << ")\n";
        }
    }

    return 0;
}