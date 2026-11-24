#pragma once
#include <array>
#include <compare>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct Hex { int q{}, r{}; auto operator<=>(const Hex&) const = default; };
struct UnitId { int a{}, b{}, c{}; auto operator<=>(const UnitId&) const = default; };
struct ProjectedGrid {
    std::string crs;
    double easting{}, northing{}, radius{};
    auto operator<=>(const ProjectedGrid&) const = default;
};
struct Kingdom { std::string id, name, ruler; };
struct Region { std::string id, name; uint32_t color; std::optional<size_t> kingdom; };
enum class Terrain { Plains, Forest, Hills, Mountains, Marsh };
inline constexpr std::array terrainNames{"Plains","Forest","Hills","Mountains","Marsh"};
struct Tile { size_t region; Terrain terrain; bool operator==(const Tile&) const = default; };
struct Map {
    std::string name;
    std::optional<ProjectedGrid> grid;
    std::vector<Kingdom> kingdoms;
    std::vector<Region> regions;
    std::map<Hex, Tile> tiles;
};
UnitId makeUnitId(Hex hex);
Hex axial(UnitId id);
std::string formatUnitId(UnitId id);
Map loadMap(const std::filesystem::path& path);
void saveMap(const Map& map, const std::filesystem::path& path);
std::wstring wide(const std::string& utf8);
std::string utf8(const std::wstring& value);
