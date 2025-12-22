#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <cmath>
#include <random>

class Visitor;
class Observer;

class NPC : public std::enable_shared_from_this<NPC> {
protected:
    std::string name;
    int x, y;
    bool alive = true;
    int moveDistance;
    int killDistance;

public:
    NPC(const std::string& n, int x_, int y_, int moveDist, int killDist)
        : name(n), x(x_), y(y_), moveDistance(moveDist), killDistance(killDist) {}
    virtual ~NPC() = default;

    virtual void accept(Visitor& visitor) = 0;
    virtual bool fight(std::shared_ptr<NPC> other) = 0;

    virtual std::string getType() const = 0;
    const std::string& getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
    void kill() { alive = false; }

    void moveRandomly(int mapSizeX, int mapSizeY);
    bool isInRangeForKill(const NPC& other) const;
    int rollDice() const;

    virtual void save(std::ofstream& os) const = 0;
    static std::shared_ptr<NPC> load(std::ifstream& is);
};

class Bear : public NPC {
public:
    Bear(const std::string& n, int x, int y) : NPC(n, x, y, 5, 10) {}
    std::string getType() const override { return "Bear"; }
    void accept(Visitor& visitor) override;
    bool fight(std::shared_ptr<NPC> other) override;
    void save(std::ofstream& os) const override;
};

class Duck : public NPC {
public:
    Duck(const std::string& n, int x, int y) : NPC(n, x, y, 50, 10) {}
    std::string getType() const override { return "Duck"; }
    void accept(Visitor& visitor) override;
    bool fight(std::shared_ptr<NPC> other) override;
    void save(std::ofstream& os) const override;
};

class Desman : public NPC {
public:
    Desman(const std::string& n, int x, int y) : NPC(n, x, y, 5, 20) {}
    std::string getType() const override { return "Desman"; }
    void accept(Visitor& visitor) override;
    bool fight(std::shared_ptr<NPC> other) override;
    void save(std::ofstream& os) const override;
};

class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(std::shared_ptr<Bear> bear) = 0;
    virtual void visit(std::shared_ptr<Duck> duck) = 0;
    virtual void visit(std::shared_ptr<Desman> desman) = 0;
};

class BattleVisitor : public Visitor {
    std::shared_ptr<NPC> attacker;
    double range;
    std::string _killer, _victim;
    bool _killOccurred = false;

public:
    BattleVisitor(std::shared_ptr<NPC> a, double r) : attacker(a), range(r) {}

    void visit(std::shared_ptr<Bear> target) override;
    void visit(std::shared_ptr<Duck> target) override;
    void visit(std::shared_ptr<Desman> target) override;

    bool wasKill() const { return _killOccurred; }
    std::string getKiller() const { return _killer; }
    std::string getVictim() const { return _victim; }

private:
    double distance(const NPC& a, const NPC& b) const {
        return std::hypot(a.getX() - b.getX(), a.getY() - b.getY());
    }
};

class Observer {
public:
    virtual ~Observer() = default;
    virtual void onKill(const std::string& killer, const std::string& victim) = 0;
};

class ConsoleObserver : public Observer {
public:
    void onKill(const std::string& killer, const std::string& victim) override;
};

class FileObserver : public Observer {
    std::ofstream logFile;
public:
    FileObserver(const std::string& filename = "log.txt");
    ~FileObserver();
    void onKill(const std::string& killer, const std::string& victim) override;
};

class Dungeon {
    std::vector<std::shared_ptr<NPC>> npcs;
    std::vector<std::shared_ptr<Observer>> observers;

public:
    bool addNPC(std::shared_ptr<NPC> npc);
    void print() const;
    void save(const std::string& filename) const;
    void load(const std::string& filename);
    void battle(double range);
    void addObserver(std::shared_ptr<Observer> obs);
    void notifyKill(const std::string& killer, const std::string& victim);
};