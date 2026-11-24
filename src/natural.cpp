#include "map_mode.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace
{
// Light lowland, woodland green, ochre hills, cool rock and blue-green wetland.
// These are class colours, not a hypsometric or elevation ramp.
constexpr std::array<Ink, 5> terrainColors{
    0xDDE6B2, // Plains
    0x79A376, // Forest
    0xC8AE78, // Hills
    0xAAA7A3, // Mountains
    0x82B9AE, // Marsh
};

struct TerrainInfo
{
    size_t index;
    const wchar_t *name;
};

TerrainInfo terrainInfo(Terrain terrain)
{
    switch (terrain)
    {
    case Terrain::Plains:
        return {0, L"Plains"};
    case Terrain::Forest:
        return {1, L"Forest"};
    case Terrain::Hills:
        return {2, L"Hills"};
    case Terrain::Mountains:
        return {3, L"Mountains"};
    case Terrain::Marsh:
        return {4, L"Marsh"};
    }
    // A manually constructed or future Tile must not cause an array access
    // outside the five classes understood by this mode.
    return {0, L"Unknown terrain"};
}

std::string trim(const std::string &value)
{
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos)
        return {};
    return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
}

std::vector<std::string> columns(const std::string &line)
{
    std::vector<std::string> result;
    size_t start = 0;
    for (;;)
    {
        const size_t end = line.find('|', start);
        result.push_back(trim(line.substr(start, end - start)));
        if (end == std::string::npos)
            return result;
        start = end + 1;
    }
}

int integer(const std::string &text, int base = 10)
{
    int result{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result, base);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::runtime_error("Invalid integer: " + text);
    return result;
}

Ink ink(const std::string &text)
{
    if (text.size() != 7 || text[0] != '#' ||
        text.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos)
        throw std::runtime_error("Invalid RGB colour: " + text);
    return static_cast<Ink>(integer(text.substr(1), 16));
}

fs::path naturalPath(fs::path data)
{
    // Accept either the sidecar itself or its companion .atlas path. This
    // keeps the mode convenient for callers which retain only the map path.
    if (data.extension() == ".atlas")
        data.replace_extension(".natural");
    return data;
}

std::vector<ModeLabelSet> loadLabels(const fs::path &path)
{
    std::vector<ModeLabelSet> labels;
    if (path.empty())
        return labels;

    std::error_code filesystemError;
    const bool exists = fs::exists(path, filesystemError);
    if (filesystemError)
        throw std::runtime_error("Cannot inspect natural-label sidecar: " + path.string());
    if (!exists)
        return labels;

    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open natural-label sidecar: " + path.string());

    enum class Section { Header, Labels, Hexes } section = Section::Header;
    std::map<std::string, size_t> ids;
    std::set<Hex> assigned;
    int lineNumber = 0;
    try
    {
        for (std::string line; std::getline(input, line);)
        {
            ++lineNumber;
            if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF"))
                line.erase(0, 3);
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;
            if (section == Section::Header && line == "NATURALATLAS 1")
            {
                section = Section::Labels;
                continue;
            }
            if (section == Section::Labels && line == "[labels]")
                continue;
            if (section == Section::Labels && line == "[hexes]")
            {
                section = Section::Hexes;
                continue;
            }

            const auto fields = columns(line);
            if (section == Section::Labels && fields.size() == 4)
            {
                if (fields[0].empty() || fields[1].empty() ||
                    !ids.emplace(fields[0], labels.size()).second)
                    throw std::runtime_error("Invalid or duplicate natural label");
                wide(fields[0]);
                labels.push_back({wide(fields[1]), wide(fields[2]), ink(fields[3]), {}});
            }
            else if (section == Section::Hexes && fields.size() == 4)
            {
                const auto label = ids.find(fields[0]);
                if (label == ids.end())
                    throw std::runtime_error("Unknown natural label: " + fields[0]);
                const UnitId id{integer(fields[1]), integer(fields[2]), integer(fields[3])};
                if (id.a < 0 || id.b < 0 || id.c < 0 ||
                    (id.a != 0 && id.b != 0 && id.c != 0))
                    throw std::runtime_error("Invalid non-canonical natural-label unit ID");
                const Hex hex = axial(id);
                if (hex.q < -100000 || hex.q > 100000 || hex.r < -100000 || hex.r > 100000)
                    throw std::runtime_error("Natural-label coordinate outside supported range");
                if (!assigned.insert(hex).second)
                    throw std::runtime_error("Duplicate natural-label hex");
                labels[label->second].hexes.push_back(hex);
            }
            else
                throw std::runtime_error("Unexpected natural sidecar row or section");
        }
        if (input.bad() || section != Section::Hexes)
            throw std::runtime_error("Natural sidecar is incomplete");
    }
    catch (const std::exception &error)
    {
        throw std::runtime_error(path.string() + ": line " + std::to_string(lineNumber) + ": " + error.what());
    }

    return labels;
}

class NaturalMode final : public ModeView
{
    const Map &map;
    std::vector<ModeLabelSet> naturalLabels;

public:
    NaturalMode(const Map &source, const fs::path &data)
        : map(source), naturalLabels(loadLabels(naturalPath(data)))
    {
    }

    ModeKind kind() const override { return ModeKind::Natural; }
    std::wstring name() const override { return L"Natural"; }

    Ink color(Hex, const Tile &tile) const override
    {
        // Region and kingdom membership deliberately play no part here.
        return terrainColors[terrainInfo(tile.terrain).index];
    }

    std::vector<ModeLabelSet> labels() const override
    {
        // Filter against the live map without rereading the sidecar after every
        // brush edit. Deleted or not-yet-painted coordinates never reach layout.
        std::vector<ModeLabelSet> result;
        for (const auto &label : naturalLabels)
        {
            ModeLabelSet current{label.name, label.detail, label.color, {}};
            current.hexes.reserve(label.hexes.size());
            std::copy_if(label.hexes.begin(), label.hexes.end(), std::back_inserter(current.hexes),
                         [this](Hex hex) { return map.tiles.contains(hex); });
            if (!current.hexes.empty())
                result.push_back(std::move(current));
        }
        return result;
    }

    std::vector<ModeLegendRow> legend() const override
    {
        return {
            {L"Plains - low-relief ground", terrainColors[0]},
            {L"Forest - wooded terrain", terrainColors[1]},
            {L"Hills - sloping or upland ground", terrainColors[2]},
            {L"Mountains - rugged high ground", terrainColors[3]},
            {L"Marsh - waterlogged lowland", terrainColors[4]},
        };
    }

    std::wstring status() const override
    {
        return std::to_wstring(map.tiles.size()) +
               L" hexes - schematic terrain classes; no measured elevation";
    }

    std::wstring inspector(Hex, const Tile &tile) const override
    {
        const auto terrain = terrainInfo(tile.terrain);
        return std::wstring(L"Terrain: ") + terrain.name +
               L" - schematic; no elevation/yield";
    }
};
} // namespace

ModePtr createNaturalMode(const Map &map, const fs::path &data)
{
    return std::make_unique<NaturalMode>(map, data);
}
