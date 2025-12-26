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

std::tuple<int, int> NPC::position() const {
    std::lock_guard<std::mutex> lck(mtx);
    return std::make_tuple(x, y);
}

void NPC::subscribe(std::shared_ptr<IFightObserver> observer) {
    std::lock_guard<std::mutex> lck(mtx);
    observers.push_back(observer);
}

void NPC::fight_notify(const std::shared_ptr<NPC> defender, bool win) {
    std::lock_guard<std::mutex> lck(mtx);
    auto self = shared_from_this();
    for (auto &o : observers) {
        o->on_fight(self, defender, win);
    }
}

bool NPC::is_close(const std::shared_ptr<NPC> &other, size_t distance) {
    std::lock_guard<std::mutex> lck(mtx);
    const auto [other_x, other_y] = other->position();
    if ((std::pow(x - other_x, 2) + std::pow(y - other_y, 2)) <= std::pow(distance, 2))
        return true;
    else
        return false;
}

void NPC::print(std::ostream& os) const {
    os << "[" << getType() << "] " << name << " @ (" << x << ", " << y << ")\n";
}

void Bear::accept(Visitor& visitor) {
    visitor.visit(std::static_pointer_cast<Bear>(shared_from_this()));
}

bool Bear::fight(std::shared_ptr<NPC> other) {
    if (!other || !other->isAlive()) return false;

    if (dynamic_cast<Duck*>(other.get()) || dynamic_cast<Desman*>(other.get())) {
        int attackPower = rollDice();
        int defensePower = other->rollDice();

        if (attackPower > defensePower) {
            other->kill();
            fight_notify(other, true);
            return true;
        }
        fight_notify(other, false);
        return true;
    }

    return false;
}

void Bear::save(std::ofstream& os) const {
    os << "Bear " << name << " " << x << " " << y << "\n";
}

std::mutex TextObserver::print_mutex;

void Duck::accept(Visitor& visitor) {
    visitor.visit(std::static_pointer_cast<Duck>(shared_from_this()));
}

bool Duck::fight(std::shared_ptr<NPC> other) {
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
        int attackPower = rollDice();
        int defensePower = other->rollDice();

        if (attackPower > defensePower) {
            other->kill();
            fight_notify(other, true);
            return true;
        }
        fight_notify(other, false);
        return true;
    }

    return false;
}

void Desman::save(std::ofstream& os) const {
    os << "Desman " << name << " " << x << " " << y << "\n";
}

void BattleVisitor::visit(std::shared_ptr<Bear> target) {
    if (!attacker || !attacker->isAlive() || !target || !target->isAlive()) return;
    if (attacker->isInRangeForKill(*target)) {
        if (attacker->fight(target)) {
            _killer = attacker->getName();
            _victim = target->getName();
            _killOccurred = true;
        }
    }
}

void BattleVisitor::visit(std::shared_ptr<Duck> target) {
    if (!attacker || !attacker->isAlive() || !target || !target->isAlive()) return;
    if (attacker->isInRangeForKill(*target)) {
        if (attacker->fight(target)) {
            _killer = attacker->getName();
            _victim = target->getName();
            _killOccurred = true;
        }
    }
}

void BattleVisitor::visit(std::shared_ptr<Desman> target) {
    if (!attacker || !attacker->isAlive() || !target || !target->isAlive()) return;
    if (attacker->isInRangeForKill(*target)) {
        if (attacker->fight(target)) {
            _killer = attacker->getName();
            _victim = target->getName();
            _killOccurred = true;
        }
    }
}