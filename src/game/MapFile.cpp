#include "game/MapFile.hpp"

#include <sstream>

namespace {
// Variation de taille des éléments semés au hasard (scatter).
constexpr float kScatterScaleJitter = 0.2f;

// Générateur pseudo-aléatoire déterministe : la carte est identique à chaque partie.
class Lcg {
public:
    explicit Lcg(unsigned seed) : state_(seed * 2654435761u + 1u) {}

    float next(float min, float max)
    {
        state_ = state_ * 1664525u + 1013904223u;
        return min + (max - min) * static_cast<float>(state_ >> 8) / static_cast<float>(1u << 24);
    }

private:
    unsigned state_;
};

bool parseCollision(const std::string& word, PropCollision& out)
{
    if (word == "-") out = PropCollision::None;
    else if (word == "solid") out = PropCollision::Solid;
    else if (word == "trunk") out = PropCollision::Trunk;
    else return false;
    return true;
}

// Chaque commande lit ses champs dans `in` ; renvoie faux si la ligne est mal formée.
bool parseProp(std::istringstream& in, MapData& map)
{
    PropDef p;
    std::string collision;
    if (!(in >> p.model >> p.position.x >> p.position.z >> p.yaw >> p.scale >> collision)) return false;
    if (!parseCollision(collision, p.collision)) return false;
    if (!(in >> p.position.y)) p.position.y = 0.0f; // hauteur facultative (toits...)
    map.props.push_back(p);
    return true;
}

bool parseRow(std::istringstream& in, MapData& map)
{
    PropDef p;
    Vector2 from;
    Vector2 to;
    int count = 0;
    std::string collision;
    if (!(in >> p.model >> from.x >> from.y >> to.x >> to.y >> count >> p.yaw >> p.scale >> collision)) return false;
    if (count < 1 || !parseCollision(collision, p.collision)) return false;
    for (int i = 0; i < count; ++i) {
        const float t = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.5f;
        p.position = {from.x + (to.x - from.x) * t, 0.0f, from.y + (to.y - from.y) * t};
        map.props.push_back(p);
    }
    return true;
}

bool parseScatter(std::istringstream& in, MapData& map)
{
    PropDef p;
    Vector2 min;
    Vector2 max;
    int count = 0;
    float scale = 1.0f;
    unsigned seed = 0;
    std::string collision;
    if (!(in >> p.model >> min.x >> min.y >> max.x >> max.y >> count >> scale >> collision >> seed)) return false;
    if (count < 1 || !parseCollision(collision, p.collision)) return false;
    Lcg rng(seed);
    for (int i = 0; i < count; ++i) {
        p.position = {rng.next(min.x, max.x), 0.0f, rng.next(min.y, max.y)};
        p.yaw = rng.next(0.0f, 360.0f);
        p.scale = scale * rng.next(1.0f - kScatterScaleJitter, 1.0f + kScatterScaleJitter);
        map.props.push_back(p);
    }
    return true;
}

bool parseWall(std::istringstream& in, MapData& map)
{
    WallDef w;
    if (!(in >> w.model >> w.from.x >> w.from.y >> w.to.x >> w.to.y)) return false;
    if (!(in >> w.scale)) w.scale = 1.0f; // échelle facultative
    map.walls.push_back(w);
    return true;
}

bool parseZone(std::istringstream& in, MapData& map)
{
    ZoneDef z;
    if (!(in >> z.id >> z.price >> z.min.x >> z.min.y >> z.max.x >> z.max.y)) return false;
    std::getline(in >> std::ws, z.name);
    if (z.name.empty()) z.name = z.id;
    map.zones.push_back(z);
    return true;
}

bool parseGate(std::istringstream& in, MapData& map)
{
    GateDef g;
    if (!(in >> g.zone >> g.position.x >> g.position.z >> g.yaw)) return false;
    map.gates.push_back(g);
    return true;
}

bool parseSpawn(std::istringstream& in, MapData& map)
{
    SpawnDef s;
    if (!(in >> s.zone >> s.position.x >> s.position.z)) return false;
    map.spawns.push_back(s);
    return true;
}

bool parsePlayer(std::istringstream& in, MapData& map)
{
    return static_cast<bool>(in >> map.playerSpawn.x >> map.playerSpawn.z);
}
}

MapData loadMapFile(const std::string& path)
{
    MapData map;
    char* text = LoadFileText(path.c_str());
    if (!text) {
        TraceLog(LOG_WARNING, "MAP: impossible de lire %s", path.c_str());
        return map;
    }
    std::istringstream file(text);
    UnloadFileText(text);

    std::string line;
    int number = 0;
    while (std::getline(file, line)) {
        ++number;
        const size_t comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        std::istringstream in(line);
        std::string command;
        if (!(in >> command)) continue;

        bool ok = false;
        if (command == "prop") ok = parseProp(in, map);
        else if (command == "row") ok = parseRow(in, map);
        else if (command == "scatter") ok = parseScatter(in, map);
        else if (command == "wall") ok = parseWall(in, map);
        else if (command == "zone") ok = parseZone(in, map);
        else if (command == "gate") ok = parseGate(in, map);
        else if (command == "spawn") ok = parseSpawn(in, map);
        else if (command == "player") ok = parsePlayer(in, map);
        if (!ok) TraceLog(LOG_WARNING, "MAP: %s:%d ligne ignoree : %s", path.c_str(), number, line.c_str());
    }
    return map;
}
