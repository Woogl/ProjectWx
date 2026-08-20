# 배타 액션을 태그 배선에서 ActivationGroup으로 전환

## 계획

### 목표
`Trait.Ability.Exclusive` 하나가 세 컨테이너(표식·잠금·취소)에 다른 뜻으로 들어가고 조합을 14개 어빌리티가 각자 골라 쓰는 규칙을, Lyra의 ActivationGroup 방식으로 대체한다. 어빌리티는 그룹 한 줄만 선언하고 판정은 ASC 한 곳이 맡으며, BP가 AssetTags를 편집하면 마커가 날아가는 함정도 함께 없앤다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxAbilityBase.h/.cpp` | 그룹 enum·필드 2개, 발동 게이트, 활성화 리셋+취소, 후딜 전이 | 수정 |
| `WxAbilitySystemComponent.h/.cpp` | `IsActivationGroupBlocked()`, `CancelActivationGroupAbilities()` | 수정 |
| 어빌리티 12종 생성자 (WxCombat 10 + WxGame 2) | 마커 관련 줄 삭제, 그룹 선언 한 줄 | 수정 |
| `WxAbility_Attack.h/.cpp` | 취소 대상 탐지와 차단 우회 분기 삭제 | 삭제 |
| `WxCharacterBase.cpp` | 점프 게이트를 그룹 판정 질의로 | 수정 |
| `WxGameplayTags.h` | 주석표 삭제, 태그 뜻을 "밖에서 거는 액션 잠금" 한 가지로 | 수정 |

### 접근 방식
- **값 집합**: Lyra의 `Independent / Exclusive_Replaceable / Exclusive_Blocking`에 `Reaction`을 더한 넷. 반응(피격·그로기·사망·처형)은 본동작 잠금을 뚫고 들어와야 하는데 Lyra 세 값으로는 표현이 안 된다.
- **선언과 상태의 통합**: 액션은 `Exclusive_Blocking`으로 선언하고, 후딜 진입 노티파이가 그룹을 `Exclusive_Replaceable`로 전이한다. 별도 후딜 플래그 없이 그룹 값 자체가 상태다. Lyra와 달리 다음 활성화 시작에서 CDO 선언값으로 되돌려, 재사용 인스턴스로 후딜 상태가 새는 것을 막는다.
- **판정**: 배타 어빌리티는 활성 인스턴스 중 본동작(Blocking·Reaction)이 있거나 밖에서 태그 잠금이 걸려 있으면 발동이 막힌다. 발동에 성공하면 후딜에 든 것을 끊고, 취소 성향을 켠 반응은 본동작까지 끊는다. ASC는 카운터 없이 활성 스펙에서 그때그때 파생한다.
- **태그의 잔존 역할**: 기믹 연출이 밖에서 거는 액션 잠금 한 가지만 남기고, WxWorld 태스크는 손대지 않아 플러그인 간 참조를 만들지 않는다. 피격의 종류별 취소(적 패턴 보존)도 기존 태그 경로를 유지한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAbilityBase.h/.cpp` | 그룹 enum·필드 2개·게이트 추가, 활성화 시 그룹 리셋과 후딜 취소, 후딜 전이 | 수정 |
| `WxAbilitySystemComponent.h/.cpp` | `IsActivationGroupBlocked()`, `CancelActivationGroupAbilities()` | 수정 |
| 어빌리티 12종 생성자 (WxCombat 10 + WxGame 2) | 마커 태그 줄 삭제, 그룹 선언 한 줄로 대체 | 수정 |
| `WxAbility_Attack.h/.cpp` | 취소 대상 탐지 함수와 차단 우회 분기 삭제 | 삭제 |
| `WxCharacterBase.cpp` | 점프 게이트를 태그 차단 조회에서 그룹 판정 질의로 | 수정 |
| `WxGameplayTags.h/.cpp` | `Trait.Ability.Exclusive` 삭제(주석표 포함) | 삭제 |
| `WxStateTreeTask_MoveInteractorToTarget.h/.cpp` | 기믹 연출의 어빌리티 태그 잠금 경로 제거(이동 입력 잠금만 유지) | 삭제 |

### 구현·결정과 그 이유
- **값 집합은 Lyra 셋 + Reaction**: 판정 함수 이름·위치·호출 지점(베이스 `CanActivateAbility`)까지 Lyra와 맞추되, 본동작 잠금을 뚫어야 하는 반응 부류는 Lyra 세 값으로 표현이 안 돼 네 번째 값으로 얹었다.
- **그룹 값이 곧 후딜 상태**: 별도 플래그 없이 후딜 진입이 그룹을 `Exclusive_Replaceable`로 전이한다. Lyra는 전이를 되돌리지 않아 재사용 인스턴스로 상태가 새므로, 활성화 시작에서 CDO 선언값으로 되돌리는 리셋 한 줄을 덧댔다.
- **카운터 대신 활성 스펙 파생**: 증감 짝 맞춤과 Notify 훅이 필요 없고 비정상 종료에도 새지 않는다. 판정 대상이 스펙 십수 개라 비용은 무시할 수준.
- **공격의 차단 우회 분기 삭제 가능 확인**: 회피 카운터의 진입 창이 완벽 회피 몽타주의 StartRecovery 노티파이와 일치해, "Blocking을 뚫는" 우회 없이 "후딜(Replaceable)을 끊는" 정상 경로로 성립한다.
- **태그 완전 삭제**: 계획은 기믹 연출의 외부 잠금 한 가지 뜻만 남기는 것이었으나, 추가 지시로 태그 자체를 삭제했다. 그에 따라 기믹 연출 중 어빌리티·점프 잠금 경로도 함께 사라졌고(이동 입력 잠금 SetIgnoreMoveInput만 유지), 재도입하려면 별도 신호가 필요하다. 피격의 종류별 취소(적 패턴 보존)는 `Ability.Attack`·`Ability.Skill` 태그라 무관하게 유지.
- **필드는 UPROPERTY 비노출**: 배타 정책은 코드 소관이고, 노출하지 않으면 BP 델타 직렬화 대상도 아니다.

### 계획 대비 달라진 점
- 승인 후 추가 지시로 `Trait.Ability.Exclusive`를 완전 삭제했다. 계획의 "외부 잠금 뜻으로 존치"와 후속의 리네임 건은 폐기되고, 기믹 연출 중 어빌리티·점프 잠금이 사라지는 동작 변화를 수반한다.
- Lyra식 `Ability.ActivateFail.ActivationGroup` 실패 사유 태그는 추가했다가 지시로 되뺐다 — 디버깅이 필요해지면 다시 넣는다. 그룹 게이트는 사유 태그 없이 false만 반환한다.

### 후속 과제
- WxEditor(Development) 빌드 성공. PIE 실동작(공격 중 잠금·후딜 캔슬·반응 취소·가드 페이즈·회피 카운터·상호작용 표시·점프 게이트)은 미검증.
- 에디터에서 GA_Skill/Pattern/Attack의 AssetTags에 남은 마커와 GA_Attack_Heavy의 CancelAbilitiesWithTag 잔재 비우기(별건). 태그가 네이티브에서 사라져 해당 에셋 로드 시 미등록 태그 경고가 날 수 있으므로 우선순위 상향.
- 기믹 연출 중 어빌리티·점프 잠금이 필요해지면 별도 신호(예: State 계열 태그)로 재설계(별건).
