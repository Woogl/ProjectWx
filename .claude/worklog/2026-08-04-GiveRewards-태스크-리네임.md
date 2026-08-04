# StateTree 태스크 GrantReward → GiveRewards 리네임

## 계획

### 목표
StateTree 태스크 `FWxStateTreeTask_GrantReward`의 이름을 `GiveRewards`로 바꾼다. 범위는 ST 태스크 하나이며, 같은 이름의 BFL `UWxRewardLibrary::GrantReward`는 건드리지 않는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h` | 태스크·인스턴스 데이터 구조체 개명, DisplayName, 구획 주석 | 수정 |
| `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp` | 정의부 개명, `GetDescription` 표시 문자열 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]` 섹션 신설, StructRedirects 2건 | 수정 |
| `Docs/Programmer/Reward_Grant_Flow.md` | 참조표의 태스크 행 | 수정 |
| `Docs/Programmer/Gimmick_State_Authority.md` | 크로스모듈 노드 예시 행 | 수정 |
| `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` | 노드를 지칭하는 주석 | 수정 |

### 접근 방식
- **리다이렉트 필수**: 이 구조체는 ST 에셋 2개(`ST_TreasureChest`, `ST_Quest_Main1`)가 경로 문자열로 직접 참조한다. 리다이렉트 없이 개명하면 로드 시 노드를 못 찾아 태스크가 조용히 사라지고 상자·퀘스트 보상 지급이 실종된다. 노드와 인스턴스 데이터가 각각 구조체 경로로 직렬화되므로 두 구조체 모두 등재한다.
- **BFL 불변**: 요청 범위가 태스크로 한정되었다. "GiveRewards 태스크가 GrantReward 라이브러리를 호출"하는 어휘 비대칭은 남는다.
- **헤드리스 덤프로 검증**: 리다이렉트가 실제로 먹었는지는 에디터를 열지 않고 덤프 커맨드릿으로 에셋을 재로드해 노드 경로를 확인한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h` | 태스크·인스턴스 데이터 구조체 개명, `DisplayName` → "Give Rewards", 구획 주석 | 수정 |
| `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp` | 생성자·`EnterState`·`GetDescription` 정의 개명, 표시 문자열 "Give Rewards ({0})" | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]` StructRedirects 2건 임시 추가 후 제거 | 없음(최종) |
| `Content/WorldObject/Gimmick/ST_TreasureChest.uasset`, `Content/Quest/ST_Quest_Main1.uasset` | 새 구조체 이름으로 재저장 | 수정 |
| `Docs/Programmer/Reward_Grant_Flow.md` | 상호작용 서술·mermaid 노드·참조표의 태스크 표기 | 수정 |
| `Docs/Programmer/Gimmick_State_Authority.md` | 크로스모듈 노드 예시 행 | 수정 |
| `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` | 0번 컨트롤러 전제를 설명하며 이 노드를 지칭하던 주석 | 수정 |
| `.claude/asset_dump/StateTrees/ST_TreasureChest.json`, `ST_Quest_Main1.json` | 검증용 재덤프 결과 반영 | 수정 |

### 구현·결정과 그 이유
- **리다이렉트 2건**: ST 에셋은 태스크 노드와 인스턴스 데이터를 별개 구조체 경로로 직렬화한다. 노드만 리다이렉트하면 태스크는 살아나도 보상 Row·오프셋·발사 속도가 통째로 날아간다.
- **리다이렉트는 일회용 다리**: 계획은 항목을 존치하는 쪽이었으나, 리다이렉트로 두 에셋을 연 뒤 새 이름으로 재저장해 이름을 구워 넣고 `[CoreRedirects]`는 제거했다. 에셋에 새 이름이 직렬화된 이상 항목은 죽은 설정이고, 남겨 두면 옛 이름이 영구히 유효한 것처럼 보인다. 재저장으로 이름이 바뀐 것은 `.uasset` 이름 테이블에서 직접 확인했다.
- **BFL 불변**: 요청 범위가 ST 태스크 하나였다. 결과적으로 GiveRewards 태스크가 GrantReward 라이브러리를 호출하는 어휘 비대칭이 남는다.
- **헤드리스 덤프로 검증**: 리다이렉트가 실제로 먹었는지는 컴파일이 답해 주지 않는다. 두 에셋을 다시 덤프해 노드 경로가 새 이름으로 해석되고 태스크가 살아 있는지 직접 확인했다.

### 계획 대비 달라진 점
- **리다이렉트 존치 → 제거**: 두 에셋을 재저장해 새 이름을 구워 넣었으므로 `[CoreRedirects]` 항목을 남기지 않았다. 커밋 시점의 `DefaultEngine.ini`는 개명 전과 동일하다.

### 후속 과제
- 없음. 빌드 성공(WxInventory·WxQuest 재컴파일). 재덤프 결과 두 에셋 모두 `WxStateTreeTask_GiveRewards`로 해석되고, 퀘스트 쪽 설명이 `Give Rewards (Gold1000)`으로 나와 인스턴스 데이터(보상 Row)까지 보존됐음을 확인했다.
