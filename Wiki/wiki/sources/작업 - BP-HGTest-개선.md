---
type: source
title: "작업 - BP-HGTest-개선"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "HGTest"
  - "GAS"
  - "도플갱어"
summary: "BP_HGTest의 궁극기 코스트를 UP 100으로 통일하고 미니언·분신 소환 중 UP 회복과 분신 소환 중 전기 루프 VFX(GameplayCue)를 넣은 2026-09-29 완료 작업 기록"
source_type: task-record
source_id: src-052f476092678a6a0d42
sha256: 4f960430d61dfbd06c2424e498e0b9ec8ed7b45be6ea6a86c730d098aea45ec4
authority: primary
independence_key: ".agents/workflow/tasks/BP-HGTest-개선.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/BP-HGTest-개선.md"
raw_copy: ".raw/captured/4f960430d61dfbd06c2424e498e0b9ec8ed7b45be6ea6a86c730d098aea45ec4.md"
claim_ids:
  - clm-4f960430d6-c1
  - clm-4f960430d6-c2
  - clm-4f960430d6-c3
  - clm-4f960430d6-c4
  - clm-4f960430d6-c5
key_claims:
  - "2026-09-29 woogle 요청으로 GA_HGTest_Ultimate_1 코스트가 MP 3에서 UP 100으로 바뀌어 궁극기 1·2가 모두 UP 100을 쓰고, GE_HGTest_Attack_Heavy_2·Skill_2는 Master.Minion 상태에서 발동마다 UP 33.4를 준다."
  - "woogle은 2026-09-29 분신 소환 중 피해 UP 10을 공격 발동 1회당 한 번, 플레이어 본인의 피해만으로 유지하기로 답했고(Q1·Q2), 기존 GA_HGTest_Passive와 GE_HGTest_AddUP를 그대로 쓴다."
  - "같은 작업에서 GE_HGTest_Skill_3이 삭제되고 DT_CharacterAttribute HGTest 행 MaxMP가 0이 되어 HGTest는 MP를 쓰지 않는다."
  - "분신 소환 중 VFX는 Master.Doppelganger 지속 요건의 Infinite GE_HGTest_Doppelganger가 GameplayCue.Doppelganger를 켜고, 순정 루프 Cue GC_Doppelganger가 NS_Player_Electricity_Looping을 캐릭터 메시에 붙이는 방식이며 ABS_HGTest가 이 GE를 부여한다."
  - "BP_HGTest 개선은 헤드리스 임시 자동화 테스트(Niagara는 -RenderOffscreen)와 빌드로 AI가 확인했고 woogle이 2026-09-29 VFX 연출과 코드 리뷰를 통과시켜 체크리스트 8/8로 완료됐다."
---

# 작업 - BP-HGTest-개선

- 원본: `.agents/workflow/tasks/BP-HGTest-개선.md`
- 원자료 사본: `.raw/captured/4f960430d61dfbd06c2424e498e0b9ec8ed7b45be6ea6a86c730d098aea45ec4.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

woogle이 2026-09-29 요청한 BP_HGTest 개선 네 가지(궁극기 1·2 코스트 UP 100 통일, BP_Minion 소환 중 Ability.Action.Attack.Heavy.2·Skill.2 발동마다 UP 33.4 회복, BP_Doppelganger 소환 중 적 피해마다 UP 10 회복, 분신 소환 중 플레이어에 Niagara GameplayCue 유지)를 다룬 작업 기록이다. 웹 처리로 정하기·구현·수정을 Claude Code가 맡았다. 상태는 완료(체크리스트 8/8 통과)다.

## 질문과 결정(woogle 2026-09-29)

- Q1 피해 UP 10의 지급 단위: 공격 발동 1회당 10(현행 유지). 다단·다수 적중으로 UP가 한 번에 차는 것을 막는 현재 패시브 설계를 쓴다.
- Q2 분신이 입힌 피해: 플레이어 본인의 피해만 센다. `Event.DamageDealt`는 피해를 준 쪽 ASC에만 가므로 코드 변경이 없다.
- Q3 남는 MP: 스킬3의 MP 획득 GE를 없애고 DT_CharacterAttribute HGTest 행 MaxMP를 0으로 한다.
- Q4 루프 Niagara: 처음 NS_Player_Buff_Looping으로 정했으나, 사람 확인에서 "버프 오라를 아까 추천해주셨던 전기 Looping 으로 바꿔주세요."라는 실패 메모로 NS_Player_Electricity_Looping으로 바꿨다.

## 구현 결과

- GA_HGTest_Ultimate_1 코스트를 MP 3에서 UP 100으로 바꿨다(Ultimate_2는 이미 UP 100).
- GE_HGTest_Attack_Heavy_2·GE_HGTest_Skill_2의 모디파이어를 MP AddBase 1에서 UP AddBase 33.4로 바꿨다. 두 GA는 이미 Master.Minion을 요구하고, 속성 클램프로 3회째에 100이 된다.
- 분신 중 피해 UP 10은 기존 GA_HGTest_Passive와 GE_HGTest_AddUP(UP +10, Master.Doppelganger 적용 요건)를 그대로 쓴다.
- GA_HGTest_Skill_3의 ActivationOwnedEffects에서 GE_HGTest_Skill_3을 빼고 참조 0건을 확인한 뒤 에셋을 지웠으며, HGTest 행 MaxMP를 3에서 0으로 바꿨다.
- 분신 VFX: 네이티브 태그 `GameplayCue.Doppelganger`(WxCore `WxGameplayTags.h/.cpp`), 순정 `GameplayCueNotify_Looping` BP `GC_Doppelganger`(캐릭터 메시에 AttachToTarget), Infinite GE `GE_HGTest_Doppelganger`(TargetTagRequirements Ongoing=Master.Doppelganger, GameplayCues=GameplayCue.Doppelganger)를 만들고 ABS_HGTest GrantedEffects에 넣었다. 분신 소환·해제로 Master.Doppelganger가 붙었다 떨어지면 GE가 억제되거나 풀리며 Cue가 켜지고 꺼진다.
- 에셋 편집은 Wx.uproject에 PythonScriptPlugin을 잠시 켜고 unreal-mcp로 했고, 끝난 뒤 원래 파일로 되돌렸다.

## 검증 범위

- AI 헤드리스(임시 자동화 테스트 뒤 삭제, 임시 테스트는 Source/WxEditor/Tests에 둠): UP 99에서 궁극기1 거절·UP 100에서 발동 뒤 UP 0, 미니언 중 33.40→66.80→100.00(클램프)·MP 0 유지, 분신 중 단독 적중 +10·같은 발동 2회 적중 +10 한 번·분신 ASC 이벤트와 소환 전·해제 후 미지급, Skill_3 뒤 MP 0·MaxMP 0, GE 억제·Cue 태그 토글.
- Niagara 판정은 nullrhi에서 컴포넌트가 만들어지지 않아 `-RenderOffscreen`으로 확인했다. 전기 이펙트 교체 뒤 첫 실행은 Niagara 첫 컴파일(1.09초) 때문에 판정 시점에 FX가 없어 실패했고, 다시 돌려 통과했다. 에디터에서 처음 한 번은 이펙트가 조금 늦게 뜰 수 있다고 기록은 적는다.
- 빌드: 테스트 포함·삭제 뒤 모두 Result: Succeeded. Export-AbilitySystemLists 목록에 값이 반영됐다.
- 사람: woogle 2026-09-29 분신 소환 중 VFX 연출(전기 교체 뒤)과 코드 리뷰 통과.

## 관련 주제

- [[어빌리티와 GAS]]
- [[캐릭터 스탯과 전투 자원]]

## 핵심 주장

- 2026-09-29 woogle 요청으로 GA_HGTest_Ultimate_1 코스트가 MP 3에서 UP 100으로 바뀌어 궁극기 1·2가 모두 UP 100을 쓰고, GE_HGTest_Attack_Heavy_2·Skill_2는 Master.Minion 상태에서 발동마다 UP 33.4를 준다. ^c1
- woogle은 2026-09-29 분신 소환 중 피해 UP 10을 공격 발동 1회당 한 번, 플레이어 본인의 피해만으로 유지하기로 답했고(Q1·Q2), 기존 GA_HGTest_Passive와 GE_HGTest_AddUP를 그대로 쓴다. ^c2
- 같은 작업에서 GE_HGTest_Skill_3이 삭제되고 DT_CharacterAttribute HGTest 행 MaxMP가 0이 되어 HGTest는 MP를 쓰지 않는다. ^c3
- 분신 소환 중 VFX는 Master.Doppelganger 지속 요건의 Infinite GE_HGTest_Doppelganger가 GameplayCue.Doppelganger를 켜고, 순정 루프 Cue GC_Doppelganger가 NS_Player_Electricity_Looping을 캐릭터 메시에 붙이는 방식이며 ABS_HGTest가 이 GE를 부여한다. ^c4
- BP_HGTest 개선은 헤드리스 임시 자동화 테스트(Niagara는 -RenderOffscreen)와 빌드로 AI가 확인했고 woogle이 2026-09-29 VFX 연출과 코드 리뷰를 통과시켜 체크리스트 8/8로 완료됐다. ^c5
