# Religion mode: England in 867

## Result and historical scope

`src/religion.cpp` implements `createReligionMode(const Map&, const path&)`.
The dataset is `maps/england-867.religion`, companion to the snapshot after the
Battle of York (21 March 867) and before the Nottingham campaign.

The map's subject is **the prevailing religious affiliation of the local
population of each occupied hex**. It is not the religion of a king, a client
ruler, a mobile army, or a political region's current controllers. On that
definition, all 25 regions and all **4,987 occupied hexes** are assigned to
`christian` (`Christianity`, `#7895B1`). This intentionally produces a visually
uniform map. Adding a pagan Deira merely to make the display more varied would
misstate the historical evidence.

This is a coarse historical reconstruction, not a census, a percentage, or a
claim that every inhabitant held identical beliefs or practised in the same
way. Institutional Christian evidence is strong; the beliefs of individuals and
the survival or blending of older practices cannot be enumerated at hex scale.

## Research and classification procedure

The decisions were made in this order:

1. Separate **local-population religion** from **elite, military, and political
   affiliation**. Political control in the `.atlas` is not treated as evidence
   of mass conversion.
2. Establish the pre-existing Christian institutions of the Anglo-Saxon
   kingdoms and Cornwall, then test alleged exceptions against their exact
   date. In particular, Scandinavian land division in Northumbria belongs to
   876, not March 867.
3. Use the coarsest defensible classification. No sub-regional religious
   boundaries, demographic proportions, or unit-level majority exceptions are
   invented.
4. Put the York army in a separate, non-colouring military-context record.
   Describe affiliation as source-bound and non-static rather than treating
   “pagan Vikings” and “Christian natives” as immutable, homogeneous blocks.
5. Resolve every atlas hex through a unit rule or its region rule; unresolved
   and explicitly unknown values remain visibly neutral. Automated checks
   enforce both coverage and the Deira decision.

## Classification decisions

| Atlas regions | Mainstream assignment | Reason and qualification |
| --- | --- | --- |
| `bernicia`, `western-northumbria`, `deira` | Christianity | Northumbria had long-established Christian royal, episcopal, and monastic institutions. York's capture and Danish military dominance did not instantly replace the local population or its religion. Political and cultural uncertainty in western Northumbria is not evidence for a pagan majority. |
| `lindsey`, `mercia-core`, `wreocensaete`, `magonsaete`, `hwicce`, `middle-anglia`, `southern-mercia`, `middle-saxons` | Christianity | These were within the Christian Anglo-Saxon institutional world. The single category does not erase provincial churches, different practices, or political subordination to Mercia. |
| `north-folk`, `south-folk` | Christianity | East Anglia was still Edmund's Christian kingdom in 867. Its peace with and supply of the Great Army in 865–866 did not make its inhabitants pagan; Edmund's death and conquest belong to 869. |
| `devon`, `dorset`, `somerset`, `hampshire`, `wight`, `wiltshire`, `berkshire`, `kent`, `surrey`, `sussex`, `essex` | Christianity | These were within Wessex and its attached south-eastern political sphere, with established Christian institutions. The contemporary 867 Canterbury charter S 338 is a direct institutional witness, not a demographic survey. British and other inheritances are not recast as separate ninth-century religions. |
| `cornwall` | Christianity | Cornwall had its own Christian church and institutional history. It is not assigned a separate “Celtic religion,” and later ecclesiastical arrangements are not projected backward. See the Canterbury caveat below. |

Every region has an explicit row in the sidecar. The notes in that file retain
more local qualifications, especially for Bernicia, Deira, western Northumbria,
East Anglia, Kent, and Cornwall. There are deliberately **no unit-level
majority-religion overrides**: the evidence does not support finer boundaries
than the atlas regions.

### Deira and Danish-held York: two different claims

The *Anglo-Saxon Chronicle* calls the invading force a “heathen army” and
records its move to York, the Northumbrian attack, and the survivors' peace in
867. The same narrative places Halfdan's division of Northumbrian land in 876.
The Victoria County History account likewise separates the 867 conquest from
that later settlement and describes the persistence and subsequent conversion
of Christianity at York, including Archbishop Wulfhere's return in 873.

Accordingly:

- `deira | christian | approximate` describes the **local majority**, including
  the hex containing York.
- The army is not a representative sample of Deira's settled population. No
  numerical population or army-to-civilian ratio is asserted.
- One separate `[military-context]` row is attached to canonical unit
  **088-000-050**, a cartographic locator for York. Its affiliation reads
  “Predominantly pagan / non-Christian in contemporary Christian narratives.”
  “Heathen” is a category used by Christian annalists, not a claim to know each
  participant's self-description.
- The row explicitly says that this is **not the majority religion of York or
  Deira**. It is a city-based annotation, not an estimated camp footprint or a
  claim that the army occupied only one hex.
- Military context never enters `color()`, `labels()`, or `legend()`. It appears
  only in that unit's `inspector()`, which continues to report Christianity as
  the population identity. Other Deira units do not inherit the annotation.
- Religion was not static. The Chronicle's account of Guthrum's baptism in 878
  and the later mixed Christian/pagan material discussed by VCH warn against a
  permanent binary. Those later developments are not backdated as measured
  facts about every participant in 867. “Predominantly” therefore summarizes
  the contemporary portrayal; it is not a counted percentage.

### Cornwall and Canterbury: what is and is not dated

O. J. Padel's historical chapter in *Early Cornish Sculpture* (Corpus of
Anglo-Saxon Stone Sculpture XI, 2013) reports that a Cornish bishop Kenstec
formally acknowledged Archbishop Ceolnoth of Canterbury, whose archiepiscopate
is dated 833–870. Padel places this during Ecgberht's reign **or later in the
ninth century**, names Kenstec's see as Dinuurrin, and says its location is
unknown. He also distinguishes this from the see established at St Germans
around 930.

Thus the 867 sidecar does **not** claim that a particular Canterbury/St Germans
administrative arrangement was certainly already in force on 21 March 867. The
profession's 833–870 date range straddles the snapshot. Cornwall remains
Christian throughout that uncertainty; ecclesiastical jurisdiction is not used
to manufacture a different religion. The St Germans arrangement of c.930 is
not backdated.

## Sources independently checked

All URLs below were consulted for this implementation on 24 November 2026.
Narrative sources have perspectives and later transmission histories; none is
used as a headcount.

1. **Anglo-Saxon Chronicle, ninth-century entries**, translated text hosted by
   Yale Law School's Avalon Project:
   <https://avalon.law.yale.edu/medieval/ang09.asp>.
   Used for the “heathen army” wording, East Anglian peace and horses (866), the
   York campaign (867), Halfdan's land division (876), and Guthrum's baptism
   (878). This is a West Saxon Christian narrative, so its labels are not
   treated as neutral self-descriptions.
2. **“Before the Norman Conquest,” A History of the County of York: the City of
   York**, Victoria County History (1961), British History Online:
   <https://www.british-history.ac.uk/vch/yorks/city-of-york/pp2-24>.
   Used for Edwin's baptism at York in 627, the archbishopric and Christian
   school, the 866/867 conquest, Wulfhere, conversion and mixed material, and
   the settlement sequence around 876. Its later conversion evidence is not
   confused with direct observation of 867.
3. **Ashmolean Museum, University of Oxford, “Viking Myths and Beliefs”**:
   <https://anglosaxondiscovery.ashmus.ox.ac.uk/vikings/viking-belief.html>.
   Provides the concise museum-level distinction that the arriving Vikings were
   pagan while the Anglo-Saxons were already Christian, and that some Vikings
   subsequently converted. It supports the broad contrast, not a demographic
   percentage.
4. **Bede, Ecclesiastical History, Book II**, Fordham Internet Medieval
   Sourcebook translation:
   <https://sourcebooks.web.fordham.edu/basis/bede-book2.asp>.
   Used as an early narrative witness to seventh-century Christian missions and
   institutions in Northumbria, East Anglia, Lindsey, Kent, and Essex. Bede's
   conversion narrative is itself Christian and teleological; it is combined
   with the later institutional evidence above rather than read as a survey.
5. **Electronic Sawyer S 338**, A.D. 867 Canterbury charter:
   <https://esawyer.lib.cam.ac.uk/charter/338.html>.
   The catalogue identifies a ninth-century manuscript and modern assessments
   of the document as contemporary/original. Æthelred's grant to the priest
   Wighelm at St Martin's, with Archbishop Ceolnoth among the witnesses, gives
   direct evidence of functioning Christian institutions in the snapshot year.
6. **O. J. Padel, “Historical background to the sculpture,”** in *Early Cornish
   Sculpture*, Corpus of Anglo-Saxon Stone Sculpture XI (Oxford, 2013), pp.
   21–36, especially pp. 24–25:
   <https://yeahbut.co.uk/corpus/vol11_chap4.pdf>
   (alternate copy: <https://chacklepie.com/ascorpus/vol11_chap4.pdf>).
   Used directly for Kenstec, Ceolnoth, Dinuurrin, and St Germans. Cambridge's
   Department of Anglo-Saxon, Norse & Celtic independently lists Padel and this
   publication: <https://www.asnc.cam.ac.uk/people/Oliver.Padel>.

## Sidecar format and runtime semantics

The file is UTF-8, pipe-delimited, and allows blank lines and full-line `#`
comments. Its order is:

```text
RELIGIONATLAS 1
map = England in 867
title = England in 867 - majority-population religion

[identities]
# id | display name | #RRGGBB | semantic scope

[regions]
# atlas region ID | identity ID | approximate/uncertain/unknown | note

[units]
# canonical aaa-bbb-ccc | identity ID | approximate/uncertain/unknown | note

[military-context]
# canonical aaa-bbb-ccc | group | affiliation | scope caveat
```

- A unit rule overrides its region rule. Unit IDs must be canonical,
  nonnegative, in the atlas, and unique. Region IDs must exist in the atlas and
  be unique in the sidecar.
- `unknown` is a built-in identity, not a religion, with neutral `#A8A8A8`.
  An `unknown` assignment must use `unknown` confidence; known identities use
  `approximate` or `uncertain`. `approximate` describes coarse geographic
  generalisation, not a numeric probability.
- The sidecar's `map` must exactly match `Map::name`, preventing accidental
  reuse on another atlas. Callers may pass either the `.religion` path or the
  companion `.atlas` path; the latter is changed to `.religion`.
- Every occupied map hex is materialised in exactly one identity's label hex
  list. `labels()` supplies the identity name, count/description, colour, and
  full hex list. `legend()` lists occupied identities with matching colours and
  counts. `inspector()` reports unit ID, region, population scope, confidence,
  assignment origin and note, plus any explicitly separate military context.
- If the sidecar is absent, unreadable, malformed, invalid UTF-8, or belongs to
  another map, construction does not throw. All hexes become explicit neutral
  unknown, labels and legend remain usable, and `status()`/`inspector()` expose
  the fallback. An omitted region in an otherwise valid sidecar affects only
  that region; outside-atlas colour queries are also neutral.
- `[military-context]` records are deliberately not identity definitions and
  have no colour. They cannot alter majority-population classification.

## Reproduction and checks

From the repository root:

```powershell
# Regenerate intentionally, after reviewing curated notes in the script.
python build/religion_generate.py

# One-command deterministic check, independent validator, compile, and C++ test.
powershell -NoProfile -ExecutionPolicy Bypass -File build/religion_build.ps1
```

The build script keeps `build/religion_validation.txt` and
`build/religion_test_results.txt`. The validator independently reads both files,
checks all 4,987 unit IDs, requires all 25 region rules, forbids an invented
extra population religion, and requires `deira` to resolve to 553 Christian
majority hexes with zero pagan-majority hexes. It also checks that the York
record is context-only and retains its non-static/non-homogeneous caveat.

The C++ test compiles `src/model.cpp`, `src/religion.cpp`, and
`build/religion_test.cpp`, then exercises the actual public mode interface. It
verifies shared colours, complete labels and legend, all inspectors needed for
the York distinction, `.atlas` companion resolution, missing/malformed/wrong-map
fallbacks, explicit and unassigned unknowns, unit-over-region precedence,
outside-atlas queries, and an empty map. Test fixture files are removed after
the run.
