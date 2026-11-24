# Natural terrain mode

`src/natural.cpp` implements `createNaturalMode(const Map&, const path&)`. It is a terrain view, not a recoloured political map: region and kingdom membership never affect a tile colour, and the mode never synthesises labels from `Map::regions` or `Map::kingdoms`.

The source `.atlas` remains authoritative and unchanged. An optional `.natural` sidecar supplies only natural-place labels; it does **not** override terrain, add elevation, or supply population or agricultural data.

## Palette and symbols

The mode returns these RGB base colours for the five existing `Terrain` values:

| Terrain | RGB | Intended reading | Existing `landSymbol()` vocabulary |
| --- | --- | --- | --- |
| Plains | `#DDE6B2` | Pale, open low-relief ground | Small grass strokes on a subset of cells |
| Forest | `#79A376` | Clearly darker woodland green | Two or three conifer-like tree marks |
| Hills | `#C8AE78` | Muted ochre, intermediate relief | Two rounded hill contours |
| Mountains | `#AAA7A3` | Neutral cool rock grey | Two angular peaks |
| Marsh | `#82B9AE` | Blue-green wet lowland | Horizontal water strokes and reeds |

The colours are deliberately separated in both hue and lightness; the existing symbols provide a second, non-colour cue. This is a **categorical palette**, not a hypsometric elevation ramp. In particular, the progression Plains → Hills → Mountains is only a broad terrain classification, not three measured height bands.

`legend()` explains all five classes. `inspector()` reports the tile's terrain as schematic and gives no elevation or yield value. `status()` reports the occupied-hex count and the schematic/no-measured-elevation limitation. Population is not inferred anywhere in the mode.

## Behaviour without a sidecar

Any map works from `Tile.terrain` alone. If the supplied sidecar path does not exist, or if an empty path is supplied:

- all five colours, the legend, status, and inspector continue to work;
- `labels()` returns an empty vector;
- no political or invented geographic labels are shown;
- empty maps and stale/empty label memberships are safe.

For caller convenience, `data` may be either `path/to/map.natural` or the companion `path/to/map.atlas`; the latter is converted to the `.natural` path. A present but malformed sidecar is an error with a source line number rather than silently invented output.

## Optional `NATURALATLAS 1` sidecar

The format is intentionally small and independent of the core atlas parser:

```text
NATURALATLAS 1
# Full-line comments and blank lines are accepted.
[labels]
# id | display name | detail | #RRGGBB
example-ridge | Example Ridge | Upland; approximate extent | #665744

[hexes]
# label id | a | b | c
example-ridge | 80 | 0 | 65
```

Rules:

- Label IDs are unique. Names and details are UTF-8; label colours are six-digit RGB.
- `a | b | c` uses the same canonical, nonnegative three-axis unit ID as `FIELDATLAS 3`; at least one component must be zero.
- A hex can occur only once in the sidecar, and each row must reference a declared label ID. Derived axial coordinates must remain within the model's supported range.
- Sidecar coordinates absent from the current `Map::tiles` are ignored, allowing a coastline to be edited without out-of-bounds access. Labels with no occupied hexes are omitted. Filtering uses the live map on every `labels()` call, so deleted hexes never reach label layout and newly restored sidecar hexes reappear without rereading the file.
- The sidecar contributes only `ModeLabelSet` records. It cannot change terrain or create political labels.

`maps/england-867.natural` currently defines four labels and 727 non-overlapping occupied-hex memberships:

| Label | Hexes | Sidecar description |
| --- | ---: | --- |
| Pennines | 433 | Upland landscape; approximate extent |
| Lake District | 101 | Fells and lakes; approximate extent |
| Dartmoor | 39 | Moorland; approximate extent |
| The Fens | 154 | Fenland; approximate extent |

These are modern, widely recognisable geographic names used for orientation. They are not claims about ninth-century political units, the period usage of each modern name, protected-area boundaries, or precise physical-geographic extents.

### How the label hexes were produced

`build/natural_generate.py` reads the map's declared EPSG:27700 grid, transforms each occupied hex centre to WGS84 longitude/latitude with `pyproj`, and intersects it with small hand-authored, deliberately coarse polygons in that script. It writes canonical unit IDs back to the sidecar. Those polygons are editorial **label-localisation aids**, not an imported boundary dataset. They are never used to recolour a tile.

No National Park, National Character Area, wetland, or historic boundary is represented as exact. The labels can span multiple terrain classes by design. The generator audits this rather than hiding it:

| Coarse label area | Plains | Forest | Hills | Mountains | Marsh |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pennines | 104 | 0 | 291 | 38 | 0 |
| Lake District | 0 | 0 | 14 | 87 | 0 |
| Dartmoor | 38 | 0 | 1 | 0 | 0 |
| The Fens | 90 | 0 | 0 | 0 | 64 |

## What the current terrain data can support

The model has only the enum `Plains`, `Forest`, `Hills`, `Mountains`, `Marsh`. A `Tile` has no elevation, slope, soil, drainage, land-cover date, or confidence field. The 4,987 occupied England hexes currently contain:

| Plains | Forest | Hills | Mountains | Marsh |
| ---: | ---: | ---: | ---: | ---: |
| 3,264 | 414 | 1,080 | 141 | 88 |

The available generation history is important:

1. `build/project_england27700.py::inherit_terrain()` copies the terrain of the nearest old hex centre in projected space when creating the v3 grid.
2. `build/redraw_england867.py` preserves the old terrain field while reconstructing coast and regions; it does not derive that field from elevation or land-cover measurements.
3. No DEM, soil dataset, ninth-century land-cover reconstruction, or terrain uncertainty model is present in the supplied map-generation inputs. The exact original editorial classification process is therefore not something this mode can certify.

The result is useful as a **schematic terrain-class view**, but not as a precise physical map or a measured reconstruction of 867. The Dartmoor audit is a concrete warning: 38 of its 39 coarse label-area hexes are `Plains`, and none is `Mountains`. This does not justify silently rewriting the atlas. Similarly, the mixture of `Marsh` and `Plains` around The Fens cannot by itself reconstruct historic wetland, later drainage, or present-day land cover.

Other limitations:

- A 3.3 km-radius hex generalises real landscapes and can mix coast, valley, and upland within one class.
- `Forest` is a symbol/category in this dataset, not proof of continuous or ancient woodland.
- `Marsh` does not encode seasonal flooding, permanent inundation, navigability, or drainage history.
- `Mountains` is not an altitude measurement, and its absence does not prove flatness.
- Political borders, coastlines, and natural-label areas remain approximate. The sidecar does not improve the underlying coastline or terrain evidence.

A future evidence-based physical map would need an explicit terrain model and documented classification rules. For example, [OS Terrain 50](https://www.ordnancesurvey.co.uk/products/os-terrain-50) describes a real 50 m-post-spacing DTM and 10 m contours. **OS Terrain 50 was not downloaded or used here**, and this document does not imply OS elevation values for any hex.

## Agriculture, carrying capacity, population, and strategy

Agrarian production was fundamental to ninth-century kingdoms, and comparatively productive agricultural lowlands are relevant background. That does **not** make the five display colours a land-capability model.

- `Plains` means low-relief ground in this schema, not automatically fertile soil, arable land, open-field cultivation, high yield, or dense settlement.
- Hills, mountains, woods, and wetlands were not necessarily unused or uninhabited. Their economic use depends on evidence about pasture, woodland management, drainage, soils, climate, technology, and tenure that these tiles do not contain.
- Mature open-field systems and expanded cereal cultivation should not be projected wholesale onto 867. Conversely, a `Plains` hex must not be read as proof that any particular field system existed there.
- No hex count or terrain-class count is converted into calories, agricultural surplus, army supply, population, or “carrying capacity.” Such a calculation would be pseudo-precision.
- There is no 867 census in this data. Even later Domesday-based estimates require contested multipliers, and back-projection over two centuries is model-dependent. The natural mode therefore shows no kingdom totals or relative population ranking.
- Terrain classes alone do not establish military defensibility or movement speed. Rivers, coasts, roads, crossings, and political/logistical context matter and are not encoded by this mode.

The user-supplied `deep-research-report.md` research notes (lines 109–135 and 151–167) were used only for these qualitative cautions about agrarian economies, strategic geography, and demographic uncertainty. Their unexpanded citation placeholders are **not** treated as verified sources or reproduced as URLs, and the report supplies no elevation data.

## References and source boundaries

The following official pages were consulted on 2026-11-24 for broad landscape character and for distinguishing real elevation data from this schematic view. They did not provide the generator's coarse polygons:

- [Lake District National Park Authority — Landscape and heritage](https://lakedistrict.gov.uk/protect/landscape-and-heritage/): rugged fells, dramatic valleys, and a cultural landscape shaped by natural forces and human activity.
- [Dartmoor National Park Authority — Welcome to the Dartmoor Story](https://www.dartmoor.gov.uk/wildlife-and-heritage/dartmoor-story/dartmoor-story-pages/welcome): granite, open moor, mires, wooded valleys, and human shaping. This does not validate the atlas's per-hex classification.
- [North Pennines National Landscape — What's special?](https://northpennines.org.uk/whats-special/): upland moors, dales, rivers, woods, peatland, and mixed land use. It describes only the North Pennines, not a boundary for the sidecar's broader Pennines label.
- [Great Fen — home](https://www.greatfen.org.uk/) and [Map of the Great Fen](https://www.greatfen.org.uk/about-great-fen/map-great-fen): a fenland landscape between Peterborough and Huntingdon and modern restoration. The Great Fen is not equated with the whole of The Fens, and current restoration is not projected unchanged onto 867.
- [GOV.UK — National Character Area profiles](https://www.gov.uk/government/publications/national-character-area-profiles-data-for-local-decision-making/national-character-area-profiles): NCAs combine landscape, biodiversity, geodiversity, history, and human activity, and their boundaries follow natural lines rather than district boundaries. No NCA boundary was imported.
- [Ordnance Survey — OS Terrain 50](https://www.ordnancesurvey.co.uk/products/os-terrain-50): reference for what an actual DTM provides; not an input to this delivery.

The existing map's EPSG:27700 positioning and its ONS/Ordnance Survey geographic-reference notices remain those of `england-867.atlas`; the natural mode adds no new boundary or elevation attribution.

## Independent generation and checks

From the repository root:

```powershell
# Compile and run the standalone C++ tests, then verify deterministic sidecar data.
powershell -ExecutionPolicy Bypass -File .\build\natural_check.ps1

# Regenerate the sidecar/report and a visual preview (uses the existing Python environment).
& .\build\proj-venv\Scripts\python.exe .\build\natural_generate.py --preview

# Read-only reproducibility check; does not rewrite the sidecar.
& .\build\proj-venv\Scripts\python.exe .\build\natural_generate.py --check
```

Generated inspection artefacts are `build/natural_generation.txt`, `build/natural_preview.png`, `build/natural_test.cpp`, and `build/natural_test.exe`. The C++ test covers exact and unique colours for all five classes, legend and inspector output, every England hex, generic maps without sidecars, an empty map, missing and malformed sidecars, companion-path handling, stale-coordinate and live-map filtering, all 727 label memberships, supported coordinate bounds, and the absence of region/kingdom/ruler text in natural labels and inspectors.
