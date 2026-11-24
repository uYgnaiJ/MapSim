#include "map_mode.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace
{
constexpr Ink unknownInk = 0xd9d8d2;
constexpr std::array<Ink, 6> populationStops{
    0xf6f0e2, 0xe4d7b4, 0xc3b184, 0x987f59, 0x644d42, 0x342d35};

struct PopulationCell
{
    uint32_t total{}, rural{}, settlement{};
};

std::string trim(const std::string &value)
{
    auto first = value.find_first_not_of(" \t\r");
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
        size_t end = line.find('|', start);
        result.push_back(trim(line.substr(start, end - start)));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return result;
}

template <typename T>
T integer(const std::string &text)
{
    if (text.empty())
        throw std::runtime_error("empty integer");
    T value{};
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::runtime_error("invalid integer: " + text);
    return value;
}

std::wstring number(uint64_t value)
{
    std::string digits = std::to_string(value);
    for (ptrdiff_t i = ptrdiff_t(digits.size()) - 3; i > 0; i -= 3)
        digits.insert(size_t(i), 1, ',');
    return std::wstring(digits.begin(), digits.end());
}

Ink ramp(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    double position = t * double(populationStops.size() - 1);
    size_t index = std::min(size_t(position), populationStops.size() - 2);
    double fraction = position - double(index);
    Ink a = populationStops[index], b = populationStops[index + 1];
    Ink result = 0;
    for (int shift = 16; shift >= 0; shift -= 8)
    {
        double av = double((a >> shift) & 0xff), bv = double((b >> shift) & 0xff);
        result |= Ink(std::lround(av + (bv - av) * fraction)) << shift;
    }
    return result;
}

fs::path sidecarPath(const Map &map, const fs::path &data)
{
    if (data.empty())
        return data;
    if (data.extension() == ".atlas")
    {
        fs::path result = data;
        result.replace_extension(".population");
        return result;
    }
    std::error_code ec;
    if (fs::is_directory(data, ec) && !ec)
    {
        if (map.name == "England in 867")
            return data / "england-867.population";
        return data / "population.population";
    }
    return data;
}

class PopulationMode final : public ModeView
{
public:
    PopulationMode(const Map &map, const fs::path &data) : map_(map), path_(sidecarPath(map, data))
    {
        refresh();
    }

    ModeKind kind() const override { return ModeKind::Population; }
    std::wstring name() const override
    {
        return year_ ? L"Population (estimated " + std::to_wstring(year_) + L")" : L"Population (estimated)";
    }

    Ink color(Hex hex, const Tile &) const override
    {
        auto found = cells_.find(hex);
        if (found == cells_.end())
            return unknownInk;
        return shade(found->second.total);
    }

    std::vector<ModeLabelSet> labels() const override { return {}; }

    std::vector<ModeLegendRow> legend() const override
    {
        if (!known_)
        {
            return {
                {L"Estimated population: unavailable", unknownInk},
                {L"Unknown (0); missing or invalid sidecar", unknownInk},
                {L"Low-confidence model; not a census", unknownInk}};
        }
        std::vector<ModeLegendRow> rows;
        rows.push_back({L"Estimated people/hex " + yearText() + L": " + number(minimum_) + L"–" + number(maximum_) +
                            L" · log colour",
                        shade(minimum_)});
        rows.push_back({L"0 (colour baseline)", shade(0)});
        for (auto [label, value] : {std::pair{L"25%", quantile(.25)}, std::pair{L"median", quantile(.50)},
                                    std::pair{L"90%", quantile(.90)}})
            rows.push_back({std::wstring(label) + L": " + number(value), shade(value)});
        rows.push_back({L"maximum: " + number(maximum_), shade(maximum_)});
        rows.push_back({L"Total est. " + number(total_) + L" · plausible " + number(low_) + L"–" + number(high_),
                        shade(maximum_)});
        rows.push_back({L"Low-confidence heuristic; not a census", shade(minimum_)});
        return rows;
    }

    std::wstring status() const override
    {
        if (!known_)
            return L"Population unknown (0) · 0/" + number(map_.tiles.size()) + L" hexes · " + issue_;
        std::wstring result = L"Estimated " + yearText() + L" · total " + number(total_) + L" · " +
                              number(known_) + L"/" + number(map_.tiles.size()) + L" hexes · low confidence";
        if (known_ != map_.tiles.size())
            result += L" · missing hexes shown grey";
        return result;
    }

    std::wstring inspector(Hex hex, const Tile &) const override
    {
        auto found = cells_.find(hex);
        if (found == cells_.end())
            return L"Estimated population: unknown (0)";
        return L"Estimated population: " + number(found->second.total) + L" (" + yearText() + L" est.)";
    }

    void refresh() override
    {
        cells_.clear();
        issue_.clear();
        loaded_ = false;
        year_ = 0;
        target_ = low_ = high_ = 0;
        try
        {
            load();
            loaded_ = true;
        }
        catch (const std::exception &error)
        {
            cells_.clear();
            try
            {
                issue_ = wide(error.what());
            }
            catch (...)
            {
                issue_ = L"Invalid population sidecar encoding; all hexes are unknown (0)";
            }
        }
        catch (...)
        {
            cells_.clear();
            issue_ = L"Unknown population sidecar error";
        }
        if (!loaded_ && issue_.empty())
            issue_ = L"No population sidecar data";
        summarise();
    }

private:
    const Map &map_;
    fs::path path_;
    std::map<Hex, PopulationCell> cells_;
    std::vector<uint32_t> sorted_;
    std::wstring issue_;
    uint64_t total_{}, target_{}, low_{}, high_{};
    uint32_t minimum_{}, maximum_{}, anchor_{1};
    size_t known_{};
    int year_{};
    bool loaded_{};

    std::wstring yearText() const { return year_ ? std::to_wstring(year_) : L"(year unspecified)"; }

    void load()
    {
        std::error_code ec;
        if (path_.empty() || !fs::exists(path_, ec) || ec)
            throw std::runtime_error("Missing population sidecar; all hexes are unknown (0)");
        std::ifstream input(path_, std::ios::binary);
        if (!input)
            throw std::runtime_error("Unreadable population sidecar; all hexes are unknown (0)");
        enum class Section { Header, Terrain, Regions, Settlements, Hexes } section = Section::Header;
        bool header = false, sawHexes = false, declaredHexes = false;
        uint64_t declaredCount = 0;
        std::string expectedMap;
        int lineNumber = 0;
        for (std::string line; std::getline(input, line);)
        {
            ++lineNumber;
            if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF"))
                line.erase(0, 3);
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;
            try
            {
                if (!header)
                {
                    if (line != "FIELDPOP 1")
                        throw std::runtime_error("expected FIELDPOP 1");
                    header = true;
                    continue;
                }
                if (line == "[terrain]")
                {
                    section = Section::Terrain;
                    continue;
                }
                if (line == "[regions]")
                {
                    section = Section::Regions;
                    continue;
                }
                if (line == "[settlements]")
                {
                    section = Section::Settlements;
                    continue;
                }
                if (line == "[hexes]")
                {
                    section = Section::Hexes;
                    sawHexes = true;
                    continue;
                }
                if (section != Section::Hexes)
                {
                    if (section != Section::Header)
                        continue; // Model parameters are documentary, not needed for display.
                    size_t equals = line.find('=');
                    if (equals == std::string::npos)
                        throw std::runtime_error("expected metadata key = value");
                    std::string key = trim(line.substr(0, equals));
                    std::string value = trim(line.substr(equals + 1));
                    if (key == "map")
                        expectedMap = value;
                    else if (key == "year")
                        year_ = integer<int>(value);
                    else if (key == "target-total")
                        target_ = integer<uint64_t>(value);
                    else if (key == "plausible-low")
                        low_ = integer<uint64_t>(value);
                    else if (key == "plausible-high")
                        high_ = integer<uint64_t>(value);
                    else if (key == "hex-count")
                    {
                        declaredCount = integer<uint64_t>(value);
                        declaredHexes = true;
                    }
                    continue;
                }
                auto fields = columns(line);
                if (fields.size() != 5)
                    throw std::runtime_error("hex row needs q | r | total | rural | settlement");
                Hex hex{integer<int>(fields[0]), integer<int>(fields[1])};
                auto total = integer<uint32_t>(fields[2]);
                auto rural = integer<uint32_t>(fields[3]);
                auto settlement = integer<uint32_t>(fields[4]);
                if (uint64_t(rural) + settlement != total)
                    throw std::runtime_error("hex total differs from rural + settlement");
                if (!map_.tiles.contains(hex))
                    throw std::runtime_error("sidecar contains a coordinate absent from the map");
                if (!cells_.emplace(hex, PopulationCell{total, rural, settlement}).second)
                    throw std::runtime_error("duplicate sidecar coordinate");
            }
            catch (const std::exception &error)
            {
                throw std::runtime_error("Population sidecar line " + std::to_string(lineNumber) + ": " + error.what());
            }
        }
        if (input.bad())
            throw std::runtime_error("Error while reading population sidecar");
        if (!header || !sawHexes)
            throw std::runtime_error("Incomplete population sidecar; all hexes are unknown (0)");
        if (!expectedMap.empty() && expectedMap != map_.name)
            throw std::runtime_error("Population sidecar belongs to a different map");
        if (declaredHexes && declaredCount != map_.tiles.size())
            throw std::runtime_error("Population sidecar hex-count differs from the map");
        if (cells_.size() != map_.tiles.size())
            throw std::runtime_error("Population sidecar does not cover every map hex");
        uint64_t sum = 0;
        for (const auto &[hex, cell] : cells_)
            sum += cell.total;
        if (target_ != sum)
            throw std::runtime_error("Population sidecar target-total differs from its hex sum");
        if (!low_ && !high_)
            low_ = high_ = target_;
        if (low_ > high_)
            throw std::runtime_error("Invalid population uncertainty range");
        if (year_ < 0 || year_ > 9999)
            throw std::runtime_error("Invalid population estimate year");
    }

    void summarise()
    {
        sorted_.clear();
        total_ = 0;
        known_ = 0;
        minimum_ = 0;
        maximum_ = 0;
        anchor_ = 1;
        if (!loaded_)
            return;
        for (const auto &[hex, tile] : map_.tiles)
        {
            auto found = cells_.find(hex);
            if (found == cells_.end())
                continue;
            ++known_;
            total_ += found->second.total;
            sorted_.push_back(found->second.total);
        }
        if (sorted_.empty())
            return;
        std::sort(sorted_.begin(), sorted_.end());
        minimum_ = sorted_.front();
        maximum_ = sorted_.back();
        std::vector<uint32_t> positive;
        positive.reserve(sorted_.size());
        for (uint32_t value : sorted_)
            if (value)
                positive.push_back(value);
        anchor_ = positive.empty() ? 1 : std::max<uint32_t>(1, positive[(positive.size() - 1) / 4]);
    }

    uint32_t quantile(double fraction) const
    {
        if (sorted_.empty())
            return 0;
        return sorted_[size_t(std::clamp(fraction, 0.0, 1.0) * double(sorted_.size() - 1))];
    }

    Ink shade(uint32_t value) const
    {
        if (!value)
            return populationStops.front();
        if (!maximum_)
            return populationStops.front();
        if (maximum_ <= anchor_)
            return populationStops.back();
        double scale = std::log1p(double(maximum_) / anchor_);
        if (!(scale > 0) || !std::isfinite(scale))
            return populationStops.front();
        double t = std::log1p(double(value) / anchor_) / scale;
        return ramp(t);
    }
};
} // namespace

ModePtr createPopulationMode(const Map &map, const fs::path &data)
{
    return std::make_unique<PopulationMode>(map, data);
}
