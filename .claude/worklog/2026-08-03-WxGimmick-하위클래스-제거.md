# WxGimmick 하위 클래스 제거

## 계획

### 목표

기믹의 실체는 이미 전부 `UWxGimmickStateTreeComponent`에 있고 `AWxGimmick`과 그 하위 넷은 메시 선언만 남은 껍데기다.
넷을 순수 BP로 재저작해 기믹용 C++ 액터 클래스를 전부 없앤다 — 기믹은 "아무 액터에 GimmickStateTree 컴포넌트를 붙인 것"이 된다.
하위가 0이 될 abstract 베이스도 죽은 코드이므로 함께 지운다(나중에 지우면 BP를 두 번 재부모해야 한다).

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Gimmick/WxDoor.h·.cpp` | 삭제 | 삭제 |
| `Plugins/WxWorld/.../Gimmick/WxElevator.h·.cpp` | 삭제 | 삭제 |
| `Plugins/WxWorld/.../Gimmick/WxTreasureChest.h·.cpp` | 삭제 | 삭제 |
| `Plugins/WxWorld/.../Gimmick/WxCheckPoint.h·.cpp` | 삭제 | 삭제 |
| `Plugins/WxWorld/.../Gimmick/WxGimmick.h·.cpp` | 삭제 | 삭제 |
| `Content/WorldObject/Gimmick/BP_Door·BP_Elevator·BP_TreasureChest·BP_CheckPoint` | AActor로 재부모 후 컴포넌트·값 재저작 | 수정 |
| `Content/WorldObject/Gimmick/ST_*` | 바인딩 재해석 확인, 끊긴 것만 재지정 | 수정 |
| `Content/__ExternalActors__/…` 배치 인스턴스 5개 | 소실된 per-instance 오버라이드 복구 | 수정 |
| `Plugins/WxWorld/README.md` 외 문서 | 삭제되는 헤더 주석의 기믹 동작 서술 이관 | 수정 |

### 접근 방식

- **전환이 성립하는 근거**: 네 클래스는 자기 파일 밖 C++ 참조가 0건이고, ST 에셋의 `ContextActorClass`가 이미 BP 클래스(`BP_Door_C` 등)를 가리킨다. 즉 바인딩은 C++ 클래스가 아니라 BP 클래스 기준이라 컴포넌트 이름만 그대로 살리면 재해석된다. `SaveId`도 오너 `ActorGuid`에서 매 등록마다 다시 심기므로 재부모로 깨지지 않는다.
- **순서**: 에셋 재저작 → C++ 삭제. 반대로 하면 BP가 부모를 잃고 깨진다.
- **덤프 후 복원**: 재부모하면 네이티브 컴포넌트와 그 위의 오버라이드(메시·트랜스폼·ST 지정·RewardRow·스플라인 포인트)가 사라진다. MCP로 BP CDO와 배치 인스턴스 5개의 값을 먼저 JSON으로 덤프하고, 재저작 후 그 값을 되돌린다.
- **이름 보존**: 컴포넌트 이름이 곧 ST 바인딩 키다. `SceneRoot`/`StateTree`/`ArrowComponent`와 각 기믹의 메시 이름을 철자 그대로 다시 만든다.
- **C++에만 있던 설정의 이관**: 액터 `Replicates`(베이스 생성자), 엘리베이터 스플라인 `Closed Loop = false`, 상자 `RewardRow`(BP 구조체 변수로 신설, `RowType` 메타 필터는 이관 불가).

---

## 완료

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Gimmick/WxGimmick·WxDoor·WxElevator·WxTreasureChest·WxCheckPoint.h/.cpp` | 삭제(10개). WxWorld 에 기믹용 C++ 액터 클래스가 없어졌다 | 삭제 |
| `Content/WorldObject/Gimmick/BP_{Door,Elevator,TreasureChest,CheckPoint}` | 순정 `AActor` 로 재부모하고 컴포넌트·값·Replicates 재저작. 상자는 `RewardRow` BP 변수 신설 | 수정 |
| `Content/__ExternalActors__/Maps/LV_DevCombat/…` 4개 | 액터 디스크립터의 네이티브 클래스를 `/Script/Engine.Actor` 로 갱신(재저장) | 수정 |
| `Plugins/WxWorld/README.md` | 삭제된 헤더 주석의 기믹 동작 서술을 「배치 기믹」 절로 이관, 타입 표·확장 규약 갱신 | 수정 |
| `Docs/Programmer/Gimmick_State_Authority.md`, `.claude/skills/design-checklist/SKILL.md` | 참조 표에서 `AWxGimmick` 항목 교체 | 수정 |

ST 에셋 4개는 손대지 않았다 — `ContextActorClass` 가 이미 BP 클래스였고, 바인딩이 이름으로 재해석돼 에디터 기동 시 넷 다 컴파일 성공했다.

검증은 세 단계로 했다. 재부모 전후 CDO·인스턴스 값을 JSON 으로 떠서 대조했고, 에디터를 새로 띄워 ST 컴파일과 액터 로드에 에러가 없음을 확인했으며, PIE 에서 넷 다 상호작용해 전이·연출·보상·저장과 재로드 복원까지 직접 확인했다.

### 구현·결정과 그 이유

- **베이스까지 지웠다**: 하위가 0이 된 abstract 베이스는 죽은 코드이고, 남겨두면 나중에 BP 를 두 번 재부모해야 한다. 이제 기믹은 "아무 액터에 GimmickStateTree 컴포넌트를 붙인 것"이라는 설계가 코드에도 그대로 성립한다.
- **덤프 → 재부모 → 복원 순서**: 재부모는 네이티브 컴포넌트와 그 위의 BP 오버라이드를 통째로 날린다. MCP 로 CDO·인스턴스 값을 먼저 JSON 으로 뜬 뒤 재저작 후 되돌리고, 마지막에 같은 방식으로 다시 떠서 대조했다(콜리전 1건을 이 대조에서 잡았다).
- **컴포넌트 이름 보존이 핵심**: ST 바인딩도, 배치 인스턴스의 오버라이드 재결합도 전부 이름으로 걸린다. 이름을 그대로 둔 덕에 상자 인스턴스의 `RewardRow`(Gold100) 가 네이티브 프로퍼티 → BP 변수 전환을 넘어 그대로 살아남았다.
- **스플라인 reparam 테이블을 직접 썼다**: 프로퍼티 직접 대입은 `UpdateSpline` 을 타지 않아 스플라인 길이가 기본값(100)에 머문다. 이동 태스크가 거리 기반이라 그대로 두면 층 이동이 어긋난다.
- **상자 메시 콜리전 복원**: BP 로 새로 붙인 SkeletalMeshComponent 의 기본은 NoCollision 이다. 이 메시가 곧 상호작용 영역이므로 원래 값(BlockAll/QueryAndPhysics)을 명시 복원했다.

### 계획 대비 달라진 점

- **World Partition 액터 디스크립터가 막았다(계획에 없던 문제)**: 재부모 후 재시작하니 배치 기믹 넷이 통째로 월드에서 빠지고 `Invalid actor native class` 경고가 떴다. 액터 패키지의 디스크립터가 옛 네이티브 클래스 경로(`/Script/WxWorld.WxDoor` 등)를 들고 있는데 그 클래스가 사라졌기 때문이다. 디스크립터는 액터를 로드해 재저장해야 갱신되는데, 로드 자체가 막히는 순환이었다.
  - 해결: 빈 껍데기 클래스 다섯 개를 임시로 되살려 빌드 → 액터가 로드되면 재저장 → 디스크립터가 `/Script/Engine.Actor` 로 갱신 → 스텁 삭제 후 재빌드. 액터를 새로 배치하는 대안도 있었지만 ActorGuid(=SaveId)가 바뀌어 세이브가 끊기므로 택하지 않았다.
- **오래된 분석 문서는 손대지 않았다**: `Interaction_System.md`·`Spawner_Enemy_Lifecycle.md` 등은 이번 변경 이전부터 이미 사라진 함수·경로를 서술하는 시점 스냅샷이라, 클래스 이름만 고치면 오히려 최신 문서처럼 보인다. `/analyze-system` 재생성 대상으로 남긴다.

### 후속 과제

- `Content/__ExternalActors__/Level/LV_Dungeon1/` 은 소유 `.umap` 이 없는 고아 데이터다(엘리베이터 1개 포함). 이번 작업 범위 밖이라 두었으나 정리 대상.
- 상자 `RewardRow` 는 BP 변수라 C++ 의 `RowType` 메타 필터가 없다 — 디테일 패널에서 아무 DataTable 이나 고를 수 있다.
