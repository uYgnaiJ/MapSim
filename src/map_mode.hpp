#pragma once
#include "model.hpp"
#include "canvas.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

enum class ModeKind { Political, Natural, Religion, Culture, Population };

struct ModeLabelSet
{
    std::wstring name, detail;
    Ink color{};
    std::vector<Hex> hexes;
};
struct ModeLegendRow
{
    std::wstring text;
    Ink color{};
};

class ModeView
{
public:
    virtual ~ModeView() = default;
    virtual ModeKind kind() const = 0;
    virtual std::wstring name() const = 0;
    virtual Ink color(Hex hex, const Tile& tile) const = 0;
    virtual std::vector<ModeLabelSet> labels() const = 0;
    virtual std::vector<ModeLegendRow> legend() const = 0;
    virtual std::wstring status() const = 0;
    virtual std::wstring inspector(Hex, const Tile&) const { return {}; }
    virtual std::vector<Hex> activeHexes() const { return {}; }
    virtual bool descend(Hex) { return false; }
    virtual void reset() {}
    virtual void refresh() {}
};

using ModePtr = std::unique_ptr<ModeView>;
ModePtr createPoliticalMode(const Map& map, const std::filesystem::path& data);
ModePtr createNaturalMode(const Map& map, const std::filesystem::path& data);
ModePtr createReligionMode(const Map& map, const std::filesystem::path& data);
ModePtr createCultureMode(const Map& map, const std::filesystem::path& data);
ModePtr createPopulationMode(const Map& map, const std::filesystem::path& data);
