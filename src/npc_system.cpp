#include "../include/npc_system.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>

std::shared_ptr<NPC> NPC::load(std::ifstream& is) {
    std::string type, name;
    int x, y;
    is >> type >> name >> x >> y;
    if (!is) return nullptr;

    if (type == "Bear") return std::make_shared<Bear>(name, x, y);
    if (type == "Duck") return std::make_shared<Duck>(name, x, y);
    if (type == "Desman") return std::make_shared<Desman>(name, x, y);
    throw std::runtime_error("Unknown NPC type: " + type);
}

void NPC::moveRandomly(int mapSizeX, int mapSizeY) {
    if (!isAlive()) return;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dx(-moveDistance, moveDistance);
    std::uniform_int_distribution<> dy(-moveDistance, moveDistance);

    int newX = x + dx(gen);
    int newY = y + dy(gen);

    x = std::max(0, std::min(newX, mapSizeX - 1));
    y = std::max(0, std::min(newY, mapSizeY - 1));
}

bool NPC::isInRangeForKill(const NPC& other) const {
    if (!isAlive() || !other.isAlive()) return false;
    double dist = std::hypot(x - other.x, y - other.y);
    return dist <= killDistance;
}

int NPC::rollDice() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dice(1, 6);
    return dice(gen);
}

void Bear::accept(Visitor& visitor) {
    visitor.visit(std::static_pointer_cast<Bear>(shared_from_this()));
}

bool Bear::fight(std::shared_ptr<NPC> other) {
    if (!other || !other->isAlive()) return false;
    if (dynamic_cast<Duck*>(other.get())) {
        other->kill();
        return true;
    }
    if (dynamic_cast<Desman*>(other.get())) {
        other->kill();
        return true;
    }
    if (dynamic_cast<Bear*>(other.get())) {
        kill();
        other->kill();
        return true;
    }
    return false;
}

void Bear::save(std::ofstream& os) const {
    os << "Bear " << name << " " << x << " " << y << "\n";
}

void Duck::accept(Visitor& visitor) {
    visitor.visit(std::static_pointer_cast<Duck>(shared_from_this()));
}

bool Duck::fight(std::shared_ptr<NPC> other) {
    if (!other || !other->isAlive()) return false;
    return false;
}

void Duck::save(std::ofstream& os) const {
    os << "Duck " << name << " " << x << " " << y << "\n";
}

void Desman::accept(Visitor& visitor) {
    visitor.visit(std::static_pointer_cast<Desman>(shared_from_this()));
}

bool Desman::fight(std::shared_ptr<NPC> other) {
    if (!other || !other->isAlive()) return false;
    if (dynamic_cast<Bear*>(other.get())) {
        other->kill();
        return true;
    }
    return false;
}

void Desman::save(std::ofstream& os) const {
    os << "Desman " << name << " " << x << " " << y << "\n";
}

void BattleVisitor::visit(std::shared_ptr<Bear> target) {
    if (!attacker || !attacker->isAlive() || !target || !target->isAlive()) return;
    if (distance(*attacker, *target) <= range) {
        if (attacker->fight(target)) {
            _killer = attacker->getName();
            _victim = target->getName();
            _killOccurred = true;
        }
    }
}

void BattleVisitor::visit(std::shared_ptr<Duck> target) {
    if (!attacker || !attacker->isAlive() || !target || !target->isAlive()) return;
    if (distance(*attacker, *target) <= range) {
        if (attacker->fight(target)) {
            _killer = attacker->getName();
            _victim = target->getName();
            _killOccurred = true;
        }
    }
}

void BattleVisitor::visit(std::shared_ptr<Desman> target) {
    if (!attacker || !attacker->isAlive() || !target || !target->isAlive()) return;
    if (distance(*attacker, *target) <= range) {
        if (attacker->fight(target)) {
            _killer = attacker->getName();
            _victim = target->getName();
            _killOccurred = true;
        }
    }
}

void ConsoleObserver::onKill(const std::string& killer, const std::string& victim) {
    std::cout << "[KILL] " << killer << " killed " << victim << std::endl;
}

FileObserver::FileObserver(const std::string& filename)
    : logFile(filename, std::ios::app) {
    if (!logFile.is_open()) {
        throw std::runtime_error("Cannot open log file: " + filename);
    }
}

FileObserver::~FileObserver() {
    if (logFile.is_open()) logFile.close();
}

void FileObserver::onKill(const std::string& killer, const std::string& victim) {
    logFile << "[KILL] " << killer << " killed " << victim << std::endl;
}

bool Dungeon::addNPC(std::shared_ptr<NPC> npc) {
    if (!npc) return false;
    npcs.push_back(npc);
    return true;
}

void Dungeon::print() const {
    for (const auto& npc : npcs) {
        if (npc->isAlive()) {
            std::cout << "[" << npc->getType() << "] "
                      << npc->getName() << " @ (" << npc->getX() << ", " << npc->getY() << ")\n";
        }
    }
}

void Dungeon::save(const std::string& filename) const {
    std::ofstream os(filename);
    if (!os) throw std::runtime_error("Cannot write to file: " + filename);
    for (const auto& npc : npcs) {
        npc->save(os);
    }
}

void Dungeon::load(const std::string& filename) {
    std::ifstream is(filename);
    if (!is) throw std::runtime_error("Cannot read from file: " + filename);

    npcs.clear();
    while (is.peek() != EOF) {
        auto npc = NPC::load(is);
        if (npc) npcs.push_back(npc);
    }
}

void Dungeon::addObserver(std::shared_ptr<Observer> obs) {
    observers.push_back(obs);
}

void Dungeon::notifyKill(const std::string& killer, const std::string& victim) {
    for (auto& obs : observers) {
        obs->onKill(killer, victim);
    }
}

void Dungeon::battle(double range) {
    for (size_t i = 0; i < npcs.size(); ++i) {
        if (!npcs[i]->isAlive()) continue;
        for (size_t j = i + 1; j < npcs.size(); ++j) {
            if (!npcs[j]->isAlive()) continue;

            double dist = std::hypot(npcs[i]->getX() - npcs[j]->getX(),
                                     npcs[i]->getY() - npcs[j]->getY());
            if (dist <= range) {
                BattleVisitor visitor_i(npcs[i], range);
                npcs[j]->accept(visitor_i);
                if (visitor_i.wasKill()) {
                    notifyKill(visitor_i.getKiller(), visitor_i.getVictim());
                }

                BattleVisitor visitor_j(npcs[j], range);
                npcs[i]->accept(visitor_j);
                if (visitor_j.wasKill()) {
                    notifyKill(visitor_j.getKiller(), visitor_j.getVictim());
                }
            }
        }
    }
}