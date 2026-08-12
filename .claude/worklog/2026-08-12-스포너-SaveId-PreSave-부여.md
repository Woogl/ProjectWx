# 스포너 SaveId 부여를 PreSave 시점으로 이관

## 계획

### 목표

`AWxSpawner::SaveId`(WxSave 슬롯 레코드 키)가 에디터 복제·붙여넣기에서 원본과 겹치는 것을 막는다. 부여 시점을 생성 훅에서 직렬화 직전(`PreSave`)으로 옮겨, 에셋에 실리는 값이 언제나 그 액터의 `ActorGuid` 와 일치하게 한다.

문제의 근거: 에디터의 액터 복제(Ctrl+W·Alt-드래그)와 붙여넣기는 `StaticDuplicateObject` 가 아니라 T3D 복사-붙여넣기 경로다(`UUnrealEdEngine::DuplicateActors`). 순서가 `SpawnActor`(새 `ActorGuid` → `PostActorCreated` 가 `SaveId` 에 기록) → `PreEditChange` → `ImportObjectProperties`(**원본 T3D 의 `SaveId` 로 덮어씀**) → `PostEditImport` → `PostEditChange` 라, `PostDuplicate` 는 아예 불리지 않고 교정 기회도 없다. `ActorGuid` 는 `TextExportTransient, NonPIEDuplicateTransient` 라 T3D 에 실리지 않아 새 값을 유지하므로, 두 스포너의 `ActorGuid` 는 다른데 `SaveId` 만 같아진다. 그러면 `WxSave` 가 한 레코드를 공유해(`WxSaveWorldSubsystem.cpp:286`, `:348`) 복원 시 양쪽에 같은 `Transform` 을 적용하고 `bIsKilled` 도 물려준다 — 명시 세이브뿐 아니라 WP 셀 스트리밍 아웃/인마다 반복된다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp` | `PostActorCreated`·`PostDuplicate` 정의 삭제, 주석 블록은 `PreSave` 쪽으로 이전. `PreSave` 정의 추가(`Super` → `IsProceduralSave()` 반환 → `SaveId = GetActorGuid()`). `UObject/ObjectSaveContext.h` include 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` | `#if WITH_EDITOR` 섹션의 `PostActorCreated`·`PostDuplicate` 선언을 `PreSave` 선언으로 교체. `SaveId` 필드 주석 정정 | 수정 |

### 접근 방식

- **부여 시점을 직렬화 직전으로**: `SaveId` 의 존재 이유가 "에셋에 직렬화되어 런타임이 읽는 키"이므로, 값이 확정돼야 하는 시점은 생성 순간이 아니라 저장 직전이다. `PreSave` 에서 `ActorGuid` 를 복사하면 생성 경로(신규 배치·`StaticDuplicateObject`·T3D 붙여넣기)가 무엇이든 무관해진다 — import 가 무엇으로 덮었든 저장 직전 이 액터의 값으로 재확정되기 때문이다. `AActor::PreSave(FObjectPreSaveContext)` 는 오버라이드 가능하고(`Actor.h:2381`), WP 외부 액터 패키지도 액터별로 이 훅을 탄다.
- **절차적 저장 제외**: 쿠킹·EditorDomain 은 `IsProceduralSave()` 로 걸러 건드리지 않는다. 사용자 편집이 개입할 수 없는 저장이고, 값은 맵 저장 때 이미 패키지에 실려 있다.
- **`Modify()` 미호출**: 이미 저장 중이라 트랜잭션에 남길 이유가 없고, 오히려 저장 직후 패키지가 dirty 로 남는다.
- **생성 훅 제거**: 부여가 한 경로로 모이므로 `PostActorCreated`·`PostDuplicate` 를 뺀다. 이로써 두 번째 구멍 — `PostActorCreated` 가 에디터 월드를 가리지 않아 에디터 빌드(PIE 포함)에서 **런타임 스폰된** 스포너에도 세션 한정 GUID 를 심던 것 — 도 함께 닫힌다. `PreSave` 는 게임 월드에서 돌지 않는다.
- **알려진 한계(현행 대비 악화 없음)**: 복제 직후 맵을 저장하지 않은 채 PIE 를 돌리면 사본은 여전히 원본의 `SaveId` 를 들고 있다. 지금도 T3D import 가 덮은 뒤라 동일하며, "저장 전이거나 런타임 스폰된 것은 저장/복원에서 제외된다"는 README 계약과도 어긋나지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp` | `PostActorCreated`·`PostDuplicate` 삭제, `PreSave` 추가. `UObject/ObjectSaveContext.h` include | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` | 두 생성 훅 선언을 `PreSave` 선언으로 교체, `SaveId` 주석 정정 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp` | `OnRegister`(SaveId compare-and-fix) 삭제, `PreSave` 추가. include 동일 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` | `OnRegister` 선언을 `PreSave` 선언으로 교체, `SaveId` 주석 정정 | 수정 |

### 구현·결정과 그 이유
- **부여 시점을 직렬화 직전으로**: `SaveId` 는 "에셋에 실려 런타임이 읽는 키"이므로 확정 시점은 생성 순간이 아니라 저장 직전이다. 그러면 생성 경로(신규 배치·`StaticDuplicateObject`·T3D 붙여넣기)를 하나도 따질 필요가 없다 — import 가 무엇으로 덮었든 저장 직전에 이 액터 값으로 재확정된다.
- **`IsProceduralSave()` 로 절차적 저장 제외**: 쿠킹·EditorDomain 은 사용자 편집이 개입할 수 없는 저장이라 값이 이미 확정돼 있다. 저장 중 오브젝트 변형은 결정성 측면에서도 피하는 편이 낫다.
- **`Modify()` 미호출**: 이미 저장 중이라 트랜잭션에 남길 이유가 없고, 부르면 저장 직후 패키지가 dirty 로 남는다. 기존 `OnRegister` 경로가 `Modify()` 를 쓰던 것은 저장과 무관한 시점이라 필요했던 것이다.
- **컴포넌트는 오너 없으면 반환**: 오너 없이 저장되는 것은 블루프린트 클래스의 컴포넌트 템플릿이다. 배치 인스턴스가 아니라 키를 심을 대상이 아니고, 무조건 대입하면 무효 GUID 로 기존 값을 지운다.
- **생성·등록 훅 제거**: 부여가 한 경로로 모였다. 곁가지로 `PostActorCreated` 가 에디터 월드를 가리지 않아 에디터 빌드(PIE 포함)에서 런타임 스폰 스포너에도 세션 한정 GUID 를 심던 문제도 닫혔다 — `PreSave` 는 게임 월드에서 돌지 않는다.

### 계획 대비 달라진 점
- `UWxGimmickStateTreeComponent` 를 같은 방식으로 함께 고쳤다(계획에선 후속 과제로 뺐던 항목). 기존 `OnRegister` compare-and-fix 도 동작은 했지만, 두 곳의 부여 방식이 갈리는 것보다 한 패턴으로 모으는 편이 낫다는 판단.

### 후속 과제
- 에디터 실기 확인 미수행: 스포너/기믹 배치 → Ctrl+W 복제 → 맵 저장 후 두 `SaveId` 가 갈리는지, 셀 스트리밍 아웃/인에서 서로 영향을 주지 않는지.
- 이미 `SaveId` 가 겹친 채 저장된 기존 배치가 있다면 그 액터를 한 번 더 저장해야 교정된다(다음 맵 저장 때 자동 흡수).
- `Docs/Programmer/module_review_WxWorld.md` 는 갱신하지 않았다 — 다음 `/module-review WxWorld` 실행 시 반영된다.
