---
type: source
title: "결정 노트 - 2026-09-25-cooldown-single-ge"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "GAS"
  - "쿨다운"
summary: "쿨다운 그룹별 GE 파생 클래스를 공용 UWxEffect_Cooldown 하나로 합치고 어빌리티 CooldownTags와 SetByCaller 대기열로 충전을 차례 회복시킨 결정"
source_type: decision-note
source_id: src-89713cd38b367ce6bb7f
sha256: 5cd247dc4de1588971b8c12f96ec86eee45bb0bf80aa348fe97cd5afe0b0f579
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-cooldown-single-ge.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-cooldown-single-ge.md"
raw_copy: ".raw/captured/5cd247dc4de1588971b8c12f96ec86eee45bb0bf80aa348fe97cd5afe0b0f579.md"
claim_ids:
  - clm-224c97d3c1-c1
  - clm-224c97d3c1-c2
  - clm-224c97d3c1-c3
  - clm-224c97d3c1-c4
key_claims:
  - "2026-09-25 결정으로 쿨다운 그룹별 GE 파생 클래스를 없애고 공용 UWxEffect_Cooldown 하나를 UWxAbilityBase의 기본 쿨다운 GE로 쓴다."
  - "쿨다운 식별자는 어빌리티의 FGameplayTagContainer CooldownTags이며 ApplyCooldown이 이를 스펙의 DynamicGrantedTags로 붙인다."
  - "ApplyCooldown은 같은 태그 쿨다운의 최대 남은 시간에 CooldownTime을 더한 값을 SetByCaller.Duration으로 넘겨 충전이 차례로 돌아오게 한다."
  - "1회 사용에 4초 쿨다운이 걸린 버그는 지속시간 MMC가 방금 추가된 자기 GE를 대기열로 센 탓이었고, 계산을 ApplyCooldown으로 옮겨 고친 뒤 사용자가 인게임에서 확인했다."
---

# 결정 노트 - 2026-09-25-cooldown-single-ge

- 원본: `.wiki/raw/notes/2026-09-25-cooldown-single-ge.md`
- 원자료 사본: `.raw/captured/5cd247dc4de1588971b8c12f96ec86eee45bb0bf80aa348fe97cd5afe0b0f579.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-cooldown-single-ge.md`(제목 "쿨다운 GE를 하나로: 어빌리티 CooldownTags와 차례 회복", source `MANUAL`, ingested 2026-09-25)를 요약한다. 2026-09-25 미커밋 작업 트리(HEAD `38d4dde08`) 기준 기록이며, 에디터 빌드·데이터 검증 커맨드릿·임시 자동화 테스트와 수정 뒤 사용자 인게임 확인이 함께 적혀 있다. 노트 날짜 기준이라 현재 코드와 다를 수 있다. 작업 기록은 `.agents/workflow/tasks/cooldown-unification.md`([[작업 - cooldown-unification]])이다.

## 사람의 판단 원문

- 발단(사용자): BP 커스텀·오버라이드 때문에 AI가 구조를 헷갈릴 수 있고, `Skill.1`·`Ultimate.1` 같은 번호 태그가 없어지길 원했다(노트의 요약 서술).
- 사용자 결정(노트 서술): `Ability.Pattern.N`은 BT 재정비에 필요할 수 있어 유지하고, `Ability.Skill.N` 제거는 BT 재정비 때 한다. 스킬·쿨다운의 파생 클래스는 최소화한다.

> 사용자 2026-09-25: "쿨다운 태그는 따로 두더라도, 쿨다운 클래스는 통합할 수 있지 않나요?"

- 필드는 `FGameplayTagContainer CooldownTags`로, `FGameplayTag CooldownGroup` 단일 태그안을 검토했지만 사용자가 컨테이너 유지를 결정했다(엔진 `GetCooldownTags()`가 컨테이너 포인터를 돌려줌).
- 사용자 지적으로 `UWxAbilityBase::SharesCooldownGroup`을 지웠다.

> 사용자 2026-09-25(수정 뒤 인게임 확인): "잘 되네요"

## 확정 구조

- `UWxEffect_Cooldown`은 구체 클래스 하나이며 `UWxAbilityBase`의 `CooldownGameplayEffectClass` 기본값이다. HasDuration, 스택 없음, `Cooldown` 부모 태그를 부여한다.
- `UWxAbilityBase::ApplyCooldown`이 공용 GE 스펙의 `DynamicGrantedTags`에 `CooldownTags`를 붙여 적용한다. `GetCooldownTags()`는 `&CooldownTags`를 돌려준다.
- 충전: 소모한 충전 하나가 GE 하나다. `CheckCooldown`은 `CooldownTags`로 찾은 활성 GE 수가 `MaxRecharges` 미만이면 통과, 아니면 순정 판정에 맡긴다. `CooldownTags`가 비면 바로 통과한다.
- 차례 회복: `ApplyCooldown`이 적용 전에 같은 태그 쿨다운의 최대 남은 시간 + `CooldownTime`을 `SetByCaller.Duration`으로 넘긴다. 예: 회피(2초, 충전 2)를 0초·0.5초에 쓰면 2초·4초에 하나씩 돌아온다.
- UI: `UWxViewModel_Ability`가 활성 GE의 `Spec.DynamicGrantedTags`로 자기 쿨다운을 세고, 표시 주기는 `IWxUIData::GetCooldownTime()`로 받은 `CooldownTime`이다(GE 지속시간은 대기열이 섞여 한 칸 주기가 아님).
- 검증 규칙: GA_에 `CooldownTime`이 있는데 `CooldownTags`가 비면 오류, 세트가 같은 태그에 다른 시간·충전 수를 쓰면 경고.
- 삭제: 파생 클래스 전부, `GrantCooldownTag`, MMC `UWxMMC_CooldownDuration`, `Cooldown.Skill.4` 태그, `SharesCooldownGroup`.

## 엔진 사실(UE 5.8, 노트 조사)

- GE 스택은 `Spec.Def`(GE 클래스)로만 합치므로 공용 GE는 쌓을 수 없다.
- `DynamicGrantedTags`는 복제되고 ASC 태그 맵에 들어가 순정 `CheckCooldown` 쿼리가 본다.
- 순정 `IsDataValid`는 쿨다운 GE가 태그를 부여하지 않으면 오류를 낸다(CVar `AbilitySystem.WarnCooldownEffectWithoutTags`). 설계 때 놓쳐 첫 검증에서 GA_ 7개가 오류였고, CVar를 끄는 대신 `Cooldown` 부모 태그 부여 + 빈 태그면 먼저 통과로 순정 규칙을 지켰다.

## 수정: 1회 사용에 4초 쿨다운

- 사용자 보고: 회피 한 번에 쿨다운 4초, UI 진행률 이상.
- 원인: 지속시간 MMC가 활성 쿨다운을 조회했는데, 엔진이 새 GE를 활성 목록에 넣은 뒤 지속시간을 재계산해 자기 자신을 대기열로 세어 2 + 2 = 4초가 됐다.
- 수정: 대기열 계산을 적용 전 `ApplyCooldown`으로 옮기고 SetByCaller로 넘겼다.

## 검증 범위

- 헤드리스 Python 커맨드릿으로 GA_ 9개의 `CooldownTags`를 채우고 `CooldownGameplayEffectClass` 저장값을 지웠다. `CooldownTime` 0인 HGTest Skill_2·Minion Skill_1은 태그 없이 둠.
- WxEditor Development 빌드 성공. 데이터 검증 커맨드릿: GameplayAbilityBlueprint 40개·WxAbilitySet 9개 오류 0.
- 임시 자동화 테스트(확인 후 삭제): `GA_Shared_Dodge`에 `ApplyCooldown` 두 번 → 지속시간 2.0·4.0초, 두 번째 뒤 `CheckCooldown` 차단 확인.
- 사람 인게임 확인은 회피 쿨다운과 UI 진행률에 대한 답이다. 소환물 쿨다운 무시와 네트워크 복제는 확인 범위에 드러나지 않았다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[UI 표시 구조]]
- [[작업 - cooldown-unification]]
- <!--wl-->결정 노트 - 2026-09-26-cooldown-play-acceptance

## 핵심 주장

- 2026-09-25 결정으로 쿨다운 그룹별 GE 파생 클래스를 없애고 공용 UWxEffect_Cooldown 하나를 UWxAbilityBase의 기본 쿨다운 GE로 쓴다. ^c1
- 쿨다운 식별자는 어빌리티의 FGameplayTagContainer CooldownTags이며 ApplyCooldown이 이를 스펙의 DynamicGrantedTags로 붙인다. ^c2
- ApplyCooldown은 같은 태그 쿨다운의 최대 남은 시간에 CooldownTime을 더한 값을 SetByCaller.Duration으로 넘겨 충전이 차례로 돌아오게 한다. ^c3
- 1회 사용에 4초 쿨다운이 걸린 버그는 지속시간 MMC가 방금 추가된 자기 GE를 대기열로 센 탓이었고, 계산을 ApplyCooldown으로 옮겨 고친 뒤 사용자가 인게임에서 확인했다. ^c4
