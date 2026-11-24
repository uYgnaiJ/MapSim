#include "model.hpp"
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::wstring wide(const std::string &s)
{
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), nullptr, 0);
    if (!n)
        throw std::runtime_error("Invalid UTF-8 text");
    std::wstring out(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), out.data(), n);
    return out;
}
std::string utf8(const std::wstring &s)
{
    if (s.empty())
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}
UnitId makeUnitId(Hex h)
{
    int k = -std::min({h.q + h.r, 0, h.r});
    return {h.q + h.r + k, k, h.r + k};
}
Hex axial(UnitId id) { return {id.a - id.c, id.c - id.b}; }
std::string formatUnitId(UnitId id)
{
    std::ostringstream out;
    out << std::setfill('0') << std::setw(3) << id.a << '-' << std::setw(3) << id.b << '-' << std::setw(3) << id.c;
    return out.str();
}
static std::string trim(const std::string &s)
{
    auto first = s.find_first_not_of(" \t\r");
    return first == s.npos ? "" : s.substr(first, s.find_last_not_of(" \t\r") - first + 1);
}
static int number(const std::string &s, int base = 10)
{
    int result{};
    auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), result, base);
    if (error != std::errc{} || end != s.data() + s.size())
        throw std::runtime_error("Invalid number: " + s);
    return result;
}
static double real(const std::string &s)
{
    size_t used{};
    double result{};
    try
    {
        result = std::stod(s, &used);
    }
    catch (...)
    {
        throw std::runtime_error("Invalid number: " + s);
    }
    if (used != s.size() || !std::isfinite(result))
        throw std::runtime_error("Invalid number: " + s);
    return result;
}
static std::vector<std::string> columns(const std::string &line)
{
    std::vector<std::string> cols;
    size_t start = 0;
    for (;;)
    {
        size_t end = line.find('|', start);
        cols.push_back(trim(line.substr(start, end - start)));
        if (end == line.npos)
            break;
        start = end + 1;
    }
    return cols;
}
Map loadMap(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open map file");
    Map m;
    std::map<std::string, size_t> ids, kingdoms;
    enum class Section { Header, Name, Kingdoms, Regions, Hexes };
    int lineNo = 0, version = 0;
    auto section = Section::Header;
    bool grouped = false;
    try
    {
        for (std::string line; std::getline(in, line);)
        {
            ++lineNo;
            if (lineNo == 1 && line.starts_with("\xEF\xBB\xBF"))
                line.erase(0, 3);
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;
            wide(line);
            if (section == Section::Header && (line == "FIELDATLAS 1" || line == "FIELDATLAS 2" || line == "FIELDATLAS 3"))
            {
                version = line.back() - '0';
                section = Section::Name;
                grouped = version == 2;
                continue;
            }
            if (section == Section::Name && line.starts_with("name = ") && m.name.empty())
            {
                m.name = trim(line.substr(7));
                continue;
            }
            if (section == Section::Name && version >= 2 && line == "[kingdoms]" && !m.name.empty() &&
                (version == 2 || m.grid))
            {
                grouped = true;
                section = Section::Kingdoms;
                continue;
            }
            if (line == "[regions]" && ((section == Section::Name && !grouped && !m.name.empty() && (version < 3 || m.grid)) ||
                                        (section == Section::Kingdoms && !kingdoms.empty() && (version < 3 || m.grid))))
            {
                section = Section::Regions;
                continue;
            }
            if (section == Section::Regions && line == "[hexes]" && !ids.empty())
            {
                section = Section::Hexes;
                continue;
            }
            auto cols = columns(line);
            if (section == Section::Name && version == 3 && !m.name.empty() && cols.size() == 4 &&
                cols[0].starts_with("grid = "))
            {
                if (m.grid)
                    throw std::runtime_error("Duplicate projected grid");
                auto crs = trim(cols[0].substr(7));
                double easting = real(cols[1]), northing = real(cols[2]), radius = real(cols[3]);
                if (crs.empty() || crs.find('|') != crs.npos || radius <= 0)
                    throw std::runtime_error("Invalid projected grid");
                m.grid = ProjectedGrid{crs, easting, northing, radius};
                continue;
            }
            if (section == Section::Kingdoms && cols.size() == 3)
            {
                if (cols[0].empty() || cols[1].empty() || cols[2].empty() ||
                    !kingdoms.emplace(cols[0], m.kingdoms.size()).second)
                    throw std::runtime_error("Invalid or duplicate kingdom");
                m.kingdoms.push_back({cols[0], cols[1], cols[2]});
            }
            else if (section == Section::Regions && cols.size() == (grouped ? 4u : 3u))
            {
                auto &id = cols[0];
                auto &color = cols[2];
                if (id.empty() || cols[1].empty() || !ids.emplace(id, m.regions.size()).second ||
                    color.size() != 7 || color[0] != '#' || color.find_first_not_of("0123456789abcdefABCDEF", 1) != color.npos)
                    throw std::runtime_error("Invalid or duplicate region");
                std::optional<size_t> kingdom;
                if (grouped)
                {
                    auto found = kingdoms.find(cols[3]);
                    if (found == kingdoms.end())
                        throw std::runtime_error("Unknown kingdom: " + cols[3]);
                    kingdom = found->second;
                }
                m.regions.push_back({id, cols[1], uint32_t(number(color.substr(1), 16)), kingdom});
            }
            else if (section == Section::Hexes && cols.size() == (version == 3 ? 5u : 4u))
            {
                bool projected = version == 3;
                Hex h;
                if (projected)
                {
                    UnitId id{number(cols[0]), number(cols[1]), number(cols[2])};
                    if (id.a < 0 || id.b < 0 || id.c < 0 || (id.a != 0 && id.b != 0 && id.c != 0))
                        throw std::runtime_error("Invalid non-canonical unit ID");
                    h = axial(id);
                }
                else
                    h = {number(cols[0]), number(cols[1])};
                if (h.q < -100000 || h.q > 100000 || h.r < -100000 || h.r > 100000)
                    throw std::runtime_error("Coordinate outside supported range");
                auto region = ids.find(cols[projected ? 3 : 2]);
                auto terrain = std::find(terrainNames.begin(), terrainNames.end(), cols[projected ? 4 : 3]);
                if (region == ids.end() || terrain == terrainNames.end())
                    throw std::runtime_error("Unknown region or terrain");
                if (!m.tiles.emplace(h, Tile{region->second, Terrain(terrain - terrainNames.begin())}).second)
                    throw std::runtime_error("Duplicate hex coordinate");
            }
            else
                throw std::runtime_error("Unexpected row or section");
        }
        if (in.bad() || section != Section::Hexes || (version == 3 && !m.grid))
            throw std::runtime_error("Map is incomplete");
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Line " + std::to_string(lineNo) + ": " + e.what());
    }
    return m;
}
void saveMap(const Map &m, const std::filesystem::path &path)
{
    if (m.grid && (m.grid->crs.empty() || m.grid->crs.find('|') != m.grid->crs.npos ||
                   !std::isfinite(m.grid->easting) || !std::isfinite(m.grid->northing) ||
                   !std::isfinite(m.grid->radius) || m.grid->radius <= 0))
        throw std::runtime_error("Invalid projected grid");
    std::wostringstream suffix;
    suffix << L"." << GetCurrentProcessId() << L".tmp";
    auto temp = path;
    temp += suffix.str();
    try
    {
        std::ofstream out(temp, std::ios::binary);
        out.exceptions(std::ios::badbit | std::ios::failbit);
        bool grouped = !m.kingdoms.empty();
        int version = m.grid ? 3 : grouped ? 2 : 1;
        out << "FIELDATLAS " << version << "\nname = " << m.name << "\n";
        if (m.grid)
            out << "grid = " << m.grid->crs << " | " << std::setprecision(12) << m.grid->easting << " | "
                << m.grid->northing << " | " << m.grid->radius << "\n";
        if (grouped)
        {
            out << "\n[kingdoms]\n# id | name | ruler\n";
            for (auto &k : m.kingdoms)
                out << k.id << " | " << k.name << " | " << k.ruler << "\n";
        }
        out << "\n[regions]\n# id | name | color" << (grouped ? " | kingdom" : "") << "\n";
        for (auto &r : m.regions)
        {
            out << r.id << " | " << r.name << " | #" << std::uppercase << std::hex << std::setw(6) << std::setfill('0') << r.color << std::dec;
            if (grouped)
                out << " | " << m.kingdoms.at(r.kingdom.value()).id;
            out << "\n";
        }
        out << "\n[hexes]\n";
        if (m.grid)
        {
            out << "# a | b | c | region | terrain; displayed ID = aaa-bbb-ccc\n";
            for (auto &[h, t] : m.tiles)
            {
                auto id = makeUnitId(h);
                out << id.a << " | " << id.b << " | " << id.c << " | " << m.regions[t.region].id << " | "
                    << terrainNames[int(t.terrain)] << "\n";
            }
        }
        else
        {
            out << "# q | r | region | terrain\n";
            for (auto &[h, t] : m.tiles)
                out << h.q << " | " << h.r << " | " << m.regions[t.region].id << " | " << terrainNames[int(t.terrain)] << "\n";
        }
        out.close();
        if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace destination file");
    }
    catch (...)
    {
        std::error_code ec;
        std::filesystem::remove(temp, ec);
        throw;
    }
}
