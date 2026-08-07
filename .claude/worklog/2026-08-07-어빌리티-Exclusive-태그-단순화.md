# 어빌리티 상호 캔슬을 Exclusive 태그로 단순화

## 계획

### 목표

어빌리티들이 서로를 이름으로 지목해 캔슬하던 구조(HitReact·Finisher가 Attack·Skill을, UseItem이 Sprint를 지목)를, 「액션 슬롯을 점유한다」는 뜻의 마커 태그 하나로 대체한다. 새 액션 어빌리티가 마커만 달면 기존 캔슬 규칙에 자동으로 편입되고, 캔슬 목록을 찾아 고치다 빠뜨리는 조용한 누락이 사라진다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/.../WxGameplayTags.h/.cpp` | `Ability_Exclusive`("Ability.Exclusive") 선언·정의, 루트와의 2단 의미 주석 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Attack.cpp` | AssetTags에 마커 추가 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Skill.cpp` | AssetTags에 마커 지정(슬롯 태그는 BP 몫) | 수정 |
| `Plugins/WxCombat/.../WxAbility_Dodge.cpp` | AssetTags에 마커 추가, 캔슬을 마커로 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Guard.cpp` | 〃 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Ultimate.cpp` | 〃 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Sprint.cpp` | AssetTags에 마커 추가 | 수정 |
| `Plugins/WxCombat/.../WxAbility_HitReact.cpp/.h` | 캔슬을 마커 하나로, 주석 갱신 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Finisher.cpp` | 〃 | 수정 |
| `Source/WxGame/.../WxAbility_UseItem.cpp` | AssetTags에 마커 추가, 캔슬을 마커로 | 수정 |
| `Source/WxGame/.../WxAbility_Interact.cpp` | AssetTags에 마커 추가 | 수정 |
| `Content/AbilitySystem/Ability/GA_Skill_1~4` | AbilityTags에 마커 추가(BP가 값을 소유해 C++ 전파 불가) | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 캔슬·차단 서술을 새 규칙에 맞춤 | 수정 |

### 접근 방식

- **태그를 2단으로 나눈다**: 루트 `Ability` = 「모든 어빌리티」(차단과 사망·그로기의 캔슬이 쓴다), 신규 `Ability.Exclusive` = 「액션 슬롯 점유」(캔슬이 쓴다). GAS는 태그를 보유한 쪽만 부모로 확장하므로 루트는 마커 보유분까지 전부 잡고, 마커는 마커 보유분만 잡는다.
- **마커는 플레이어 액션에만 준다**: Attack·Skill·Dodge·Guard·Ultimate·Sprint·UseItem·Interact. 적 패턴은 마커가 없어 「피격이 패턴을 끊지 않는다」는 현행 설계가 그대로 유지되고, 그로기·사망은 루트로 캔슬하므로 패턴을 계속 끊는다.
- **차단은 손대지 않는다**: 모든 `BlockAbilitiesWithTag`가 루트를 유지하므로 점프 게이트·기믹 연출 차단·피격 중 패턴 차단의 동작이 그대로다.
- **BP 델타 주의**: `GA_Skill_1~4`는 AbilityTags를 BP가 소유(델타)해 부모 CDO의 마커가 전파되지 않는다. 에셋을 직접 고치고, 덤프로 전 어빌리티의 실제 태그를 재확인한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/.../WxGameplayTags.h/.cpp` | `Ability_Exclusive` 신설, 루트와의 2단 의미 주석 | 수정 |
| `.../WxAbility_Attack.cpp`, `_Sprint.cpp` | AssetTags에 마커 추가 | 수정 |
| `.../WxAbility_Skill.cpp` | AssetTags에 마커 지정(슬롯 태그는 BP), 전파 한계 주석 | 수정 |
| `.../WxAbility_Dodge.cpp`, `_Guard.cpp`, `_Ultimate.cpp` | AssetTags에 마커 추가 + 캔슬을 마커로 | 수정 |
| `.../WxAbility_HitReact.cpp/.h`, `_Finisher.cpp` | Attack·Skill 지목을 마커 하나로, 주석 갱신 | 수정 |
| `.../WxAbility_Groggy.cpp`, `_Death.cpp` | 루트 캔슬을 유지하는 이유 주석 | 수정 |
| `Source/WxGame/.../WxAbility_UseItem.cpp` | AssetTags에 마커 추가 + Sprint 지목을 마커로 | 수정 |
| `Source/WxGame/.../WxAbility_Interact.cpp` | AssetTags에 마커 추가 | 수정 |
| `Content/AbilitySystem/Ability/GA_Skill_1~4` | AbilityTags에 마커 추가 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 캔슬·차단 서술을 새 규칙으로 | 수정 |
| `.claude/asset_dump/**` | 전체 재덤프(160 files, errors=0) | 수정 |

### 구현·결정과 그 이유
- **마커는 AssetTags에 둔다**: 엔진은 캔슬·차단 대상 판정에 CDO의 AssetTags만 본다(`CancelAbilities`, `DoesAbilitySatisfyTagRequirements`). 다른 태그 컨테이너로는 「이 어빌리티를 끊어라」 판정이 성립하지 않는다. 식별 태그를 앞에 두어 선두 태그를 읽는 소비자도 안전하고, 나머지 소비자는 전부 부분 매칭(HasAll·HasTag)이라 태그가 하나 늘어도 영향이 없음을 확인했다.
- **캔슬 축만 옮기고 차단은 루트 유지**: 차단까지 옮기면 점프 게이트·기믹 연출 차단의 인자와 피격 중 적 패턴 차단이 함께 바뀐다. 파장이 다른 별건이라 후속으로 미뤘다.
- **그로기·사망만 루트 캔슬 유지**: 마커가 없는 적 패턴까지 멈춰야 하는 유일한 두 경우다.
- **에셋은 GA_Skill_1~4만**: 이 넷만 AbilityTags를 BP 델타로 소유해 부모 CDO의 마커가 덮인다. 나머지는 `.uasset` 이름 테이블에 해당 프로퍼티가 없어(=델타 없음) 상속에 맡겼고, 실제로 상속됐음을 라이브 CDO 조회로 확인했다.

### 계획 대비 달라진 점
- 검증에 덤프뿐 아니라 라이브 CDO 조회(unreal-mcp)를 더했다. 어느 어빌리티가 마커를 실제로 갖는지 즉시 확정할 수 있어 델타 색출이 확실해진다.

### 후속 과제
- **2단계**: 차단 축도 마커로 이전 + 반응 어빌리티(HitReact·Groggy·Death·Finisher·LockOn)에 식별 태그 부여 + 패턴 차단 명시화. LockOn의 「AssetTag 비워두기」 예외가 사라진다.
- **3단계**: `WxCharacterMovementComponent`의 앉기 예외(어빌리티 스펙 순회 + `IsA<UWxAbility_LockOn>`)를 차단 여부 판정으로 교체, 스프린트는 자기 발동 시 `UnCrouch()`.
- PIE 실동작 확인 미실시(빌드·CDO·덤프 검증까지 완료).
