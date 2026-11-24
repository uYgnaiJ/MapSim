# Population mode: England in 867

## What the numbers mean

**Every value is a low-confidence heuristic estimate, not a census total, an observed town population, or a precise historical statistic.** There is no population census for England in 867. Even the much richer Domesday evidence of 1086 requires contested decisions about omitted people, households and multipliers. Moving back another 219 years adds uncertainty rather than removing it.

The sidecar therefore has two separate aims:

1. keep the national total within a defensible **order-of-magnitude working envelope**; and
2. show a predominantly rural, regionally varied distribution without copying modern population data.

The selected central scenario is **1,200,000 people**, with a deliberately broad sensitivity envelope of **800,000–1,800,000**. This envelope is not a confidence interval and has no frequentist or Bayesian probability attached to it. It is a scenario range for a map whose absolute and local values are both poorly constrained. No kingdom-by-kingdom figures are claimed as secure estimates.

The supplied background report was used as a research lead about agricultural society, army scale and demographic limitations. Its placeholder citations were **not** reused. The independently checked links below are the actual references for this implementation.

## Online research and how it was used

All links below were checked online on 24 November 2026.

- [Bank of England (2016), *A Millennium of Macroeconomic Data*, processed by Our World in Data](https://ourworldindata.org/grapher/population-of-england-millennium) — the [CSV](https://ourworldindata.org/grapher/population-of-england-millennium.csv) gives **1.71 million for England in 1086**, and the [metadata](https://ourworldindata.org/grapher/population-of-england-millennium.metadata.json) identifies the source and 1086–2016 time span. This is a later order-of-magnitude anchor only. It is **not** an 867 observation, and the generator does not apply a mechanical growth-rate back-projection.
- [Open Domesday: About](https://opendomesday.org/about/) — describes the 1086 survey and the University of Hull data created by Professor J. J. N. Palmer's team. Domesday is evidence about land, resources and recorded people, not a ready-made modern census. Its later regional pattern was used only as a qualitative sanity check. No Domesday headcounts, county shares or household multipliers are imported, and the northern impact of the Conquest makes direct transfer especially unsafe.
- [Hamerow, “Settlement mobility and the ‘Middle Saxon Shift’” (1991)](https://doi.org/10.1017/S026367510000171X) — documents settlement mobility and warns against treating later nucleated villages as direct early/middle Saxon predecessors. Consequently the model does not populate the map by stamping modern or later villages onto 867.
- [Williamson, “The Environmental Contexts of Anglo-Saxon Settlement” (2010)](https://doi.org/10.1017/9781846158780.007) — supports treating environment and landscape as relevant to settlement. Here that evidence informs only broad relative terrain capacities; it does not calibrate a predictive archaeological model.
- [Victoria County History, “York before the Norman Conquest”](https://www.british-history.ac.uk/vch/yorks/city-of-york/pp2-24) — describes York as the north's largest urban settlement, its court/minster/market roles, and the later intensification of Viking-age economic activity. The scenario gives York a strong settlement correction but does not copy a later Jórvík population back into March 867.
- [Rory Naismith, *Citadel of the Saxons: The Rise of Early London* (2019); bibliographic review](https://doi.org/10.1080/03058034.2020.1717040) and the [Anglo-Saxon Chronicle's ninth-century entries](https://avalon.law.yale.edu/medieval/ang09.asp) — provide chronological discipline for London. Alfred's fortification is entered under 886, after this map's date. The London parameter therefore represents a pre-886 settlement scenario, not London's later medieval or modern size.
- [Martin Biddle (ed.), *Winchester in the Early Middle Ages*; Cecily Clark's review](https://doi.org/10.3828/archives.1977.14) — bibliographic context for early medieval Winchester. The Chronicle also records the 860 attack on Winchester. Neither source supplies the sidecar's numerical Winchester parameter.
- [Hadley and Richards, “The Winter Camp of the Viking Great Army, AD 872–3, Torksey” (2016)](https://doi.org/10.1017/S0003581516000718) — its abstract reports **several thousand** people at the later 872–873 camp, including warriors, craftworkers and merchants. This rejects a “few hundred only” assumption but does **not** reveal the army's size at York in 867. The population surface represents residents; it does not add a speculative Great Army headcount to York.
- The [Chronicle](https://avalon.law.yale.edu/medieval/ang09.asp) places the army at York in 867 and systematic Northumbrian land division/cultivation in 876. The sidecar therefore does not assume that a later Scandinavian rural colonisation pattern was already present in 867.

The settlement quantities below are **model allocations**, not numbers quoted from these publications. Modern population counts are not read or used anywhere by the generator. Modern British National Grid coordinates are used only to locate historic places geographically.

## Geometry

The atlas declares `EPSG:27700`, origin `(E=100000, N=700000)` and hex circumradius `R=3300 m`. For each canonical three-axis ID, the generator derives `q=a-c` and `r=c-b`, then calculates the centre:

```text
E = 100000 + R * sqrt(3) * (q + r/2)
N = 700000 - R * 1.5 * r
```

Hex area is:

```text
A = (3 * sqrt(3) / 2) * R^2 / 1,000,000
  = 28.293049942 km²
```

For 4,987 occupied hexes, the nominal covered area is **141,097.440059 km²**. This is the sum of complete hex areas, not a claim that every coastal hex was wholly land.

## Deterministic model

### 1. Rural capacity surface

For hex `i`, the unnormalised rural weight is:

```text
w(i) = A * 9.0 * regionModifier(i) * terrainCapacity(i) * localField(i)

localField(i) = 1
              + 0.07 * sin(E / 37000)
              + 0.05 * cos(N / 43000)
              + 0.03 * sin((E + N) / 57000)
```

The smooth local field ranges approximately from 0.85 to 1.15. It prevents large, artificially uniform region blocks; it is **not evidence for individual settlements**. The nominal `9.0 people/km²` base is only a relative starting density: global normalisation to the chosen total means it is not asserted as a measured 867 density.

Terrain capacities are:

| Terrain | Capacity |
| --- | ---: |
| Plains | 1.00 |
| Forest | 0.62 |
| Hills | 0.48 |
| Mountains | 0.16 |
| Marsh | 0.22 |

Regional agrarian modifiers are explicit heuristic parameters, not modern population shares:

| Region | Modifier | Region | Modifier |
| --- | ---: | --- | ---: |
| bernicia | 0.52 | western-northumbria | 0.58 |
| deira | 0.82 | lindsey | 1.18 |
| mercia-core | 1.05 | wreocensaete | 0.82 |
| magonsaete | 0.83 | hwicce | 1.00 |
| middle-anglia | 1.13 | southern-mercia | 1.16 |
| middle-saxons | 1.12 | north-folk | 1.42 |
| south-folk | 1.46 | devon | 0.82 |
| dorset | 0.94 | somerset | 1.05 |
| hampshire | 1.05 | wight | 1.08 |
| wiltshire | 1.03 | berkshire | 1.11 |
| kent | 1.26 | surrey | 0.95 |
| sussex | 0.96 | essex | 1.16 |
| cornwall | 0.68 |  |  |

The 20 settlement kernels sum to `S=39,800`, leaving a rural target `R=1,160,200`. Initial rural quotas are:

```text
ruralQuota(i) = R * w(i) / sum(w)
```

They are converted to nonnegative integers by largest-remainder apportionment. Fractional remainders are ordered descending, with ties broken by `q`, then `r`, so the operation is deterministic and `sum(rural(i))=R` exactly.

### 2. Settlement corrections

Each settlement allocation `S(j)` is an **additional concentration above the rural baseline**, not a claimed archaeological city population. For each place:

1. `round(0.58 * S(j))` people are assigned to the occupied hex nearest its British National Grid location.
2. The remainder is apportioned with weights `exp(-d(i,j)² / (2*sigma(j)²))`, using hex-centre distance `d` and a `2.5*sigma` cutoff.
3. Overlapping kernels are added. Every kernel is independently rounded by the same deterministic largest-remainder method.

| Settlement kernel | E | N | Added people | Sigma (km) |
| --- | ---: | ---: | ---: | ---: |
| York / Eoforwic | 460500 | 451700 | 6500 | 6.0 |
| London / Lundenwic | 530800 | 181000 | 5500 | 6.0 |
| Winchester | 448500 | 129500 | 5000 | 5.5 |
| Canterbury | 615000 | 157800 | 2600 | 5.5 |
| Lincoln | 497500 | 371500 | 2400 | 5.5 |
| Ipswich / Gipeswic | 616300 | 244600 | 1800 | 5.5 |
| Hamwic / Southampton | 442500 | 112500 | 1800 | 5.5 |
| Exeter | 292300 | 92500 | 1600 | 5.5 |
| Worcester | 385000 | 255000 | 1400 | 5.5 |
| Gloucester | 383200 | 118600 | 1400 | 5.5 |
| Tamworth | 420000 | 204000 | 1300 | 5.5 |
| Leicester | 458700 | 304400 | 1100 | 5.5 |
| Bath | 375000 | 165000 | 1100 | 5.5 |
| Thetford | 587500 | 283000 | 1000 | 5.0 |
| Nottingham | 457400 | 339300 | 1000 | 5.5 |
| Rochester | 574000 | 176800 | 1000 | 5.5 |
| Repton | 430300 | 227200 | 900 | 4.5 |
| Oxford | 451300 | 206300 | 900 | 4.5 |
| Wareham | 392300 | 87400 | 800 | 4.5 |
| Dorchester (Dorset) | 369000 | 90700 | 700 | 4.5 |

The final value is `population(i)=rural(i)+sum_j(kernel(j,i))`. All values are nonnegative 32-bit integers. Mobile campaigning armies are not separately added to this resident surface.

## Generated result

The current checked-in sidecar and `build/population_validation.json` give:

| Measure | Result |
| --- | ---: |
| Hexes | 4,987 |
| Total | 1,200,000 |
| Working total envelope | 800,000–1,800,000 |
| Rural component | 1,160,200 |
| Settlement-kernel component | 39,800 (3.3167%) |
| Minimum / maximum cell | 23 / 4,356 |
| P25 / median / P90 | 139 / 238 / 372 |
| P95 / P99 | 399 / 461 |
| Cells above 1,000 / 2,000 | 13 / 4 |
| Rural median: Plains | 290 |
| Rural median: Forest / Hills | 176 / 117 |
| Rural median: Marsh / Mountains | 100 / 30 |

Thus a few settlement peaks do not turn the whole country into a set of large cities. The maximum cell is York's kernel plus its rural baseline; it is not presented as a measured population of York.

## Sidecar format and interface behaviour

`maps/england-867.population` is an independent UTF-8 `FIELDPOP 1` text file. It stores the model metadata, all terrain/regional/settlement parameters and one row per hex:

```text
q | r | estimated people | rural component | settlement-kernel component
```

It does not alter `.atlas` files or shared `Tile` records. `createPopulationMode(const Map&, const path&)` accepts a direct `.population` path, the `.atlas` path, or the `maps` directory. The loader checks map name, declared hex count, coordinate coverage, duplicates, component sums and total. Missing, unreadable, malformed or mismatched data produces no exception to the application:

- every affected hex is neutral grey (`D9D8D2`) and treated as **unknown (0)**;
- status and legend clearly report missing/invalid data;
- the inspector says `Estimated population: unknown (0)`.

With valid data, the inspector shows a compact estimated population and year. `labels()` is empty by design.

### Continuous colour scale

Let `M` be maximum cell population, `a=max(1, first quartile of positive values)`, and `p>0`:

```text
t = ln(1 + p/a) / ln(1 + M/a)
colour = sequentialRamp(clamp(t, 0, 1))
```

Zero uses the lightest colour. All-zero and `M<=a` cases have explicit safe handling. The six sequential earth-tone stops are:

```text
F6F0E2 → E4D7B4 → C3B184 → 987F59 → 644D42 → 342D35
```

Interpolation is continuous and all RGB channels are nonincreasing with population. Log compression preserves rural variation while still allowing York, London and Winchester to stand out; a linear max-normalised scale would compress most rural hexes into nearly one light colour. The legend says **Estimated**, reports the actual cell range and total/envelope, and identifies the scale as logarithmic and low confidence.

## Reproduce and test

From the repository root:

```powershell
# Check that the committed sidecar is exactly reproducible, validate data,
# compile the C++ tests with warnings as errors, and run them.
powershell -ExecutionPolicy Bypass -File build/population_check.ps1

# Explicitly regenerate maps/england-867.population before the same checks.
powershell -ExecutionPolicy Bypass -File build/population_check.ps1 -Regenerate
```

Individual tools:

```powershell
python build/population_generate.py --check
python build/population_validate.py
```

- `build/population_generate.py` uses only the Python standard library and reads the atlas without modifying it.
- `build/population_validate.py` independently checks all 4,987 coordinates, nonnegative integers, exact totals, distribution limits and terrain ordering; it writes `build/population_validation.json`.
- `build/population_test.cpp` exercises the public C++ interface, including total/coverage, monotone log colours, known zero, a 1,000,000-person extreme, negative/malformed input, empty maps and missing files.

## Limitations and appropriate use

- The national total, regional modifiers and settlement allocations are explicit scenario choices, not recovered statistics. Local values may be proportionally much more uncertain than the national envelope.
- The total envelope captures only simple scale sensitivity. It does not capture uncertainty in coastlines, terrain, region boundaries, settlement location, abandonment, warfare, migration or epidemic losses.
- Terrain is an approximate atlas input. A `Plains` hex is not proof of cultivated arable land, and `Forest` does not imply empty land. Marsh and mountain penalties are broad relative assumptions.
- Settlement kernels are cartographic devices. Their cores and Gaussian tails are not walls, hinterland boundaries, parish populations or archaeological density estimates.
- Domesday and later urban evidence cannot simply be rewound to 867. The Harrying of the North, later agricultural expansion, settlement shifts and changing urban networks all break direct comparability.
- The map excludes a separate speculative army count; it therefore must not be used to infer military manpower or the number of combatants at York.
- Exact integer arithmetic and exact reconciliation are computational properties only. They do **not** imply historical precision.
