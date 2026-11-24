#include "map_mode.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace
{
constexpr Ink unknownColor = 0xA8A8A8;

struct ReligionIdentity
{
    std::string id, name, description;
    Ink color{};
    std::vector<Hex> hexes;
};
struct AssignmentSpec
{
    std::string identity, confidence, note;
};
struct MilitaryContext
{
    std::string group, affiliation, note;
};
struct ReligionSidecar
{
    std::string mapName, title;
    std::vector<ReligionIdentity> identities;
    std::map<std::string, AssignmentSpec> regions;
    std::map<Hex, AssignmentSpec> units;
    std::map<Hex, std::vector<MilitaryContext>> contexts;
};

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
[[noreturn]] void invalid(size_t line, const std::string &message)
{
    throw std::runtime_error("Religion sidecar line " + std::to_string(line) + ": " + message);
}
bool validId(const std::string &id)
{
    return !id.empty() && std::all_of(id.begin(), id.end(), [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-';
    });
}
int integer(const std::string &text, size_t line)
{
    int value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc{} || end != text.data() + text.size())
        invalid(line, "Invalid integer: " + text);
    return value;
}
Ink colorValue(const std::string &text, size_t line)
{
    if (text.size() != 7 || text[0] != '#' ||
        text.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos)
        invalid(line, "Identity colour must be #RRGGBB");
    unsigned int value{};
    const auto [end, error] = std::from_chars(text.data() + 1, text.data() + text.size(), value, 16);
    if (error != std::errc{} || end != text.data() + text.size())
        invalid(line, "Invalid identity colour");
    return static_cast<Ink>(value);
}
Hex unitId(const std::string &text, size_t line)
{
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;)
    {
        const size_t end = text.find('-', start);
        fields.push_back(text.substr(start, end - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    if (fields.size() != 3)
        invalid(line, "Unit ID must have the form aaa-bbb-ccc");
    const UnitId id{integer(fields[0], line), integer(fields[1], line), integer(fields[2], line)};
    if (id.a < 0 || id.b < 0 || id.c < 0 || (id.a != 0 && id.b != 0 && id.c != 0))
        invalid(line, "Unit ID is not canonical and nonnegative");
    const long long q = static_cast<long long>(id.a) - id.c;
    const long long r = static_cast<long long>(id.c) - id.b;
    if (q < -100000 || q > 100000 || r < -100000 || r > 100000)
        invalid(line, "Unit coordinate is outside the supported range");
    const Hex hex{static_cast<int>(q), static_cast<int>(r)};
    if (makeUnitId(hex) != id)
        invalid(line, "Unit ID is not canonical");
    return hex;
}
void checkConfidence(const std::string &confidence, size_t line)
{
    if (confidence != "approximate" && confidence != "uncertain" && confidence != "unknown")
        invalid(line, "Confidence must be approximate, uncertain, or unknown");
}

ReligionSidecar readSidecar(const Map &map, const fs::path &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open religion sidecar");
    ReligionSidecar result;
    enum class Section { Header, Identities, Regions, Units, MilitaryContext };
    Section section = Section::Header;
    size_t lineNo = 0;
    bool header = false, identitiesSection = false, regionsSection = false, unitsSection = false;
    std::set<std::string> identityIds{"unknown"}, regionIds;
    std::set<Ink> colors{unknownColor};
    for (const auto &region : map.regions)
        regionIds.insert(region.id);

    for (std::string line; std::getline(input, line);)
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
            if (line != "RELIGIONATLAS 1")
                invalid(lineNo, "Expected RELIGIONATLAS 1");
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
            if (section != Section::Identities || result.identities.empty())
                invalid(lineNo, "[regions] must follow a nonempty [identities] section");
            section = Section::Regions;
            regionsSection = true;
            continue;
        }
        if (line == "[units]")
        {
            if (section != Section::Regions)
                invalid(lineNo, "[units] must follow [regions]");
            section = Section::Units;
            unitsSection = true;
            continue;
        }
        if (line == "[military-context]")
        {
            if (section != Section::Units)
                invalid(lineNo, "[military-context] must follow [units]");
            section = Section::MilitaryContext;
            continue;
        }

        const auto fields = columns(line);
        if (section == Section::Identities && fields.size() == 4)
        {
            if (!validId(fields[0]) || fields[0] == "unknown" || fields[1].empty() || fields[3].empty() ||
                !identityIds.insert(fields[0]).second)
                invalid(lineNo, "Invalid or duplicate identity");
            const Ink ink = colorValue(fields[2], lineNo);
            if (!colors.insert(ink).second)
                invalid(lineNo, "Identity colours must be distinct");
            result.identities.push_back({fields[0], fields[1], fields[3], ink, {}});
        }
        else if ((section == Section::Regions || section == Section::Units) && fields.size() == 4)
        {
            if (!identityIds.contains(fields[1]) || fields[3].empty())
                invalid(lineNo, "Unknown identity or empty assignment note");
            checkConfidence(fields[2], lineNo);
            if ((fields[1] == "unknown") != (fields[2] == "unknown"))
                invalid(lineNo, "The unknown identity and unknown confidence must be used together");
            AssignmentSpec spec{fields[1], fields[2], fields[3]};
            if (section == Section::Regions)
            {
                if (!validId(fields[0]) || !regionIds.contains(fields[0]))
                    invalid(lineNo, "Unknown atlas region: " + fields[0]);
                if (!result.regions.emplace(fields[0], std::move(spec)).second)
                    invalid(lineNo, "Duplicate atlas region assignment");
            }
            else
            {
                const Hex hex = unitId(fields[0], lineNo);
                if (!map.tiles.contains(hex))
                    invalid(lineNo, "Unit override is outside the atlas");
                if (!result.units.emplace(hex, std::move(spec)).second)
                    invalid(lineNo, "Duplicate unit assignment");
            }
        }
        else if (section == Section::MilitaryContext && fields.size() == 4)
        {
            if (fields[1].empty() || fields[2].empty() || fields[3].empty())
                invalid(lineNo, "Military context needs a group, affiliation, and scope note");
            const Hex hex = unitId(fields[0], lineNo);
            if (!map.tiles.contains(hex))
                invalid(lineNo, "Military-context unit is outside the atlas");
            result.contexts[hex].push_back({fields[1], fields[2], fields[3]});
        }
        else
            invalid(lineNo, "Unexpected row or section");
    }
    if (input.bad() || !header || result.mapName.empty() || !identitiesSection || !regionsSection || !unitsSection)
        throw std::runtime_error("Religion sidecar is incomplete");
    if (result.mapName != map.name)
        throw std::runtime_error("Religion sidecar belongs to '" + result.mapName + "', not '" + map.name + "'");
    if (result.title.empty())
        result.title = "Majority-population religion - " + map.name;
    return result;
}

fs::path sidecarPath(fs::path data)
{
    if (data.extension() == ".atlas")
        data.replace_extension(".religion");
    return data;
}

class ReligionMode final : public ModeView
{
    enum class LoadState { Missing, Loaded, Error };
    enum class Origin { Outside, Missing, Error, Region, Unit, Unassigned };
    struct Cell
    {
        size_t identity{};
        std::string confidence = "unknown", note;
        Origin origin = Origin::Outside;
    };

    const Map &map_;
    fs::path source_;
    LoadState state_ = LoadState::Missing;
    std::string title_, diagnostic_;
    std::vector<ReligionIdentity> identities_;
    std::map<std::string, size_t> identityIds_;
    std::map<Hex, Cell> cells_;
    std::map<Hex, std::vector<MilitaryContext>> contexts_;
    size_t known_{}, explicitUnknown_{}, unassigned_{};

    const Cell &cell(Hex hex) const
    {
        static const Cell outside{0, "unknown", "Hex is outside the source atlas.", Origin::Outside};
        const auto found = cells_.find(hex);
        return found == cells_.end() ? outside : found->second;
    }

public:
    ReligionMode(const Map &map, const fs::path &data) : map_(map), source_(sidecarPath(data))
    {
        ReligionSidecar sidecar;
        std::error_code filesystemError;
        const bool present = !source_.empty() && fs::exists(source_, filesystemError);
        if (filesystemError)
        {
            state_ = LoadState::Error;
            diagnostic_ = "Cannot inspect the religion sidecar path.";
        }
        else if (present)
        {
            try
            {
                sidecar = readSidecar(map, source_);
                state_ = LoadState::Loaded;
            }
            catch (const std::exception &error)
            {
                state_ = LoadState::Error;
                diagnostic_ = error.what();
            }
        }

        identities_.push_back({"unknown", "Unknown / unassigned",
                               "Neutral evidentiary category: no majority religious identity is asserted; this is not a religion.",
                               unknownColor, {}});
        if (state_ == LoadState::Loaded)
        {
            title_ = std::move(sidecar.title);
            contexts_ = std::move(sidecar.contexts);
            for (auto &identity : sidecar.identities)
                identities_.push_back(std::move(identity));
        }
        else
            title_ = state_ == LoadState::Missing ? "Religion - no sidecar" : "Religion - sidecar error";
        for (size_t i = 0; i < identities_.size(); ++i)
            identityIds_.emplace(identities_[i].id, i);

        for (const auto &[hex, tile] : map.tiles)
        {
            Cell resolved;
            if (state_ == LoadState::Missing)
            {
                resolved.origin = Origin::Missing;
                resolved.note = "No religion sidecar was found; no identity is inferred from political control.";
            }
            else if (state_ == LoadState::Error)
            {
                resolved.origin = Origin::Error;
                resolved.note = "The religion sidecar could not be used; the whole layer falls back safely to neutral unknown.";
            }
            else
            {
                resolved.origin = Origin::Unassigned;
                resolved.note = "No unit or atlas-region assignment exists; neutral unknown fallback.";
                const AssignmentSpec *spec = nullptr;
                if (const auto found = sidecar.units.find(hex); found != sidecar.units.end())
                {
                    spec = &found->second;
                    resolved.origin = Origin::Unit;
                }
                else if (tile.region < map.regions.size())
                {
                    if (const auto found = sidecar.regions.find(map.regions[tile.region].id); found != sidecar.regions.end())
                    {
                        spec = &found->second;
                        resolved.origin = Origin::Region;
                    }
                }
                else
                    resolved.note = "The atlas tile has an invalid region index; neutral unknown fallback.";

                if (spec)
                {
                    resolved.identity = identityIds_.at(spec->identity);
                    resolved.confidence = spec->confidence;
                    resolved.note = spec->note;
                }
            }

            if (resolved.identity != 0)
                ++known_;
            else if (state_ == LoadState::Loaded && resolved.origin != Origin::Unassigned)
                ++explicitUnknown_;
            if (state_ == LoadState::Loaded && resolved.origin == Origin::Unassigned)
                ++unassigned_;
            identities_[resolved.identity].hexes.push_back(hex);
            cells_.emplace(hex, std::move(resolved));
        }
    }

    ModeKind kind() const override { return ModeKind::Religion; }
    std::wstring name() const override { return wide(title_); }
    Ink color(Hex hex, const Tile &) const override { return identities_[cell(hex).identity].color; }

    std::vector<ModeLabelSet> labels() const override
    {
        std::vector<ModeLabelSet> result;
        for (const auto &identity : identities_)
        {
            if (identity.hexes.empty())
                continue;
            const auto count = identity.hexes.size();
            result.push_back({wide(identity.name),
                              std::to_wstring(count) + (count == 1 ? L" hex - " : L" hexes - ") + wide(identity.description),
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
            const auto count = identity.hexes.size();
            result.push_back({wide(identity.name) + L" - " + std::to_wstring(count) + (count == 1 ? L" hex" : L" hexes"),
                              identity.color});
        }
        return result;
    }
    std::wstring status() const override
    {
        std::wostringstream out;
        if (state_ == LoadState::Missing)
            out << L"No religion sidecar - " << cells_.size() << L" neutral unknown";
        else if (state_ == LoadState::Error)
            out << L"Religion sidecar error - " << cells_.size() << L" neutral unknown - " << source_.filename().wstring();
        else
        {
            out << known_ << L" classified - " << explicitUnknown_ << L" explicitly unknown";
            if (unassigned_)
                out << L" - " << unassigned_ << L" unassigned/unknown";
            if (!contexts_.empty())
                out << L" - military context is inspector-only";
            out << L" - " << source_.filename().wstring();
        }
        return out.str();
    }
    std::wstring inspector(Hex hex, const Tile &tile) const override
    {
        const auto &entry = cell(hex);
        const auto &identity = identities_[entry.identity];
        std::wostringstream out;
        out << L"Unit " << wide(formatUnitId(makeUnitId(hex))) << L"  (q " << hex.q << L", r " << hex.r << L")";
        out << L"\nReligious identity: " << wide(identity.name);
        if (tile.region < map_.regions.size())
            out << L"\nAtlas region: " << wide(map_.regions[tile.region].name);
        out << L"\nPopulation scope: mainstream local-population affiliation, not the ruler's or an occupying army's faith";
        out << L"\nConfidence: " << wide(entry.confidence) << L" (geographic generalisation, not a percentage)";
        out << L"\nBasis: ";
        switch (entry.origin)
        {
        case Origin::Unit: out << L"canonical unit override in religion sidecar"; break;
        case Origin::Region: out << L"atlas-region assignment in religion sidecar"; break;
        case Origin::Unassigned: out << L"neutral fallback for an unassigned atlas region"; break;
        case Origin::Missing: out << L"neutral fallback; sidecar absent"; break;
        case Origin::Error: out << L"neutral fallback; sidecar could not be read or validated"; break;
        case Origin::Outside: out << L"neutral fallback outside the atlas"; break;
        }
        out << L"\nIdentity scope: " << wide(identity.description);
        out << L"\nAssignment note: " << wide(entry.note);
        if (state_ == LoadState::Error && !diagnostic_.empty())
            out << L"\nSidecar diagnostic: " << wide(diagnostic_);

        if (const auto found = contexts_.find(hex); found != contexts_.end())
        {
            for (const auto &context : found->second)
            {
                out << L"\n\nMilitary context (separate from the population layer): " << wide(context.group);
                out << L"\nArmy affiliation: " << wide(context.affiliation);
                out << L"\nMilitary caveat: " << wide(context.note);
            }
            out << L"\nThis context does not change the hex's majority-population colour or label.";
        }
        return out.str();
    }
};
} // namespace

ModePtr createReligionMode(const Map &map, const fs::path &data)
{
    return std::make_unique<ReligionMode>(map, data);
}
