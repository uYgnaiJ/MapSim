#include "map_mode.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace
{
constexpr Ink unknownColor = 0xA8A8A8;

struct CultureIdentity
{
    std::string id, name, description;
    Ink color{};
    std::vector<Hex> hexes;
};
struct AssignmentSpec
{
    std::string identity, confidence, note;
};
struct CultureSidecar
{
    std::string mapName, title;
    std::vector<CultureIdentity> identities;
    std::map<std::string, AssignmentSpec> regions;
    std::map<Hex, AssignmentSpec> hexes;
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
            break;
        start = end + 1;
    }
    return result;
}
[[noreturn]] void invalid(size_t line, const std::string &message)
{
    throw std::runtime_error("Culture sidecar line " + std::to_string(line) + ": " + message);
}
bool validId(const std::string &id)
{
    return !id.empty() && std::all_of(id.begin(), id.end(), [](unsigned char ch)
                                      { return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-'; });
}
int integer(const std::string &text, size_t line)
{
    int value{};
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc{} || end != text.data() + text.size())
        invalid(line, "Invalid integer: " + text);
    return value;
}
Ink color(const std::string &text, size_t line)
{
    if (text.size() != 7 || text[0] != '#' ||
        text.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos)
        invalid(line, "Identity color must be #RRGGBB");
    unsigned int value{};
    auto [end, error] = std::from_chars(text.data() + 1, text.data() + text.size(), value, 16);
    if (error != std::errc{} || end != text.data() + text.size())
        invalid(line, "Invalid identity color");
    return value;
}
Hex unitId(const std::string &text, size_t line)
{
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;)
    {
        size_t end = text.find('-', start);
        fields.push_back(text.substr(start, end - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    if (fields.size() != 3)
        invalid(line, "Unit ID must have the form aaa-bbb-ccc");
    UnitId id{integer(fields[0], line), integer(fields[1], line), integer(fields[2], line)};
    if (id.a < 0 || id.b < 0 || id.c < 0 || (id.a != 0 && id.b != 0 && id.c != 0))
        invalid(line, "Unit ID is not canonical and nonnegative");
    Hex hex = axial(id);
    if (makeUnitId(hex) != id)
        invalid(line, "Unit ID is not canonical");
    return hex;
}
void checkConfidence(const std::string &confidence, size_t line)
{
    if (confidence != "approximate" && confidence != "uncertain" && confidence != "unknown")
        invalid(line, "Confidence must be approximate, uncertain, or unknown");
}

CultureSidecar readSidecar(const Map &map, const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open culture sidecar");
    CultureSidecar result;
    enum class Section { Header, Identities, Regions, Hexes };
    Section section = Section::Header;
    size_t lineNo = 0;
    bool header = false, identitiesSection = false;
    std::set<std::string> identityIds{"unknown"}, regionIds;
    std::set<Ink> colors{unknownColor};
    for (const auto &region : map.regions)
        regionIds.insert(region.id);

    for (std::string line; std::getline(in, line);)
    {
        ++lineNo;
        if (lineNo == 1 && line.starts_with("\xEF\xBB\xBF"))
            line.erase(0, 3);
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        try
        {
            wide(line);
        }
        catch (...)
        {
            invalid(lineNo, "Text must be valid UTF-8");
        }
        if (!header)
        {
            if (line != "CULTUREATLAS 1")
                invalid(lineNo, "Expected CULTUREATLAS 1");
            header = true;
            continue;
        }
        if (section == Section::Header && line.starts_with("map = "))
        {
            if (!result.mapName.empty())
                invalid(lineNo, "Duplicate map name");
            result.mapName = trim(line.substr(6));
            if (result.mapName.empty())
                invalid(lineNo, "Map name cannot be empty");
            continue;
        }
        if (section == Section::Header && line.starts_with("title = "))
        {
            if (!result.title.empty())
                invalid(lineNo, "Duplicate title");
            result.title = trim(line.substr(8));
            if (result.title.empty())
                invalid(lineNo, "Title cannot be empty");
            continue;
        }
        if (line == "[identities]")
        {
            if (section != Section::Header || result.mapName.empty())
                invalid(lineNo, "Map name must precede [identities]");
            section = Section::Identities;
            identitiesSection = true;
            continue;
        }
        if (line == "[regions]")
        {
            if (section != Section::Identities)
                invalid(lineNo, "[regions] must follow [identities]");
            section = Section::Regions;
            continue;
        }
        if (line == "[hexes]")
        {
            if (section != Section::Regions)
                invalid(lineNo, "[hexes] must follow [regions]");
            section = Section::Hexes;
            continue;
        }

        auto cols = columns(line);
        if (section == Section::Identities && cols.size() == 4)
        {
            if (!validId(cols[0]) || cols[0] == "unknown" || cols[1].empty() || cols[3].empty() ||
                !identityIds.insert(cols[0]).second)
                invalid(lineNo, "Invalid or duplicate identity");
            Ink ink = color(cols[2], lineNo);
            if (!colors.insert(ink).second)
                invalid(lineNo, "Identity colors must be distinct");
            result.identities.push_back({cols[0], cols[1], cols[3], ink, {}});
        }
        else if ((section == Section::Regions || section == Section::Hexes) && cols.size() == 4)
        {
            if (cols[1].empty() || !identityIds.contains(cols[1]) || cols[3].empty())
                invalid(lineNo, "Unknown identity or empty assignment note");
            checkConfidence(cols[2], lineNo);
            if ((cols[2] == "unknown") != (cols[1] == "unknown"))
                invalid(lineNo, "Unknown identity and confidence must agree");
            AssignmentSpec spec{cols[1], cols[2], cols[3]};
            if (section == Section::Regions)
            {
                if (!validId(cols[0]) || !regionIds.contains(cols[0]))
                    invalid(lineNo, "Unknown atlas region: " + cols[0]);
                if (!result.regions.emplace(cols[0], std::move(spec)).second)
                    invalid(lineNo, "Duplicate atlas region assignment");
            }
            else
            {
                Hex hex = unitId(cols[0], lineNo);
                if (!map.tiles.contains(hex))
                    invalid(lineNo, "Unit override is outside the atlas");
                if (!result.hexes.emplace(hex, std::move(spec)).second)
                    invalid(lineNo, "Duplicate unit assignment");
            }
        }
        else
            invalid(lineNo, "Unexpected row or section");
    }
    if (in.bad() || !header || result.mapName.empty() || !identitiesSection)
        throw std::runtime_error("Culture sidecar is incomplete");
    if (result.mapName != map.name)
        throw std::runtime_error("Culture sidecar belongs to '" + result.mapName + "', not '" + map.name + "'");
    if (result.title.empty())
        result.title = "Mainstream cultural identities — " + map.name;
    return result;
}

class CultureMode final : public ModeView
{
    enum class Origin { Fallback, Region, Unit, Missing };
    struct Cell
    {
        size_t identity{};
        std::string confidence = "unknown", note;
        Origin origin = Origin::Fallback;
    };
    const Map &map_;
    fs::path source_;
    bool loaded_{};
    std::string title_;
    std::vector<CultureIdentity> identities_;
    std::map<std::string, size_t> identityIds_;
    std::map<Hex, Cell> cells_;
    size_t known_{}, explicitUnknown_{}, unassigned_{};

public:
    CultureMode(const Map &map, const fs::path &path) : map_(map), source_(path)
    {
        CultureSidecar sidecar;
        std::error_code error;
        loaded_ = !path.empty() && fs::exists(path, error);
        if (error)
            throw std::runtime_error("Cannot inspect culture sidecar path");
        if (loaded_)
        {
            sidecar = readSidecar(map, path);
            identities_ = std::move(sidecar.identities);
            title_ = std::move(sidecar.title);
        }
        else
            title_ = "Cultural identity — no sidecar";
        identities_.push_back({"unknown", "Unknown / unassigned",
                               "No single mainstream identity is asserted; this is a neutral evidentiary category, not an ethnic group.",
                               unknownColor, {}});
        for (size_t i = 0; i < identities_.size(); ++i)
            identityIds_.emplace(identities_[i].id, i);
        size_t unknown = identityIds_.at("unknown");

        for (const auto &[hex, tile] : map.tiles)
        {
            Cell resolved;
            resolved.identity = unknown;
            if (!loaded_)
                resolved.note = "No culture sidecar was found; no identity is inferred from the political region.";
            else
            {
                const AssignmentSpec *spec = nullptr;
                Origin origin = Origin::Missing;
                if (auto found = sidecar.hexes.find(hex); found != sidecar.hexes.end())
                {
                    spec = &found->second;
                    origin = Origin::Unit;
                }
                else if (auto found = sidecar.regions.find(map.regions.at(tile.region).id); found != sidecar.regions.end())
                {
                    spec = &found->second;
                    origin = Origin::Region;
                }
                if (spec)
                {
                    resolved.identity = identityIds_.at(spec->identity);
                    resolved.confidence = spec->confidence;
                    resolved.note = spec->note;
                    resolved.origin = origin;
                }
                else
                {
                    resolved.note = "The sidecar has no region or unit assignment for this hex; neutral unknown fallback.";
                    ++unassigned_;
                }
            }
            if (resolved.identity != unknown)
                ++known_;
            else if (loaded_ && resolved.origin != Origin::Missing)
                ++explicitUnknown_;
            identities_[resolved.identity].hexes.push_back(hex);
            cells_.emplace(hex, std::move(resolved));
        }
    }

    ModeKind kind() const override { return ModeKind::Culture; }
    std::wstring name() const override { return wide(title_); }
    Ink color(Hex hex, const Tile &) const override
    {
        auto found = cells_.find(hex);
        return found == cells_.end() ? unknownColor : identities_[found->second.identity].color;
    }

    std::vector<ModeLabelSet> labels() const override
    {
        std::vector<ModeLabelSet> result;
        for (const auto &identity : identities_)
        {
            if (identity.hexes.empty())
                continue;
            auto count = identity.hexes.size();
            result.push_back({wide(identity.name),
                              std::to_wstring(count) + (count == 1 ? L" hex · " : L" hexes · ") + wide(identity.description),
                              identity.color, identity.hexes});
        }
        return result;
    }
    std::vector<ModeLegendRow> legend() const override
    {
        std::vector<ModeLegendRow> result;
        for (const auto &identity : identities_)
        {
            if (identity.hexes.empty())
                continue;
            auto count = identity.hexes.size();
            result.push_back({wide(identity.name) + L" — " + std::to_wstring(count) + (count == 1 ? L" hex" : L" hexes"),
                              identity.color});
        }
        return result;
    }
    std::wstring status() const override
    {
        std::wostringstream out;
        if (!loaded_)
            out << L"No culture sidecar · " << cells_.size() << L" neutral unknown";
        else
        {
            out << known_ << L" classified · " << explicitUnknown_ << L" explicitly unknown";
            if (unassigned_)
                out << L" · " << unassigned_ << L" unassigned/unknown";
            out << L" · " << source_.filename().wstring();
        }
        return out.str();
    }
    std::wstring inspector(Hex hex, const Tile &tile) const override
    {
        auto found = cells_.find(hex);
        Cell outside;
        if (found == cells_.end())
        {
            outside.identity = identityIds_.at("unknown");
            outside.note = "Hex is outside the source atlas.";
        }
        const Cell &entry = found == cells_.end() ? outside : found->second;
        const auto &identity = identities_[entry.identity];
        std::wostringstream out;
        out << L"Unit " << wide(formatUnitId(makeUnitId(hex))) << L"  (q " << hex.q << L", r " << hex.r << L")";
        out << L"\nCultural identity: " << wide(identity.name);
        if (tile.region < map_.regions.size())
            out << L"\nAtlas region: " << wide(map_.regions[tile.region].name);
        out << L"\nConfidence: " << wide(entry.confidence);
        out << L"\nBasis: ";
        switch (entry.origin)
        {
        case Origin::Unit: out << L"unit override in culture sidecar"; break;
        case Origin::Region: out << L"atlas-region assignment in culture sidecar"; break;
        case Origin::Missing: out << L"neutral fallback for an unassigned atlas region"; break;
        case Origin::Fallback: out << (loaded_ ? L"neutral fallback outside the atlas" : L"neutral fallback; sidecar absent"); break;
        }
        out << L"\nIdentity scope: " << wide(identity.description);
        out << L"\nAssignment note: " << wide(entry.note);
        out << L"\nScope: broad mainstream/regional identity, not a claim about every inhabitant or a demographic percentage.";
        return out.str();
    }
};
} // namespace

ModePtr createCultureMode(const Map &map, const fs::path &data)
{
    return std::make_unique<CultureMode>(map, data);
}
