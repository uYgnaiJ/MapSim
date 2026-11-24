# Field Atlas

Field Atlas is a native Win32 map editor written in C++20. Vulkan draws the hexes, terrain symbols, labels, and editor interface. The included Avelin Coast map contains 258 hexes grouped into five regions.

## Run

Open `build/FieldAtlas.exe` to explore the sample. To start a map, open `maps/starter.atlas`, then use **Ctrl+Shift+S** to save a copy under a new name. You can also pass a map path to the executable.

## England in 867

Open [england-867.atlas](maps/england-867.atlas) for a reconstruction immediately after the Battle of York on 21 March 867 and before the army's move to Nottingham. It covers roughly the footprint of present-day England, including Cornwall and the Isle of Wight, with 4,987 hexes in 25 historical-geographic regions. The six political groupings are Northumbria under the client king Ecgberht I, Danish-held York under the Great Heathen Army, Mercia under Burgred, East Anglia under Edmund, Wessex under Æthelred I, and Cornwall under a local ruler whose name is not securely known for 867.

The map uses `FIELDATLAS 3` on the EPSG:27700 British National Grid. Its permanent hex-grid origin is `(E=100000, N=700000)` and its hex circumradius is 3,300 metres. Every unit has a stable, nonnegative three-axis ID stored in the file and shown in the inspector; adding or removing edge hexes does not renumber the others.

Regions are not independent kingdoms. Kent, Sussex, Surrey, and Essex belong to the West Saxon political sphere; Lindsey, Hwicce, Magonsæte, and Wreocensæte belong to Mercia. Bernicia and Deira are older regional names within the former Northumbrian kingdom. North Folk/South Folk, Western Northumbria, and Southern Mercia are explicitly cartographic conveniences rather than claims about exact ninth-century administrative units.

The York/Deira shading represents Danish military dominance after the battle, not a precisely bounded kingdom or the later Danelaw; systematic Scandinavian land division in Northumbria belongs to 876. Ecgberht's authority and its extent come from later northern accounts, often described as north of the Tyne. Western Northumbria, the Fens, the upper Thames frontier, and the Cornish border are particularly uncertain. Dungarth is documented as a Cornish ruler around 875, so the map no longer presents him as a securely attested ruler in 867.

Political borders, coastlines, and terrain remain approximate. Modern districts were used only as geographic positioning aids, not as evidence that modern county boundaries existed in 867. Individual region labels appear as you zoom in.

Historical references: [Anglo-Saxon Chronicle, ninth-century entries](https://avalon.law.yale.edu/medieval/ang09.asp) · [Victoria County History: York before the Norman Conquest](https://www.british-history.ac.uk/vch/yorks/city-of-york/pp2-24) · [Kershaw et al. on Viking-age Northumbria](https://ora.ox.ac.uk/objects/uuid:e36b9387-f7ca-4c24-a26a-e28e2a08a84a) · [Bassett on Hwicce and Magonsæte](https://www.cambridge.org/core/journals/anglo-saxon-england/article/hidation-of-the-hwicce-investigating-its-halving-between-the-eighth-century-and-1086/717DA3169156F7627BC2B8A4F86541F7) · [Electronic Sawyer, S 206](https://esawyer.lib.cam.ac.uk/charter/206.html) · [British Library on county names](https://www.bl.uk/stories/blogs/posts/the-anglo-saxon-origins-of-the-english-counties) · [Annales Cambriae](https://sourcebooks.web.fordham.edu/source/annalescambriae.asp).

Geographic reference: [UK-GeoJSON](https://github.com/martinjc/UK-GeoJSON), containing Ordnance Survey and Office for National Statistics data © Crown copyright and database right 2016, under the [Open Government Licence](https://www.nationalarchives.gov.uk/doc/open-government-licence/version/3/) and OS OpenData Licence.

## Rebuild

To rebuild on 64-bit Windows 10 or 11, install the Vulkan SDK and a MinGW-w64 C++20 toolchain with POSIX threads. The script uses Clang with LLD when available and otherwise uses GCC. Set `VULKAN_SDK` and put the compiler on your PATH.

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Run
```

The shaders are compiled into the executable. Running the built app requires a Vulkan-capable graphics driver. Keep the `maps` folder beside the executable or one directory above it for automatic sample loading.

## Build a map

- **Select (V)** clicks a hex to display its axial coordinates, region, and terrain. Click either value in the inspector to change it. **Delete** removes the selected hex.
- **Brush (B)** paints the active region and terrain. Choose a region in the index and a terrain in Brush settings. Drag over empty space to add hexes, or over existing hexes to repaint them. Each drag is one undoable edit.
- **Pan (H)** drags the camera. Middle-drag and Space-drag also pan. Use the wheel for cursor-centered zoom, **+ / −** to zoom at the center, and **F** to fit all units.
- **G** toggles the grid. **L** toggles map labels. Scroll the region index to reach additional regions.
- **Ctrl+Z / Ctrl+Y** undo and redo. **Ctrl+O** opens a file, **Ctrl+S** saves, and **Ctrl+Shift+S** saves a copy. Opening another map or closing the app prompts for unsaved changes.

## Editable map files

Each UTF-8 `.atlas` file holds one complete map. You can create and rename regions, change colors, or add units in any text editor. Reopen the file to load those edits. Save pending in-app changes first.

```text
FIELDATLAS 1
name = River Valley

[regions]
# stable ID | display name | RGB color
valley | River Valley | #91AF8C
ridge  | West Ridge   | #A5AFAD

[hexes]
# q | r | region ID | terrain
0 | 0 | valley | Forest
1 | 0 | valley | Plains
0 | 1 | ridge  | Mountains
```

In `FIELDATLAS 1` and `FIELDATLAS 2`, a hex is identified by its integer axial coordinates `(q, r)`. The six neighbors are `(q+1,r)`, `(q+1,r-1)`, `(q,r-1)`, `(q-1,r)`, `(q-1,r+1)`, and `(q,r+1)`. Unlisted coordinates are empty and can be painted. Each occupied coordinate belongs to exactly one region. Region membership and counts are derived from the hex rows. A region can be empty or span several islands.

For grouped maps, use `FIELDATLAS 2` and add a `[kingdoms]` section between the map name and `[regions]`. Each kingdom row uses `id | name | ruler`. Add its ID as the fourth column of every region row. Hex rows use the same format in both versions.

### Projected maps and unit IDs

`FIELDATLAS 3` adds one projected-grid row immediately after the map name. Its fields are `CRS | origin easting | origin northing | hex circumradius`; the three numeric grid values are in metres. For example:

```text
FIELDATLAS 3
name = England in 867
grid = EPSG:27700 | 100000 | 700000 | 3300

[hexes]
# a | b | c | region | terrain
73 | 0 | 135 | cornwall | Plains
74 | 0 | 136 | cornwall | Plains
```

Version 3 replaces stored `q | r` with a canonical, nonnegative three-axis unit ID `a | b | c`. At least one component must be zero. The inspector displays the components separated by hyphens and padded to at least three digits, so `(a=1,b=0,c=0)` appears as `001-000-000`. IDs are derived from the fixed grid origin, not the current map bounds, and newly painted hexes receive them automatically.

A step from the origin produces these direction codes:

| Direction | Derived axial step `(q,r)` | Three-axis code |
| --- | --- | --- |
| Right | `(1,0)` | `100` |
| Left | `(-1,0)` | `011` |
| Upper-left | `(0,-1)` | `010` |
| Lower-right | `(0,1)` | `101` |
| Lower-left | `(-1,1)` | `001` |
| Upper-right | `(1,-1)` | `110` |

The opposite codes are complementary because `(1,1,1)` represents zero net displacement. Conversion between the internal axial position and its canonical ID is:

```text
q = a - c
r = c - b

k = -min(q + r, 0, r)
a = q + r + k
b = k
c = r + k
```

For a version 3 grid, the unit centre in the declared projected CRS is:

```text
E = originEasting  + radius * sqrt(3) * (q + r/2)
N = originNorthing - radius * 1.5 * r
```

Version 3 can be grouped or ungrouped; its kingdom and region rows otherwise follow the same rules as versions 1 and 2. Saving a legacy map preserves its legacy version, while saving a projected map preserves version 3 and its grid metadata.

Keep the header, map name, optional version 3 grid, optional kingdoms, regions section, and hexes section in that order. Define at least one region. Region and kingdom IDs are unique and case-sensitive, and fields cannot contain `|`. Allowed terrain values are `Plains`, `Forest`, `Hills`, `Mountains`, and `Marsh`. Derived axial coordinate values range from −100000 to 100000; stored version 3 ID components must be nonnegative and canonical. Blank lines and full-line comments beginning with `#` are accepted. Saving writes a canonical file and replaces the destination atomically. Custom comments and spacing are discarded on save.

## Source layout

| Files | Responsibility |
| --- | --- |
| `src/model.*` | Hex and region records, projected grids, three-axis unit IDs, validation, UTF-8 parsing, atomic saves |
| `src/cartography.hpp` | Pointy-top axial geometry, hit testing, terrain symbols, map icons |
| `src/main.cpp` | Win32 event loop, map editing, camera, undo/redo, inspector, file dialogs |
| `src/canvas.*` | 2D mesh construction and font atlas |
| `src/vulkan.*`, `shaders/` | Win32 surface, swapchain, rendering pipeline, resource lifetime |
| `maps/` | Editable example map and blank starter |
