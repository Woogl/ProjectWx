---
title: "쿨다운 GE를 하나로: 어빌리티 CooldownTags와 차례 회복"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, combat, ui, decision]
summary: "쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 없애고 공용 UWxEffect_Cooldown 하나로 합쳤다. 쿨다운 식별자는 어빌리티의 CooldownTags이고 스펙의 DynamicGrantedTags로 붙인다. 쌓지 않고 충전 하나당 GE 하나를 걸며, ApplyCooldown이 같은 태그의 남은 시간 뒤로 줄을 세운 지속시간을 SetByCaller로 넘겨 충전이 차례로 돌아온다."
---

# 쿨다운 GE를 하나로

2026-09-25 미커밋 작업 트리(HEAD `38d4dde08`). 에디터 빌드, 데이터 검증 커맨드릿, 임시 자동화 테스트로 확인했고, 아래 수정 뒤 사용자가 인게임에서 확인했다. 작업 기록은 [쿨다운 GE 통합](../../../.agents/workflow/tasks/cooldown-unification.md)이다.

## 결정

- 발단(사용자): BP 커스텀·오버라이드 때문에 AI가 구조를 헷갈릴 수 있다. `Skill.1`·`Ultimate.1` 같은 번호 태그가 없어지길 원한다.
- 사용자 결정: `Ability.Pattern.N`은 BT 재정비에 필요할 수 있어 유지한다. `Ability.Skill.N` 제거는 BT 재정비 때 한다. 스킬·쿨다운의 파생 클래스는 최소화한다.
- 사용자: "쿨다운 태그는 따로 두더라도, 쿨다운 클래스는 통합할 수 있지 않나요?" → 쿨다운 GE 클래스를 하나로 합치고 태그는 어빌리티에 둔다.
- 필드는 `FGameplayTagContainer CooldownTags`다. `FGameplayTag CooldownGroup` 단일 태그안을 검토했지만 사용자가 컨테이너를 유지하기로 했다(엔진 `GetCooldownTags()`가 컨테이너 포인터를 돌려준다).
- 사용자 지적으로 `UWxAbilityBase::SharesCooldownGroup`을 지웠다. 세트 검증은 `GetCooldownTags()`를 직접 비교한다.
- 외부 사례: GASDocumentation(tranek, 비공식 커뮤니티 문서) 4.5.15가 동적 태그 쿨다운을 소개한다. 충전형 공개 사례(Ninh Hoang의 스택 쿨다운 GE 등)는 이전 구조와 같은 방식이다.

## 구조

- `UWxEffect_Cooldown`은 구체 클래스 하나이고 `UWxAbilityBase`의 `CooldownGameplayEffectClass` 기본값이다. HasDuration, 스택 없음. `Cooldown` 부모 태그를 부여한다(아래 엔진 규칙).
- `UWxAbilityBase::ApplyCooldown`이 공용 GE 스펙을 만들고 `DynamicGrantedTags`에 `CooldownTags`를 붙여 적용한다. `GetCooldownTags()`는 `&CooldownTags`를 돌려준다.
- 충전: 소모한 충전 하나가 GE 하나다. `CheckCooldown`은 `CooldownTags`로 찾은 활성 GE 수가 `MaxRecharges` 미만이면 통과시키고, 아니면 순정 판정(실패 태그 채움)에 맡긴다. `CooldownTags`가 비면 바로 통과한다.
- 차례 회복: `ApplyCooldown`이 적용 전에 같은 태그 쿨다운의 최대 남은 시간 + `CooldownTime`을 구해 `SetByCaller.Duration`으로 넘긴다(HitStop GE와 같은 방식). 예: 회피(2초, 충전 2)를 0초와 0.5초에 쓰면 2초·4초에 하나씩 돌아온다. 이전 스택 GE(NeverRefresh + RemoveSingleStackAndRefreshDuration)와 같은 결과다.
- UI: `UWxViewModel_Ability`가 활성 GE의 `Spec.DynamicGrantedTags`로 자기 쿨다운을 센다. 표시 주기는 `IWxUIData::GetCooldownTime()`(WxCore, 기본 0)으로 받은 `CooldownTime`이다. GE 지속시간은 대기열이 섞여 충전 하나의 주기가 아니다.
- 쿨다운 무시(`UWxEffect_IgnoreCooldowns`)는 그대로 `Cooldown` 부모 태그로 막고 걷는다.
- 검증: GA_에 `CooldownTime`이 있는데 `CooldownTags`가 비면 오류. 세트는 같은 쿨다운 태그를 쓰는데 시간이나 충전 수가 다르면 경고.
- 지운 것: 파생 클래스 전부와 `GrantCooldownTag`, 지속시간 MMC `UWxMMC_CooldownDuration`, `Cooldown.Skill.4` 태그, `SharesCooldownGroup`.

## 엔진 사실(UE 5.8)

- GE 스택은 `Spec.Def`(GE 클래스)로만 합친다(`FindStackableActiveGameplayEffect`). 태그가 달라도 같은 클래스면 한 스택이 되므로 공용 GE는 쌓을 수 없다.
- `DynamicGrantedTags`는 복제되고 `GetAllGrantedTags`와 ASC 태그 맵에 들어간다. 그래서 순정 `CheckCooldown`·`MatchAnyOwningTags` 쿼리가 그대로 본다. 동적 태그가 다른 스펙을 같은 스택에 쌓으면 엔진이 ensure로 경고한다.
- 순정 규칙: `UGameplayAbility::IsDataValid`는 `CooldownGameplayEffectClass`의 GE가 태그를 부여하지 않으면 "grants no tags" 오류를 낸다. `CheckCooldown`은 `GetCooldownTags()`가 빈 컨테이너인데 `CooldownGameplayEffectClass`가 있으면 경고한다. 둘 다 CVar `AbilitySystem.WarnCooldownEffectWithoutTags`가 켜고, 엔진 주석은 `CheckCooldown`·`GetCooldownGameplayEffect`를 오버라이드하면 끄라고 한다. 설계 때 이 규칙을 놓쳐 첫 검증에서 GA_ 7개가 오류였다. CVar를 끄는 대신 공용 GE가 `Cooldown` 부모 태그를 부여하고 빈 태그면 `CheckCooldown`이 먼저 통과시켜 순정 규칙을 지킨다. 부모 태그는 자식 태그 쿼리(`Cooldown.Dodge` 등)에 걸리지 않는다.

## 에셋 이관과 검증

- 헤드리스 Python 커맨드릿으로 GA_ 9개의 `CooldownTags`를 채우고 `CooldownGameplayEffectClass` 저장값을 지웠다(기본값을 따름). HGTest Skill_1 `Cooldown.Skill.1`, Skill_3 `Cooldown.Skill.3`, Ultimate_1·2 `Cooldown.Ultimate`, Template Skill `Cooldown.Skill.1`, Template Ultimate `Cooldown.Ultimate`, Shared Dodge `Cooldown.Dodge`. `CooldownTime`이 0인 HGTest Skill_2·Minion Skill_1은 태그 없이 두었다.
- WxEditor Development 빌드 성공. 데이터 검증 커맨드릿: GameplayAbilityBlueprint 40개·WxAbilitySet 9개 오류 0(경고 1건은 MCP 플러그인 EULA 안내).
- 임시 자동화 테스트(확인 후 삭제): 게임 월드의 ASC에 GA_Shared_Dodge를 주고 `ApplyCooldown`을 두 번 불러 지속시간 2.0초 → 2.0·4.0초, 첫 적용 뒤 `CheckCooldown` 통과·두 번째 뒤 차단을 확인했다.
- 사용자 인게임 확인(아래 수정 뒤, 2026-09-25): "잘 되네요". 보고했던 회피 쿨다운과 UI 진행률에 대한 답이다. 소환물 쿨다운 무시와 네트워크 복제는 확인 범위에 드러나지 않았다.

## 수정: 1회 사용에 4초 쿨다운

- 사용자 보고: 회피를 한 번만 썼는데 쿨다운이 4초이고 UI 진행률이 이상하다.
- 원인: 처음에는 지속시간 MMC가 활성 쿨다운을 조회했다. 엔진 `FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec`은 새 GE를 활성 목록에 넣은 뒤 지속시간을 다시 계산한다(`GameplayEffect.cpp` 4353행 추가, 4429행 재계산). 그래서 MMC가 방금 들어간 자기 자신(스펙 생성 때 계산한 2초)을 대기열로 세어 2 + 2 = 4초가 됐다. UI는 남은 시간을 충전 하나의 주기(2초)로 나눠 진행률이 1을 넘었다.
- 수정: 대기열 계산을 적용 전 `ApplyCooldown`으로 옮기고 값은 SetByCaller로 넘겼다. 재계산은 저장된 SetByCaller 값을 그대로 읽는다(1240행). GASDocumentation 4.5.15의 ApplyCooldown 예시도 동적 태그 + SetByCaller 지속시간이다.
