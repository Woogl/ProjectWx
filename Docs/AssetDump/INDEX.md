# AssetDump

에셋을 JSON으로 덤프한 텍스트 미러다. 에셋 내용 검색은 여기서 grep으로 한다. 갱신은 `/dump-assets`.

| 카테고리 | 파일 수 | 위치 |
|---|---|---|
| 전체 인덱스 | 315 에셋 | `index.json` |
| blueprints | 75 | `Blueprints/` |
| dataassets | 41 | `DataAssets/` |
| datatables | 7 | `DataTables/` |
| statetrees | 5 | `StateTrees/` |
| widgets | 34 | `Widgets/` |

인덱스에서 제외한 폴더(마켓플레이스·샘플·OFPA)와 에셋 수:

- `/Game/ARPG_Pack` — 962
- `/Game/EasyInputPrompts` — 191
- `/Game/Fab` — 90
- `/Game/Grz_HammerPack` — 306
- `/Game/Imortal_Loot_Drop_VFX` — 24
- `/Game/Mannequins` — 238
- `/Game/NiagaraExamples` — 671
- `/Game/Nodachi_AnimSet` — 85
- `/Game/ParagonShinbi` — 1268
- `/Game/UIMaterialLab` — 277
- `/Game/__ExternalActors__` — 312
- `/Game/__ExternalObjects__` — 9

본문 덤프는 DataTable·DataAsset·StateTree·BP/WBP만 한다. 몽타주·BT·레벨·아트 에셋은 인덱스에만 있다.

*문서 기준 커밋 `a7246818` · 생성일 2026-08-03 · 에셋 315개 — `/dump-assets`로 갱신*
