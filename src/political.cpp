#include "map_mode.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace
{
constexpr Ink paper = 0xE8E4DC;
constexpr std::array fallbackPalette{
    0x536B8F, 0x7D9B76, 0xD2A64F, 0xB77E9C, 0x5E9E96, 0xA87961,
    0x7772A8, 0x8C9B55, 0x4F8FA8, 0xB26F72, 0x947A50, 0x6F8B7C,
};

std::string trim(const std::string &value)
{
    auto first = value.find_first_not_of(" \t\r");
    return first == std::string::npos ? "" : value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
}

std::vector<std::string> columns(const std::string &line)
{
    std::vector<std::string> result;
    size_t start = 0;
    for (;;)
    {
        size_t end = line.find('|', start);
        result.push_back(trim(line.substr(start, end - start)));
        if (end == std::string::npos)
            return result;
        start = end + 1;
    }
}

bool validId(const std::string &id)
{
    return !id.empty() && id.front() >= 'a' && id.front() <= 'z' &&
           id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789._-") == std::string::npos;
}

uint32_t parseColor(const std::string &text)
{
    if (text.size() != 7 || text.front() != '#' ||
        text.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos)
        throw std::runtime_error("Expected an RGB colour in #RRGGBB form");
    uint32_t result{};
    auto [end, error] = std::from_chars(text.data() + 1, text.data() + text.size(), result, 16);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::runtime_error("Invalid RGB colour");
    return result;
}

Ink blend(Ink from, Ink to, float amount)
{
    auto channel = [&](int shift)
    {
        float a = float((from >> shift) & 0xFF);
        float b = float((to >> shift) & 0xFF);
        return uint32_t(a + (b - a) * amount + 0.5F) << shift;
    };
    return channel(0) | channel(8) | channel(16);
}

struct Node
{
    std::string name, ruler, detail;
    Ink color{};
    bool independent{true};
    bool region{false};
    std::optional<size_t> parent;
    std::vector<size_t> children;
    std::vector<Hex> hexes;
    uint64_t population{};
};

struct State
{
    std::vector<Node> nodes;
    std::map<std::string, size_t> ids;
    std::vector<size_t> regionNodes, roots;
    std::vector<Hex> allHexes;
};

size_t addNode(State &state, const std::string &key, Node node)
{
    size_t index = state.nodes.size();
    if (!state.ids.emplace(key, index).second)
        throw std::runtime_error("Duplicate political or region ID");
    state.nodes.push_back(std::move(node));
    return index;
}

State fallbackState(const Map &map)
{
    State state;
    std::vector<size_t> kingdomNodes;
    kingdomNodes.reserve(map.kingdoms.size());
    for (size_t i = 0; i < map.kingdoms.size(); ++i)
    {
        const auto &kingdom = map.kingdoms[i];
        std::string id = kingdom.id.empty() ? "kingdom-" + std::to_string(i) : kingdom.id;
        kingdomNodes.push_back(addNode(state, "p:" + id,
                                       {kingdom.name, kingdom.ruler, {}, kingdom.color, kingdom.independent, false, std::nullopt}));
    }
    for (size_t i = 0; i < map.kingdoms.size(); ++i)
    {
        if (auto parent = map.kingdoms[i].parent)
        {
            if (*parent >= kingdomNodes.size())
                throw std::runtime_error("Kingdom parent index is out of range");
            state.nodes[kingdomNodes[i]].parent = kingdomNodes[*parent];
        }
    }

    state.regionNodes.reserve(map.regions.size());
    std::set<std::string> regionKeys;
    for (size_t i = 0; i < map.regions.size(); ++i)
    {
        const auto &region = map.regions[i];
        std::string key = "r:" + (region.id.empty() ? "region-" + std::to_string(i) : region.id);
        if (!regionKeys.insert(key).second)
            key += ":" + std::to_string(i);
        size_t node = addNode(state, key, {region.name, {}, {}, region.color, true, true, std::nullopt});
        state.regionNodes.push_back(node);
        if (auto kingdom = region.kingdom)
        {
            if (*kingdom >= kingdomNodes.size())
                throw std::runtime_error("Region kingdom index is out of range");
            state.nodes[node].parent = kingdomNodes[*kingdom];
        }
    }
    return state;
}

struct RawPolity
{
    std::string id, parent, name, ruler, detail;
    Ink color{};
    bool independent{true};
    size_t line{};
};

State sidecarState(const Map &map, const fs::path &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open political sidecar");

    enum class Section { Header, Map, Polities, Regions };
    Section section = Section::Header;
    std::vector<RawPolity> polities;
    std::map<std::string, std::pair<std::string, size_t>> assignments;
    bool sawMap = false, sawPolities = false, sawRegions = false;
    size_t lineNo = 0;
    try
    {
        for (std::string line; std::getline(input, line);)
        {
            ++lineNo;
            if (lineNo == 1 && line.starts_with("\xEF\xBB\xBF"))
                line.erase(0, 3);
            line = trim(line);
            if (line.empty() || line.front() == '#')
                continue;
            wide(line);
            if (section == Section::Header)
            {
                if (line != "FIELDATLAS-POLITICAL 1")
                    throw std::runtime_error("Expected FIELDATLAS-POLITICAL 1");
                section = Section::Map;
                continue;
            }
            if (section == Section::Map)
            {
                if (!line.starts_with("map = "))
                    throw std::runtime_error("Expected map = <map name>");
                if (trim(line.substr(6)) != map.name)
                    throw std::runtime_error("Political sidecar belongs to a different map");
                sawMap = true;
                section = Section::Polities;
                continue;
            }
            if (line == "[polities]")
            {
                if (sawPolities || sawRegions)
                    throw std::runtime_error("Duplicate or out-of-order [polities] section");
                sawPolities = true;
                section = Section::Polities;
                continue;
            }
            if (line == "[regions]")
            {
                if (!sawPolities || sawRegions || polities.empty())
                    throw std::runtime_error("Missing, duplicate, or out-of-order [regions] section");
                sawRegions = true;
                section = Section::Regions;
                continue;
            }
            auto cols = columns(line);
            if (section == Section::Polities && !sawPolities)
                throw std::runtime_error("Expected [polities] before political rows");
            if (section == Section::Polities && cols.size() == 7)
            {
                if (!validId(cols[0]) || (cols[1] != "-" && !validId(cols[1])) || cols[2].empty())
                    throw std::runtime_error("Invalid political ID, parent, or name");
                bool independent{};
                if (cols[4] == "true")
                    independent = true;
                else if (cols[4] == "false")
                    independent = false;
                else
                    throw std::runtime_error("Independence must be true or false");
                polities.push_back({cols[0], cols[1], cols[2], cols[3], cols[6],
                                    parseColor(cols[5]), independent, lineNo});
            }
            else if (section == Section::Regions && cols.size() == 2)
            {
                if (!validId(cols[1]) || cols[0].empty() ||
                    !assignments.emplace(cols[0], std::pair{cols[1], lineNo}).second)
                    throw std::runtime_error("Invalid or duplicate region assignment");
            }
            else
                throw std::runtime_error("Unexpected political row or section");
        }
        if (input.bad() || !sawMap || !sawPolities || !sawRegions)
            throw std::runtime_error("Political sidecar is incomplete");
    }
    catch (const std::exception &error)
    {
        throw std::runtime_error("Line " + std::to_string(lineNo) + ": " + error.what());
    }

    State state;
    std::map<std::string, size_t> rawIds;
    for (const auto &raw : polities)
    {
        if (!rawIds.emplace(raw.id, state.nodes.size()).second)
            throw std::runtime_error("Line " + std::to_string(raw.line) + ": Duplicate political ID: " + raw.id);
        addNode(state, "p:" + raw.id, {raw.name, raw.ruler, raw.detail, raw.color, raw.independent, false, std::nullopt});
    }
    for (size_t i = 0; i < polities.size(); ++i)
    {
        if (polities[i].parent == "-")
            continue;
        auto found = rawIds.find(polities[i].parent);
        if (found == rawIds.end())
            throw std::runtime_error("Line " + std::to_string(polities[i].line) + ": Unknown parent: " + polities[i].parent);
        if (found->second == i)
            throw std::runtime_error("Line " + std::to_string(polities[i].line) + ": A polity cannot parent itself");
        state.nodes[i].parent = found->second;
    }

    std::map<std::string, size_t> mapRegions;
    for (size_t i = 0; i < map.regions.size(); ++i)
        if (!mapRegions.emplace(map.regions[i].id, i).second)
            throw std::runtime_error("Duplicate region ID in map");
    state.regionNodes.resize(map.regions.size());
    std::set<size_t> assigned;
    for (const auto &[regionId, assignment] : assignments)
    {
        auto region = mapRegions.find(regionId);
        if (region == mapRegions.end())
            throw std::runtime_error("Line " + std::to_string(assignment.second) + ": Unknown map region: " + regionId);
        auto parent = rawIds.find(assignment.first);
        if (parent == rawIds.end())
            throw std::runtime_error("Line " + std::to_string(assignment.second) + ": Unknown leaf polity: " + assignment.first);
        size_t i = region->second;
        const auto &source = map.regions[i];
        size_t node = addNode(state, "r:" + regionId, {source.name, {}, {}, source.color, true, true, std::nullopt});
        state.nodes[node].parent = parent->second;
        state.regionNodes[i] = node;
        assigned.insert(i);
    }
    if (assigned.size() != map.regions.size())
        throw std::runtime_error("Every map region must have exactly one political assignment");
    return state;
}

void finalise(const Map &map, State &state)
{
    for (size_t i = 0; i < state.nodes.size(); ++i)
    {
        if (auto parent = state.nodes[i].parent)
        {
            if (*parent >= state.nodes.size() || *parent == i)
                throw std::runtime_error("Invalid political parent index");
            state.nodes[*parent].children.push_back(i);
        }
        else
            state.roots.push_back(i);
    }

    std::vector<int> mark(state.nodes.size(), 0);
    auto visit = [&](auto &&self, size_t index) -> void
    {
        if (mark[index] == 1)
            throw std::runtime_error("Cycle in political parent chain");
        if (mark[index] == 2)
            return;
        mark[index] = 1;
        if (auto parent = state.nodes[index].parent)
            self(self, *parent);
        mark[index] = 2;
    };
    for (size_t i = 0; i < state.nodes.size(); ++i)
        visit(visit, i);

    size_t rootColour = 0;
    for (size_t root : state.roots)
        if (state.nodes[root].color == 0)
            state.nodes[root].color = fallbackPalette[rootColour++ % fallbackPalette.size()];
    auto inherit = [&](auto &&self, size_t index) -> void
    {
        for (size_t child : state.nodes[index].children)
        {
            if (state.nodes[child].color == 0)
                state.nodes[child].color = state.nodes[index].color;
            self(self, child);
        }
    };
    for (size_t root : state.roots)
        inherit(inherit, root);

    if (state.regionNodes.size() != map.regions.size())
        throw std::runtime_error("Political region index is incomplete");
    for (const auto &[hex, tile] : map.tiles)
    {
        state.allHexes.push_back(hex);
        if (tile.region >= state.regionNodes.size())
            continue;
        auto &region = state.nodes[state.regionNodes[tile.region]];
        region.hexes.push_back(hex);
        region.population += tile.population;
    }
    auto aggregate = [&](auto &&self, size_t index) -> void
    {
        auto &node = state.nodes[index];
        for (size_t child : node.children)
        {
            self(self, child);
            const auto &source = state.nodes[child];
            node.hexes.insert(node.hexes.end(), source.hexes.begin(), source.hexes.end());
            node.population += source.population;
        }
        std::sort(node.hexes.begin(), node.hexes.end());
    };
    for (size_t root : state.roots)
        aggregate(aggregate, root);
}

fs::path sidecarPath(const fs::path &data)
{
    if (data.empty())
        return {};
    if (data.extension() == ".atlas")
    {
        fs::path result = data;
        result.replace_extension(".political");
        return result;
    }
    return data;
}

bool fileExists(const fs::path &path)
{
    if (path.empty())
        return false;
    std::error_code error;
    return fs::is_regular_file(path, error) && !error;
}

class PoliticalMode final : public ModeView
{
    const Map &map_;
    fs::path data_;
    State state_;
    std::vector<size_t> path_;

    std::optional<size_t> displayedNode(size_t leaf) const
    {
        if (leaf >= state_.nodes.size())
            return std::nullopt;
        if (path_.empty())
        {
            while (state_.nodes[leaf].parent)
                leaf = *state_.nodes[leaf].parent;
            return leaf;
        }
        size_t current = path_.back();
        if (leaf == current)
            return current;
        while (state_.nodes[leaf].parent && *state_.nodes[leaf].parent != current)
            leaf = *state_.nodes[leaf].parent;
        if (!state_.nodes[leaf].parent || *state_.nodes[leaf].parent != current)
            return std::nullopt;
        return leaf;
    }

    std::optional<size_t> tileLeaf(Hex hex) const
    {
        auto found = map_.tiles.find(hex);
        if (found == map_.tiles.end() || found->second.region >= state_.regionNodes.size())
            return std::nullopt;
        return state_.regionNodes[found->second.region];
    }

    std::vector<size_t> visible() const
    {
        if (path_.empty())
            return state_.roots;
        const auto &children = state_.nodes[path_.back()].children;
        if (!children.empty())
            return children;
        return {path_.back()};
    }

    std::wstring pathText() const
    {
        std::wstring result;
        for (size_t index : path_)
        {
            if (!result.empty())
                result += L" › ";
            result += wide(state_.nodes[index].name);
        }
        return result;
    }

public:
    PoliticalMode(const Map &map, const fs::path &data) : map_(map), data_(sidecarPath(data)) { refresh(); }

    ModeKind kind() const override { return ModeKind::Political; }
    std::wstring name() const override { return L"Political"; }

    Ink color(Hex, const Tile &tile) const override
    {
        if (tile.region >= state_.regionNodes.size())
            return paper;
        size_t leaf = state_.regionNodes[tile.region];
        Ink base = state_.nodes[leaf].color;
        auto shown = displayedNode(leaf);
        if (!shown)
        {
            size_t root = leaf;
            while (state_.nodes[root].parent)
                root = *state_.nodes[root].parent;
            base = state_.nodes[root].color;
            return blend(base, paper, 0.74F);
        }
        return state_.nodes[*shown].color;
    }

    std::vector<ModeLabelSet> labels() const override
    {
        std::vector<ModeLabelSet> result;
        for (size_t index : visible())
        {
            const auto &node = state_.nodes[index];
            if (node.hexes.empty())
                continue;
            // The map's second line is deliberately concise; longer historical
            // qualifications remain available in inspector() and the sidecar.
            result.push_back({wide(node.name), wide(node.ruler), node.color, node.hexes});
        }
        return result;
    }

    std::vector<ModeLegendRow> legend() const override
    {
        std::vector<ModeLegendRow> result;
        for (size_t index : visible())
        {
            const auto &node = state_.nodes[index];
            if (node.hexes.empty())
                continue;
            std::wstring text = wide(node.name);
            if (!node.ruler.empty())
                text += L" — " + wide(node.ruler);
            result.push_back({text, node.color});
        }
        return result;
    }

    std::wstring status() const override
    {
        std::wstring path = pathText();
        return L"Political authority · " + (path.empty() ? L"Root / effective authorities" : path);
    }

    std::wstring inspector(Hex, const Tile &tile) const override
    {
        if (tile.region >= map_.regions.size() || tile.region >= state_.regionNodes.size())
            return L"No valid political assignment";
        size_t leaf = state_.regionNodes[tile.region];
        size_t authority = leaf;
        if (state_.nodes[leaf].region && state_.nodes[leaf].parent)
            authority = *state_.nodes[leaf].parent;

        std::vector<size_t> chain;
        for (size_t at = authority;;)
        {
            chain.push_back(at);
            if (!state_.nodes[at].parent)
                break;
            at = *state_.nodes[at].parent;
        }
        std::reverse(chain.begin(), chain.end());
        std::wostringstream out;
        out << L"Region: " << wide(map_.regions[tile.region].name);
        const auto &node = state_.nodes[authority];
        out << L"\nPolity: " << wide(node.name);
        if (!node.ruler.empty())
            out << L"\nRuler: " << wide(node.ruler);
        out << L"\nStatus: " << (node.independent ? L"Independent / effective authority" : L"Client or subordinate");
        if (!node.detail.empty())
            out << L"\n" << wide(node.detail);
        out << L"\nAuthority path: ";
        for (size_t i = 0; i < chain.size(); ++i)
        {
            if (i)
                out << L" › ";
            out << wide(state_.nodes[chain[i]].name);
        }
        out << L"\nTerritory: " << node.hexes.size() << L" hexes";
        if (!path_.empty())
            out << L"\nCurrent view: " << pathText();
        return out.str();
    }

    std::vector<Hex> activeHexes() const override
    {
        return path_.empty() ? state_.allHexes : state_.nodes[path_.back()].hexes;
    }

    bool descend(Hex hex) override
    {
        auto leaf = tileLeaf(hex);
        if (!leaf)
            return false;
        auto shown = displayedNode(*leaf);
        if (!shown || state_.nodes[*shown].children.empty())
            return false;
        path_.push_back(*shown);
        return true;
    }

    void reset() override { path_.clear(); }

    void refresh() override
    {
        std::vector<std::string> previous;
        for (size_t index : path_)
            for (const auto &[key, candidate] : state_.ids)
                if (candidate == index)
                {
                    previous.push_back(key);
                    break;
                }

        State next = fileExists(data_) ? sidecarState(map_, data_) : fallbackState(map_);
        finalise(map_, next);
        state_ = std::move(next);
        path_.clear();
        for (const auto &key : previous)
        {
            auto found = state_.ids.find(key);
            if (found == state_.ids.end() || state_.nodes[found->second].children.empty())
                break;
            size_t index = found->second;
            if ((!path_.empty() && state_.nodes[index].parent != path_.back()) ||
                (path_.empty() && state_.nodes[index].parent))
                break;
            path_.push_back(index);
        }
    }
};
} // namespace

ModePtr createPoliticalMode(const Map &map, const fs::path &data)
{
    return std::make_unique<PoliticalMode>(map, data);
}
