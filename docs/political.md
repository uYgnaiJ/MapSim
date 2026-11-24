# Political map mode

## Scope and historical method

`maps/england-867.political` is a **snapshot after the Northumbrian defeat at York on 21 March 867 and before the Nottingham campaign of 868**. It is not a map of the later Danelaw, the settled Scandinavian kingdom of York, Alfred's Wessex, or the later English shires.

The implementation separates two questions that are often conflated:

1. **Effective authority at the root view.** The root has five colour groups: the Great Army at York, Mercia, East Anglia, Wessex, and local Cornish authority. Northumbria is not shown as a sixth independent peer: Ecgberht's Scandinavian-backed client kingship is a child of the York authority.
2. **Geographic subdivisions after drill-down.** The existing 25 regions are retained, but they are not promoted to 25 sovereign kingdoms. In particular, Kent, Sussex, Surrey, Essex, Hwicce, Lindsey, and Middle Anglia do not appear as independent root states.

The supplied `deep-research-report.md` was used as a research checklist, especially its warnings that the Great Army was a mobile coalition rather than a settled territorial state, that East Anglia remained independent in 867, and that layered lordship should not be mistaken for modern bureaucracy. Its `turn…` citation placeholders were **not** used as evidence. The claims below were checked against the separately listed primary, academic, or institutional URLs.

### Sources checked

Accessed 24 November 2026 unless otherwise noted.

- **Anglo-Saxon Chronicle**, entries 813, 823, 835–838, 851–876 (Yale Avalon translation): <https://avalon.law.yale.edu/medieval/ang09.asp>. The 866 entry records peace and horse supply in East Anglia; 867 records the deaths of Osberht and Ælle at York and peace by the survivors. Nottingham belongs to 868, Edmund's defeat to the following campaign, Burgred's expulsion to 874, and Halfdan's division of Northumbrian land to 876. Those later events are deliberately not backdated. The earlier entries also document Cornish–West Saxon conflict and the West Saxon acquisition of south-eastern dependencies.
- **Annals of Ulster**, U867.8 (CELT, University College Cork): <https://celt.ucc.ie/document/T100001A>. This independently records the victory of the “dark foreigners” over the northern Saxons at York and Ælle's death. The transmitted annal heading has an AD offset, so it is synchronized with the English chronology rather than read mechanically.
- **Historia Regum / Symeon of Durham tradition**, Stevenson translation (1855), the northern annal for 867, printed p. 489: <https://archive.org/details/historicalworks00simegoog>. This much later source says that Ecgberht was appointed under Scandinavian dominion and ruled Northumbrians beyond the Tyne. It is the basis for the client link and for limiting what is called secure; it is not treated as a contemporary administrative survey. An accessible modern excerpt and chronology is also provided at <http://www.anglo-saxons.net/hwaet?do=get&type=chron&from=856&to=899>.
- **S 338**, an original Canterbury charter dated 867 (Electronic Sawyer): <https://esawyer.lib.cam.ac.uk/charter/338.html>. It names Æthelred king of the West Saxons and Kentishmen. This supports putting Kent under Wessex rather than drawing it as a separate peer kingdom. It also illustrates the charter, aristocratic, and ecclesiastical relationships through which ninth-century kings governed; it does not imply a modern county hierarchy.
- **Æthelred's reign**, Royal Family institutional biography: <https://www.royal.uk/ethelred-r866-871>; Britannica: <https://www.britannica.com/biography/Aethelred-I>. Alfred remains a brother and commander, not king, in this snapshot.
- **Burgred of Mercia**, Britannica: <https://www.britannica.com/biography/Burgred>. Burgred was still king in 867. His later appeal to Wessex in 868 was an alliance, not evidence that Mercia was already a West Saxon dependency; his expulsion belongs to 874.
- **Edmund of East Anglia**, Britannica quick facts: <https://www.britannica.com/facts/Edmund-king-of-East-Anglia>, read with the Chronicle above. East Anglia's accommodation of the army in 865/866 is not shown as Danish annexation. Edmund was still the local king in 867; conquest and settlement are later.
- **Hadley and Richards, “The Winter Camp of the Viking Great Army, AD 872–3, Torksey, Lincolnshire,” _Antiquaries Journal_ 96 (2016), 23–67**: <https://doi.org/10.1017/S0003581516000718> (open article: <https://www.cambridge.org/core/journals/antiquaries-journal/article/winter-camp-of-the-viking-great-army-ad-8723-torksey-lincolnshire/C54BB610EA9E0E567DC2C0622BA753EB>). The paper describes multiple warbands and a camp population in the thousands. This supports “Great Army” rather than an overly precise single monarch or mono-ethnic label. **Torksey is later evidence**: no Torksey population or territorial footprint is transferred to York in 867.
- **Annales Cambriae**, translated annals including Dungarth, king of Cernyw, drowned in 875: <https://www.roman-britain.co.uk/classical-references/welsh-celtic-literature/annales-cambriae/>. Dungarth is *not* inserted as the attested ruler in 867. The sidecar deliberately says “Ruler unnamed in 867.”
- **T. M. Charles-Edwards, _Wales and the Britons, 350–1064_** (Oxford Academic, DOI and bibliography): <https://academic.oup.com/book/10443>. The work includes Cornwall and Brittonic relations with the English. Together with the sparse annals, it counsels caution about calling Cornwall either a fully sovereign equal of Wessex or an ordinary West Saxon province.
- **Cornwall Heritage Trust, “The Age of the Saints”**: <https://www.cornwallheritagetrust.org/timeline/the-age-of-the-saints>. This institutional summary distinguishes the Cornish kingdom and its later tenth-century absorption. It is used as contextual heritage interpretation, not as proof of a precise 867 frontier or ruler.

## Hierarchy and colour design

The sidecar contains six named political nodes; region nodes are generated from the atlas:

```text
Root / effective authorities
├── Great Army at York                 #536B8F  slate blue
│   ├── Deira                          atlas region colour; direct-dominance approximation
│   └── Northumbria (client)           #86A9BE  lighter blue
│       ├── Bernicia
│       └── Western Northumbria        uncertain western reach
├── Mercia — King Burgred              #7D9B76  green
│   └── 8 existing regions, including Lindsey, Hwicce and the Middle Saxons
├── East Anglia — King Edmund          #D2A64F  amber
│   ├── North Folk
│   └── South Folk
├── Wessex — King Æthelred I           #B77E9C  mauve
│   └── 11 existing western, southern and south-eastern regions
└── Cornwall (West Welsh)              #5E9E96  teal
    └── Cornwall (Cerniw)
```

The five root colours were selected to remain distinguishable at map scale while fitting the atlas's muted palette. Lighter blue differentiates the Northumbrian client on drill-down without suggesting an unrelated root power. At deeper levels the existing region colours provide local variation.

### Important interpretations

- **Great Army at York:** a military occupation/effective supremacy node, not a settled territorial state with a securely known single king. Its root territory is a schematic map of the atlas's York/Northumbrian assignment, not a surveyed occupation boundary. “Danelaw” appears only in a negative caveat.
- **Northumbria:** `parent = york` and `independent = false` encode overlordship, not annexation or a modern department. Ecgberht's client rule beyond the Tyne is the secure part. The atlas's Western Northumbria assignment is explicitly uncertain.
- **Deira:** direct assignment to the York node follows the atlas's existing Danish/Scandinavian military-dominance interpretation. It is an approximation, not proof that every Deiran hex was garrisoned or administered by the army.
- **Mercia and Wessex:** independent roots. Their marriage/military relationship is not encoded as a parent link. The upper Thames, Lindsey, London/Middle Saxon, and other frontier assignments inherit the atlas's approximations.
- **East Anglia:** an independent Edmund root despite having bought peace and supplied horses. It must not be coloured as conquered Danish territory in 867.
- **Cornwall:** shown separately because a distinct local Cornish authority is more defensible than silently absorbing it into Wessex. The `true` root flag means “display as an effective local authority,” not a judgement that its sovereignty was uncontested. The precise tribute/overlordship relation with Wessex and the identity of its ruler in 867 remain uncertain.
- **Regions:** Bernicia, Deira, Hwicce, Devon, Kent, and similar names are useful geographic subdivisions here. They are not all claims of separate 867 kingships. No later Norman shire, Duchy of Cornwall, Danelaw polity, or tenth-century boundary is added merely to obtain another level.

## Sidecar file format

The format is UTF-8, pipe-delimited, stable-ID based, and intentionally readable without the C++ loader.

```text
FIELDATLAS-POLITICAL 1
map = England in 867

[polities]
# id | parent (- for root) | name | ruler | independent | #RRGGBB | detail
northumbria | york | Northumbria | Ecgberht I (client king) | false | #86A9BE | ...

[regions]
# atlas region id | leaf polity id
bernicia | northumbria
deira | york
```

Rules enforced by `src/political.cpp`:

- The header and `map =` binding must match the loaded `Map::name`.
- Full-line `#` comments and blank lines are allowed. There is no inline escaping: a field cannot contain `|`.
- Political IDs match `[a-z][a-z0-9._-]*`; IDs and map region assignments must be unique.
- Parent references may be forward references. Every parent must exist, self-parenting and cycles are rejected, and depth is not fixed at one or two levels.
- Colour must be exactly `#RRGGBB`; both letter cases are accepted. `independent` is exactly `true` or `false`.
- Every atlas region must occur exactly once in `[regions]`, and its leaf polity must exist. Region display names and region-level colours come from the atlas.
- An existing malformed sidecar throws a descriptive line-numbered error. Only a genuinely missing sidecar triggers fallback; bad data is not silently ignored.

`createPoliticalMode(map, data)` accepts a `.political` path. For convenience, an `.atlas` path is converted to the adjacent `.political` path. An empty or missing path uses the model fallback.

### Runtime behaviour

- `labels()` and `legend()` expose the currently visible children. At root these are the effective authorities; after drill-down they are the current node's children. Each label includes name, ruler in its concise `detail` line, colour, and **all descendant hexes**, not only an anchor. Longer historical qualifications stay in `inspector()` so they do not overflow the map label.
- `color()` uses the visible child on the current path. Territory outside the current subtree is blended 74% toward neutral paper (`#E8E4DC`), retaining a faint political identity without competing with the selected subtree.
- `activeHexes()` is the full map at root and the complete current subtree after drill-down, so fitting does not crop detached parts.
- `descend(hex)` enters only the displayed node containing that hex and only if that node has children. Foreign, unknown, and leaf clicks return `false`. `reset()` returns to root. `status()` displays the full current name path.
- `inspector()` reports atlas region, leaf political affiliation, ruler, client/independent status, historical caveat, authority chain, current view, and aggregated hex count.
- `refresh()` reloads an existing sidecar (or rebuilds fallback), recalculates every descendant hex/population statistic, and preserves a still-valid path. Map edits therefore update labels, fitting, and inspector counts without recreating the view.

### Missing-sidecar fallback

The fallback uses existing `Map::kingdoms`, including any programmatically supplied `parent`, `color`, and `independent` fields. `Region::kingdom` is the region's leaf polity. A null region kingdom makes that region an independent root. Zero kingdom colours inherit or receive deterministic palette colours, so old maps with default `color = 0` remain readable. Ungrouped v1/v3 maps therefore work without any political file. Invalid in-memory parent indices and cycles produce validation errors rather than unbounded recursion.

## Reproduction and independent checks

Run from the repository root:

```powershell
python build/political_generate.py            # regenerate the sidecar deterministically
python build/political_generate.py --check    # byte-for-byte freshness check
python build/political_validate.py            # independent Python structural/data check
python build/political_run_tests.py           # all checks + strict C++20 build + executable test
```

The retained test files are:

- `build/political_generate.py`: derives all 25 leaf assignments from the atlas and emits the documented historical node table.
- `build/political_validate.py`: an independent parser that checks v3 canonical hex IDs, duplicate/unknown references, exactly 4,987 unique tiles and 25 regions, sidecar/map agreement, RGB values, parent existence, acyclicity, five root territories, and complete non-overlapping coverage.
- `build/political_test.cpp`: independent C++ tests compiled only with shared `model.cpp` and the new `political.cpp`. They cover root labels and rulers; multi-level sidecar and in-memory parent chains; drill-down/reset; child-only descent; direct and descendant colours; external weakening; active fit sets; inspectors; refresh statistics and path retention; malformed colour rejection; ungrouped fallback; missing-sidecar fallback; and exact colour/label coverage of every one of England's 4,987 tiles.
- `build/political_run_tests.py`: reproducible clang++ MinGW test driver using `-Wall -Wextra -Wpedantic -Werror` (the shared model's old missing-field warnings are suppressed).

Expected validated root coverage is York 1,231; Mercia 1,747; East Anglia 396; Wessex 1,442; Cornwall 171 hexes, totalling 4,987.

## Limitations

The atlas geometry is derived from modern boundary data and is a schematic reconstruction, not a contemporaneous surveyed map. Ninth-century authority could be personal, overlapping, tributary, discontinuous, or confined to strategic centres. A single fill per hex cannot represent all of those relationships. The Great Army's exact command at York, actual control away from York, Ecgberht's western reach, Cornish overlordship, and several Mercian/West Saxon frontier zones remain uncertain. The sidecar carries these caveats in labels/inspectors rather than presenting false precision.
