# 文化地图模式：England in 867

## 1. 目标、研究计划与交付物

本模式回答的问题不是“867 年每格属于哪个国王”，而是：**在证据允许的粗粒度上，哪一区域文化／身份认同可以作为该格的主流标签，哪里不能诚实地作出这种判断？**

实际工作流程：

1. 读取 `maps/england-867.atlas` 的 24 个既有区域和 4987 个 unit，不改动地图或政治分类。
2. 分别核查 Bede 的族群称谓、Tribal Hidage 与地方 folk/province、九世纪政治从属、Old English 方言证据、西部 Brittonic 连续性，以及 866–876 年 Scandinavian 军队活动与定居的年代差。
3. 先确定证据等级，再确定分类粒度。North/South Folk 不拆成两种文化；Middle Anglia 不伪造内部小族边界；western Northumbria 无法选择单一人数占优势的认同，故显式 `unknown`。
4. 由 `src/culture.cpp` 实现只读 sidecar、`ModeView` 着色、标签、图例、状态与 inspector；由 `maps/england-867.culture` 保存身份定义及逐区域分配。
5. `build/culture_generate.py` 校验并生成逐格表，`build/culture_tests.cpp` 直接测试 C++ 公共接口，`build/culture_build.ps1` 一键复现。

## 2. “文化”在这里的语义

这里显示的是 **mainstream culture / regional identity（主流文化或区域身份认同）的谨慎重建**，不是现代意义上的民族、种族、国籍，也不是每个居民的自我认同。一个标签最多表示某地区在文献、语言和政治共同体证据下可采用的宽标签。

- 身份可以叠层：一个人可以同时有本地 lineage、folk、kingdom、Christian 等认同。本图只选择一种适合当前比例尺的区域标签，不否认其他层次。
- 政治统治不等于文化替换。Kent、Sussex、Essex 虽在 West Saxon 政治体系中，仍分别保留 Kentish、South Saxon、East Saxon；Wight 的 Jutish 传承记录也不会自动变成 867 年的独立多数认同。
- 地方 folk 名不等于纯血缘群体。Bede 的 Angles/Saxons/Jutes 是八世纪的解释框架；器物、丧葬和地名不能单独证明人口来源或自我认同。
- Old English 的 Northumbrian、Mercian、Kentish、West Saxon 方言分类提供辅助背景，不是逐格语言边界。存世文本偏向教会和精英，且许多证据晚于 867。
- `unknown` 是证据类别，不是“无人居住”，更不是一种文化。其固定中性色为 `#A8A8A8`。
- `approximate` 表示宽身份有文献基础，但 Atlas 多边形和文化过渡带是近似的；`uncertain` 表示占领、 frontier 或内部构成使这一具体区域分配更不确定；`unknown` 表示不断言单一人数占优势的认同。

## 3. 关键年代判断：为什么 Deira 没有在 867 整体“丹麦化”

《Anglo-Saxon Chronicle》867 年条记载的是军队进入 York、Northumbrian 内战、两位国王死亡以及幸存者议和。它没有记载 Deira 全体居民被替换，也没有给出 Danish 人口占多数的证据。该编年史把 Halfdan 分配 Northumbrian 土地、军队开始耕作放在 **876 年**。

因此本图：

- `bernicia` 与 `deira` 共用 **Northumbrian Anglian** 和 `#4F78A0`；
- `deira` 标为 `uncertain`，inspector 明说 Danish-held York 是军事／政治控制，并提示 876 年土地分配和后续文化变化；
- 不建立 “Danish Deira”、“Danish” 或 “Norse” 主流文化色块；
- 不把后期 Yorkshire/East Midlands 的 Old Norse 地名和词汇影响直接回填到 867 年 3 月；
- 不把复合、流动的 Great Army 简化成当地全体居民的族群代表。

用户提供的补充研究材料强调了军队复合构成、语言文化多层性和 Old Norse 证据的年代限制。该材料的 `turn…search…` 引用是占位符，**没有被当作可验证来源**；上述结论均另以编年史、考古和地名研究的公开 URL 独立核查。

## 4. 实际分类

### 4.1 身份、颜色与格数

| identity id | 显示名 | 颜色 | Atlas 区域 | 格数 |
|---|---|---:|---|---:|
| `northumbrian` | Northumbrian Anglian | `#4F78A0` | Bernicia 328；Deira 553 | 881 |
| `mercian` | Mercian Anglian | `#6C9A62` | Mercia (Trent core) 371；Southern Mercia 184 | 555 |
| `lindsey` | Lindsey Anglian (Mercian sphere) | `#8FA45F` | Lindsey | 163 |
| `wreocensaete` | Wreocensæte (western Mercian) | `#4E8B72` | Wreocensæte | 211 |
| `magonsaete` | Magonsæte (western Mercian) | `#7A9B5A` | Magonsæte | 185 |
| `hwicce` | Hwiccean (Mercian) | `#6F9F8A` | Hwicce | 192 |
| `middle-anglian` | Middle Anglian (Mercian) | `#A39156` | Middle Anglia | 350 |
| `middle-saxon` | Middle Saxon | `#8D7860` | Middle Saxons | 91 |
| `east-anglian` | East Anglian | `#D1A33E` | North Folk 249；South Folk 147 | 396 |
| `west-saxon` | West Saxon | `#B56F8E` | Devon 276；Dorset 104；Somerset 176；Hampshire 150；Wight 17；Wiltshire 124；Berkshire 58；Surrey 71 | 976 |
| `kentish` | Kentish (Cantware) | `#8D6BA8` | Kent | 161 |
| `south-saxon` | South Saxon | `#C47B57` | Sussex | 154 |
| `east-saxon` | East Saxon | `#A55464` | Essex | 151 |
| `cornish` | Cornish / West Welsh (Cerniw) | `#318C83` | Cornwall | 171 |
| `unknown` | Unknown / unassigned | `#A8A8A8` | Western Northumbria | 350 |
| **合计** | **14 个命名身份＋unknown** |  | **24 个区域** | **4987** |

其中 4637 格有命名认同，350 格显式 unknown。置信度计数为：`approximate` 3491、`uncertain` 1146、`unknown` 350。

### 4.2 逐区域逻辑与限制

- **Bernicia / Deira**：两者早已构成 Northumbria 的不同省区；本图采用共同的 Northumbrian Anglian 外层身份，而不把 Bernician/Deiran 王室竞争直接解释成两个封闭文化。Deira 因 867 年占领局势另标 `uncertain`。
- **Western Northumbria**：Northumbrian 扩张的政治归属不足以判定 Anglian、Brittonic 或混合地方认同的人数优势。Rheged 的位置、精英连续性和后期 Strathclyde 扩张均有争论，所以 350 格全部显式 unknown；不把十世纪 Norse/Gaelic/Brittonic 地名格局倒推到 867。
- **Mercia core / Southern Mercia**：保留宽 Mercian Anglian 标签，不用证据不足的小族名进一步细分 Thames/Trent 过渡带。
- **Lindsey**：其王国、主教区和地方身份有独立证据，679 年后处于 Mercian 控制不等于立刻失去地方身份。标签保留 Lindsey，同时括注 Mercian sphere；British 背景不被抹去。
- **Wreocensæte**：九世纪 charter 等资料可独立支持这一地方名。区域色只表示 provincial affiliation，不声称其人口“纯种”或边界精确。
- **Magonsæte**：地方群体有文献依据，但精确范围及 Tribal Hidage 的 `Westerne` 是否就是它存在学术分歧，因此只保留既有 Atlas 粒度并标 `approximate`。
- **Hwicce**：文献和教会史支持 Hwiccean 区域身份；在 867 年它是 Mercia 的组成部分，并不因此统一改成 Mercian core 色。
- **Middle Anglia**：这里本由多个小政治／folk 单位组成。采用粗 Middle Anglian 是有意减少虚假精度，不是宣称内部一致。
- **Middle Saxons**：八世纪 charters 证明 Middle Saxon province；它同 East Saxon 的历史关系及先前独立程度并不完全清楚，所以与 Essex 分色但不过度解释。
- **North Folk / South Folk**：Norfolk/Suffolk 是 East Angles 的北部人民／南部人民，共用 East Anglian，不制造两种现代郡文化。
- **Devon / Cornwall**：Devon 采用宽 West Saxon 并标 `uncertain`，承认 Dumnonian/British 连续性和内部语言前沿不明。Cornwall 采用 Cornish / West Welsh，也标 `uncertain`；West Saxon 权力、精英和定居影响正在增加，但不足以消除 Cornish 标签。东西接触带逐格位置不伪造，936 年 Tamar 边界也不倒投到 867。
- **Hampshire / Wight**：归入宽 West Saxon。Bede 记录 Wight 和 Solent 对岸的 Jutish 渊源，但这种传统在 867 年是否仍是主要自我认同并不确定；Wight 因此为 `uncertain`，而不是新造一块确定的 Jutish 多数区。
- **Berkshire / Surrey / Wiltshire**：upper Thames 和 Surrey 是长期接触、争夺区域。Berkshire、Surrey 的 West Saxon 分配标 `uncertain`；Surrey 的本地连续性、Kentish 和 Middle Saxon 联系写在 inspector，不把政治易手当作瞬间文化替换。
- **Kent / Sussex / Essex**：分别保留 Kentish、South Saxon、East Saxon，明确展示“政治从属 Wessex ≠ 主流认同 West Saxon”。

## 5. sidecar 格式与程序行为

`createCultureMode(const Map&, const std::filesystem::path&)` 的第二个参数直接指向 `.culture` 文件。格式为 UTF-8、竖线分栏，`#` 起始行是注释：

```text
CULTUREATLAS 1
map = England in 867
title = England in 867 — mainstream culture and regional identity

[identities]
# lowercase-id | display name | #RRGGBB | semantic scope
northumbrian | Northumbrian Anglian | #4F78A0 | ...

[regions]
# atlas-region-id | identity-id | approximate/uncertain/unknown | note
bernicia | northumbrian | approximate | ...

[hexes]
# 可选 canonical unit override：aaa-bbb-ccc | identity-id | confidence | note
```

规则：

1. `map` 必须与 `Map::name` 完全一致，防止误用其他地图的数据。
2. identity id、region id 和 unit 引用均严格校验；重复定义、未知引用、无效颜色和非法 canonical ID 会抛出带行号的异常，不静默制造错误地图。`unknown` 身份必须与 `unknown` 置信度成对使用。
3. 不同命名身份使用不同颜色；同一身份的全部 unit 必然同色。内建 `unknown` 不可被重定义或改色。
4. unit override 优先于 region；没有 override 时继承 region。当前 867 数据刻意没有 unit override。
5. sidecar 存在但漏掉某区域时，该区域显示 unknown，并在 status 中计为 unassigned/unknown；inspector 说明是缺分配而不是历史结论。
6. **sidecar 不存在或 path 为空时，不抛异常**：全部 4987 格为中性 `#A8A8A8` 的 `Unknown / unassigned`，`labels()` 与 `legend()` 各有一个完整 unknown 项，status 明示 `No culture sidecar`，inspector 不从政治区域猜文化。路径存在但打不开、文件损坏或地图不匹配则报告错误。
7. `labels()` 为每个实际出现的身份提供显示名、格数、语义说明、颜色和完整 `hexes`；`legend()` 提供相同身份的颜色及格数；`inspector()` 提供 unit ID、q/r、身份、Atlas 区域、置信度、分配依据、身份语义、区域注释和解释范围。

## 6. 来源与独立核查

以下来源均于 **2026-11-24** 在线调查；URL 同时记录其能支持和不能支持的内容。正式数据没有把搜索摘要或占位引用当作唯一证据。

1. **Bede, _Ecclesiastical History_, I.15**（约 731 年完成；英译本）  
   https://www.gutenberg.org/ebooks/38326  
   支持 East/Middle Angles、Mercians、Northumbrians、East/South/West Saxons、Kent/Wight 的 Jutish 传统等历史称谓。它是早期的族源组织框架，不是五世纪移民的现代人口统计，也不能单独证明 867 年的自我认同。
2. **_Anglo-Saxon Chronicle_，866、867、876 年条（Yale Avalon 英译）**  
   https://avalon.law.yale.edu/medieval/ang09.asp  
   支持 East Anglia 驻军、York 战事、Northumbrian 幸存者议和，以及 876 年 Northumbrian 土地分配之间的年代区分。编年史主要在 West Saxon 环境编成，仍需注意视角与纪年。
3. **York Archaeology，Richard Hall, “The Viking Capture of York”**  
   https://learning.yorkarchaeology.co.uk/wp-content/uploads/2024/09/JORVIK-Fact-Sheet-The-Viking-Capture-of-York.pdf  
   支持 866/867 York 军事占领、Anglian Egbert 的代理统治及 876 年土地分配。其通俗概述不能替代人口比例证据。
4. **Hadley & Richards, “The winter camp of the Viking Great Army, AD 872–3, Torksey”**  
   https://www.cambridge.org/core/journals/antiquaries-journal/article/winter-camp-of-the-viking-great-army-ad-8723-torksey-lincolnshire/C54BB610EA9E0E567DC2C0622BA753EB  
   支持军队包含不同 warbands、营地人数可能以千计及背景多样性。Torksey 是 872–873 年的证据，不能把其规模或族群构成直接数值化到 York 867，更不能代表 Deira 全体居民。
5. **Fellows-Jensen, “Scandinavian Settlement in England: the evidence of place-names and personal names”**  
   https://www.persee.fr/doc/acsam_0000-0000_1989_act_2_1_1024  
   明确把定居研究的重要 Chronicle 线索放在 876、877、880，而非把行军路线等同定居。地名分布反映后期 Scandinavian 影响，不能证明 867 年 Deira 已经 Danish 化。
6. **British Library, Becky Lawton, “The Anglo-Saxon origins of the English counties”**  
   https://www.bl.uk/stories/blogs/posts/the-anglo-saxon-origins-of-the-english-counties  
   支持 Tribal Hidage 中的 East Angles、East Saxons、Cantware、South Saxons、West Saxons，以及 North Folk/South Folk 作为 East Angles 南北人民的语义。
7. **British Library, “The Tribal Hidage Online”**  
   https://blogs.bl.uk/digitisedmanuscripts/2012/04/the-tribal-hidage-online.html  
   说明该名单约在七至九世纪之间、用途和 Mercian/Northumbrian 来源均有争论。因此本模式只用较可靠的地方名，不把整张 Tribal Hidage 当成精确文化疆界。
8. **Cyril Hart, “The Tribal Hidage” (1971)**  
   https://www.cambridge.org/core/journals/transactions-of-the-royal-historical-society/article/abs/tribal-hidage/D245028C50C572F792D8D09D855826EB  
   支持 Wreocensæte 的九世纪 charter 证据及若干 Mercian 地方群体研究，同时提醒不要沿用机械“七国”模型。
9. **Steven Bassett, “The hidation of the Hwicce…”**  
   https://www.cambridge.org/core/journals/anglo-saxon-england/article/hidation-of-the-hwicce-investigating-its-halving-between-the-eighth-century-and-1086/717DA3169156F7627BC2B8A4F86541F7  
   支持 Hwicce 的地方行政史，也明确指出 Tribal Hidage 的 `Westerne = Magonsæte` 没有学术共识。
10. **Alan Vince, East Midlands Historic Environment Research Framework, “Anglo-Saxon Period”**  
    https://researchframeworks.org/emherf/regional-overview/anglo-saxon-period  
    支持 Lindsey、Middle Anglia、London/Middlesex 的关系，以及 overarching Anglian identity 与更小 folk identity 的叠层；也强调物质文化不能直接当族属标签。
11. **Stephen Rippon, “The Province of the Middle Saxons”**  
    https://www.cambridge.org/core/services/aop-cambridge-core/content/view/AA4958D2A970796E4CB86556E59D1B1A/stamped-9781787447813c6_p129-144_CBO.pdf/province_of_the_middle_saxons.pdf  
    支持八世纪 charters 中 Middle Saxon province 及它同 East Saxon/Mercia 的历史关系；不支持捏造精确族群边界。
12. **Fiona Edmonds, “The expansion of the kingdom of Strathclyde” (2015)，DOI `10.1111/emed.12087`**  
    https://research.lancaster-university.uk/en/publications/the-expansion-of-the-kingdom-of-strathclyde  
    公开稿：https://api.repository.cam.ac.uk/server/api/core/bitstreams/b580c610-873c-4748-a20d-d8ef4bb5d081/content  
    支持 Northumbrian 西扩影响不均、Brittonic 精英连续性的不确定，以及 Strathclyde 南扩主要是十世纪问题。因此 western Northumbria 选 unknown 而非伪造单一边界。
13. **John Hines 的 Surrey 考古研究与 John Blair 的区域历史导言**  
    https://www.surreyarchaeology.org.uk/sites/default/files/aspects07.pdf  
    区域历史导言：https://www.surreyarchaeology.org.uk/sites/default/files/EMedSryIntro_0.pdf  
    支持 Surrey 是有地方连续性的接触／边境区域，历经 Kentish、Mercian、West Saxon 权力，而非随宗主权瞬间换文化。
14. **John Baker、Jayne Carroll, “The Afterlives of Bede’s Tribal Names in English Place-Names”，载 Alexander Langlands、Ryan Lavelle 编 _The Land of the English Kin_（Brill, 2020）**  
    https://brill.com/display/book/edcoll/9789004421899/BP000008.xml?language=en  
    支持 Jutish 名称可能较早被 West Saxon 认同吸收、晚期证据的解释仍有争议。因此 Wight 保留 West Saxon 宽标签并标 uncertain。
15. **Simon den Hertog, “Subgrouping the Old English Dialects Using Historical Glottometry” (2024)**  
    https://brill.com/view/journals/abag/84/4/article-p470_2.xml?language=en  
    支持传统 Northumbrian/Mercian/Kentish/West Saxon 方言分组及 Anglian 等较粗组合，同时明确存世文本的教会、精英与社会偏差。方言不能机械充当 867 年逐格人口文化。
16. **Cornwall Heritage Trust, “The Age of the Saints”**  
    https://www.cornwallheritagetrust.org/timeline/the-age-of-the-saints  
    支持 Cornwall / West Welsh 称谓、Cornish 与 West Saxon 冲突及本地传统的延续；通俗时间线中的具体细节仍需同其他资料互证。
17. **John Fletcher, “Vikings and Settlers in Cornwall” (2023)**  
    https://sagy.vikingove.cz/en/vikings-and-settlers-in-cornwall  
    支持九世纪 Cornwall 与 Wessex 接触、边界可能移动和复杂身份。该文是带参考书目的研究性概述，本图没有把其重建边界当作确定测绘。
18. **Siarl Ferdinand, “A Brief History of the Cornish Language…”**  
    https://minds.wisconsin.edu/bitstream/handle/1793/94464/ferdinand_2_6.pdf  
    支持 Cornish 的 Brittonic 语言背景及九世纪前后材料的稀缺；语言史不能推出每格居民的语言或认同比例。

## 7. 生成、校验与测试

在项目根目录运行：

```powershell
& build/culture_build.ps1
```

脚本依次执行：

1. `python build/culture_generate.py --check`：不写文件地校验 sidecar 和 atlas。
2. 用项目相同的 Clang/MinGW 目标编译 `src/model.cpp`、`src/culture.cpp` 和 `build/culture_tests.cpp`；只读使用共享代码，不修改共享构建入口。
3. 运行 `build/culture_tests.exe maps/england-867.atlas maps/england-867.culture`。
4. 生成 `build/culture_assignments.tsv`（4987 行数据及表头）与 `build/culture_report.json`（计数、颜色、身份汇总）。

C++ 测试覆盖：总数与逐格恰好一次分配、颜色一致性、labels/legend/inspector 完整性、Deira 553 格与 Bernicia 328 格共用 Northumbrian、没有 Danish/Norse 主流身份、western Northumbria 350 格 unknown、North/South Folk 同色、Kent/Sussex/Essex 不被 West Saxon 换色、Hwicce 不被 Mercian core 换色、图外查询回退、缺 sidecar 及空 path 的 4987 格中性 unknown 回退。

本次实际结果：

```text
culture_generate: PASS: 4987 units; 4637 classified; 350 explicitly unknown; 15 displayed identities
culture_tests: PASS
  units=4987, classified=4637, unknown=350
  labels=15, legend_rows=15
  Deira=Northumbrian; western Northumbria=unknown
  missing sidecar: 4987 neutral unknown; empty path: PASS
Culture generation, data validation, and C++ tests all passed.
```

另以共享 `build.ps1` 的同一编译参数，将完整应用链接为独立输出 `build/culture_integration.exe`，结果为 `culture_integration: full application build PASS`；该检查未覆盖共享 `build/FieldAtlas.exe`。

## 8. 剩余局限

- 这不是基于 867 年调查问卷或人口普查的数据，也不存在可支持的每格文化百分比。任何视觉上的实心色块都是制图抽象。
- Atlas 区域来自既有重建，海岸线、政区界和省区界并不自动等于文化边界。本模式沿用它们是为了避免伪造更细边界，而非声称边界精确。
- 七至九世纪文献常由王国、教会或精英书写；平民、女性、奴隶、Brittonic-speaking 社区及流动人口很可能在资料中被低估。
- `approximate` 和 `uncertain` 是定性透明度标记，不是统计置信区间。相同等级也不表示各区域证据数量相同。
- Cornwall、Devon、western Northumbria、Surrey、upper Thames 和 Danish-occupied York 周边尤其不应被读成突变边界；应结合 inspector 和本文来源阅读。
- 后续若增加 unit override，必须有针对该 unit 的直接证据；不能仅为了让色块更“细腻”而增加分类。
