# 삭제된 Trait.* · Gimmick.* 어휘의 잔재 정리

## 계획

### 목표
`module_review_WxCore.md` 1·3·4 항목. 이미 사라진 이름(`Trait.*` 태그 네임스페이스, `IsInteractionEnabled`, `Gimmick.*` 태그)을 아직 가리키는 주석·README·태그 리다이렉트를 걷어내 태그 어휘 원본과 계약 헤더가 현재 사실만 말하게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | Ability 섹션 머리주석의 "분류 마커는 Trait.*에 있다" → 성질은 태그가 아닌 `EWxAbilityActivationGroup`(WxCombat) 선언이라는 사실로 정정 | 수정 |
| `Plugins/WxCore/README.md` | `Ability.*` 설명의 "(성질 분류는 별도 `Trait.*`)" 정정 | 수정 |
| `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` | `SetInteractionEnabled` 주석의 `IsInteractionEnabled` → `CanInteract` | 수정 |
| `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp` | 같은 잔재 이름 정정 | 수정 |
| `Config/DefaultGameplayTags.ini` | 대상 없는 `Gimmick.* → Device.*` 리다이렉트 9줄과 설명 주석 삭제 | 수정 |

### 접근 방식
- **삭제 전 전수 확인**: `Gimmick.<태그>` 문자열은 `Content/`·`Plugins/*/Content/` 어디에도 없다(폴더명 `WorldObject/Gimmick/` 히트만 존재). `Trait.` 을 선언하는 코드·ini 도 없다. 즉 리다이렉트는 대상이 0건이라 삭제해도 이관 경로가 끊길 에셋이 없다.
- **섹션 헤더는 남긴다**: `[/Script/GameplayTags.GameplayTagsSettings]` 만 남겨 에디터가 태그 설정을 쓸 자리를 유지한다.
- **GA 에셋 3개(`GA_Skill_2/3/4`)의 `Trait.Ability.Exclusive` 잔재는 별도**: 바이너리 패키지라 에디터가 필요하다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | Ability 머리주석: "분류 마커는 Trait.*" → "성질(배타 그룹 등)은 태그가 아니라 `EWxAbilityActivationGroup`(WxCombat)" | 수정 |
| `Plugins/WxCore/README.md` | `Ability.*` 설명의 `Trait.*` 잔재를 같은 문구로 정정 | 수정 |
| `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` | `SetInteractionEnabled` 주석의 `IsInteractionEnabled` → `CanInteract` | 수정 |
| `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp` | 같은 잔재 이름 정정 | 수정 |
| `Config/DefaultGameplayTags.ini` | `Gimmick.* → Device.*` 리다이렉트 9줄과 설명 주석 삭제(섹션 헤더만 남김) | 수정 |
| `Content/AbilitySystem/Ability/GA_Skill_2/3/4.uasset` | AssetTags 의 `Trait.Ability.Exclusive` 잔재 제거 후 재저장 | 수정 |
| `.claude/CLAUDE.md` | (별건) 코딩 규칙 6 에 템플릿·StateTree `GetInstanceDataType()` 예외 명문화 | 수정 |
| `Plugins/WxInventory/.../WxItemInstance.h`, `Plugins/WxUI/.../WxPrimaryGameLayout.h` | (별건) 누락돼 있던 규칙 6 예외 사유 주석 보완 | 수정 |

### 구현·결정과 그 이유
- **에셋 잔재는 빈 목적지 리다이렉트 + 커맨드릿으로 걷었다**: 엔진은 미등록 태그를 로드 시 경고만 하고 컨테이너에 남기지만(`GameplayTagsManager.cpp:1063-1088`), 리다이렉트가 걸린 태그는 목적지가 무효면 되돌려 넣지 않고 제거한다(`GameplayTagRedirectors.cpp:149` → `NamesToRemove`). 그래서 `NewTagName=""` 리다이렉트를 임시로 걸고 `ResavePackages` 를 3개 패키지에만 돌린 뒤 리다이렉트를 다시 지웠다. 에디터를 열어 손으로 지우는 것과 결과가 같고 재현 가능하다.
- **결과를 문자열 단위로 검증했다**: 재저장 전후 패키지의 인쇄 가능 문자열 집합을 비교해 세 에셋 모두 차이가 `Trait.Ability.Exclusive` 제거 한 건뿐임을 확인했다(추가·소실 없음).
- **리다이렉트 삭제 전 대상 0건을 확인했다**: `Gimmick.<태그>` 문자열이 어느 패키지에도 없다. 폴더명 `Content/WorldObject/Gimmick/` 히트는 태그가 아니다.

### 계획 대비 달라진 점
- 에셋 3개를 "에디터 필요, 별도 처리"로 남기려 했으나, 커맨드릿 경로로 이번에 함께 끝냈다(사용자 선택).

### 후속 과제
- 없음. WxEditor(Development) 빌드 `Result: Succeeded`.
