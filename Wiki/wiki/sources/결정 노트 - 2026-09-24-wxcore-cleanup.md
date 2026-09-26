---
type: source
title: "결정 노트 - 2026-09-24-wxcore-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "모듈"
  - "GameplayTag"
summary: "읽는 곳 없는 Damage.Guarded·Device.Locked 태그와 빈 FWxCoreModule, WxAI 자동화 테스트 두 개를 지운 WxCore 정리 기록"
source_type: decision-note
source_id: src-80c6c8e3c28e7a130a33
sha256: 3b4ab190a666b4296740e3b02435dd884d57de642ba72836fd6230a065e7cb73
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-wxcore-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-wxcore-cleanup.md"
raw_copy: ".raw/captured/3b4ab190a666b4296740e3b02435dd884d57de642ba72836fd6230a065e7cb73.md"
claim_ids:
  - clm-02481efe23-c1
  - clm-02481efe23-c2
  - clm-02481efe23-c3
  - clm-02481efe23-c4
key_claims:
  - "사용자는 2026-09-24 WxCore에서 게임플레이 로직을 구현하지 않고 순수 정의만 두겠다고 말했다."
  - "커밋 db5898fa8은 읽는 곳이 없던 Damage.Guarded와 참조 0건인 Device.Locked 네이티브 태그를 삭제했다."
  - "커밋 6ec2d9e8f 이후 WxCore 모듈은 FWxCoreModule 없이 FDefaultModuleImpl로 등록된다."
  - "WxAI의 MirrorMovement 순간이동 테스트와 정찰 경로 스폰 테스트가 삭제되어 WxAI에는 자동화 테스트가 남지 않는다."
---

# 결정 노트 - 2026-09-24-wxcore-cleanup

- 원본: `.wiki/raw/notes/2026-09-24-wxcore-cleanup.md`
- 원자료 사본: `.raw/captured/3b4ab190a666b4296740e3b02435dd884d57de642ba72836fd6230a065e7cb73.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-24. 커밋 `db5898fa8`(태그)·`6ec2d9e8f`(모듈 클래스)·`b746867c3`·`bd03a9249`(테스트).
- WxCore 소스 전체를 읽고 태그마다 WxCore 밖 C++와 `Content`·`Plugins/*/Content`·`Config` 바이너리 문자열을 검색해 사용처를 센 정적 조사다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-24: "WxCore에서 게임플레이 로직 구현 안하고, 순수 정의만 할거라서요."

- 이 발언이 빈 `StartupModule`/`ShutdownModule`만 있던 `FWxCoreModule`을 없애는 근거다. 사용자는 `Damage_Guarded`·`Device_Locked` 제거와 `Tests/WxMirrorMovementAbilityTeleportTest` 제거를 지시했다(지시 원문은 노트에 없음).

## 변경

- `Damage.Guarded`: ExecCalc(`UWxExecCalc_Damage`)가 일반 가드 히트에 붙이기만 하고 읽는 곳이 없었다. 2026-09-23 `FWxDamageResult`(`bGuarded` 포함) 삭제 뒤로 소비처가 없다. 태그와 부착 분기만 지웠고 가드 경감·SP 차감의 `bGuardHit` 판정은 그대로다.
- `Device.Locked`: 2026-09-22 `f87b5e3ae`에서 추가됐지만 StateTree 에셋 포함 참조 0건. 잠금은 받는 쪽 대기 태스크 활성 여부로 판단한다.
- `FWxCoreModule`: 헤더를 지우고 `IMPLEMENT_MODULE(FDefaultModuleImpl, WxCore)`로 등록한다.
- WxAI `FWxMirrorMovementAbilityTeleportTest`와 `UWxBTService_MirrorMovement`의 `friend` 선언, 이어서 사용자 지시로 `FWxPatrolPathSpawnTest`도 지워 WxAI에 자동화 테스트가 남지 않는다. 테스트가 지키던 함정(빙의가 폰 Owner를 덮기 전에만 스폰 주체를 알 수 있다)은 `UWxAIBehaviorComponent` 주석에 남았다.

## 미결정·남긴 후보

- 참조 0건이지만 남긴 후보: `Ability.Skill.4`·`Cooldown.Skill.4`(+ `UWxEffect_Cooldown_Skill_4`), `Ability.Pattern.4`~`9`. 클래스 이름의 `_N`은 FName 숫자로 분리 저장되어 문자열 검색으로 에셋 참조를 확정할 수 없다.
- `AWxDevice::GetInteractionPrompt`는 호출되지 않지만 `IWxInteractable`의 순수 가상이라 남아 있다.

## 검증 범위

- WxEditor Win64 Development 빌드 성공(`Saved/Logs/BuildDoctor/` 로그 두 개). 동작 변화가 없는 제거라 플레이는 확인하지 않았다.

## 관련 주제

- [[모듈 구조와 코드 정리]]
- [[피해 파이프라인]]
- [[상호작용과 장치]]

## 핵심 주장

- 사용자는 2026-09-24 WxCore에서 게임플레이 로직을 구현하지 않고 순수 정의만 두겠다고 말했다. ^c1
- 커밋 db5898fa8은 읽는 곳이 없던 Damage.Guarded와 참조 0건인 Device.Locked 네이티브 태그를 삭제했다. ^c2
- 커밋 6ec2d9e8f 이후 WxCore 모듈은 FWxCoreModule 없이 FDefaultModuleImpl로 등록된다. ^c3
- WxAI의 MirrorMovement 순간이동 테스트와 정찰 경로 스폰 테스트가 삭제되어 WxAI에는 자동화 테스트가 남지 않는다. ^c4
