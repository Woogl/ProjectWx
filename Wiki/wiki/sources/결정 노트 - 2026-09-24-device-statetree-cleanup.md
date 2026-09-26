---
type: source
title: "결정 노트 - 2026-09-24-device-statetree-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "StateTree"
  - "장치"
  - "구조"
summary: "장치 StateTree 정리 기록. 복원 판정을 IsRestoring으로 옮기고 중복 장치를 지우며 몽타주 태스크를 WxCombat으로 이관하고 연출 태스크를 고쳤다."
source_type: decision-note
source_id: src-6c61df55f8cfa6a27c95
sha256: 5248383c77f14befbbdef3b1276876e7db56e92b011813b3b79cd304e52ea7aa
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-device-statetree-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-device-statetree-cleanup.md"
raw_copy: ".raw/captured/5248383c77f14befbbdef3b1276876e7db56e92b011813b3b79cd304e52ea7aa.md"
claim_ids:
  - clm-c74c23a86d-c1
  - clm-c74c23a86d-c2
  - clm-c74c23a86d-c3
  - clm-c74c23a86d-c4
key_claims:
  - "2026-09-24 이후 장치 StateTree의 복원 판정은 UWxDeviceStateTreeComponent::IsRestoring이 SourceStateID 무효 또는 bRestoringState로 내린다."
  - "상호작용자 몽타주 재생은 WxWorld 태스크에서 WxCombat FWxStateTreeTask_PlayMontageOnce로 옮겨졌다."
  - "IsRestoring을 모르는 다른 도메인 태스크는 서버의 InitialState 전이를 실제 진입으로 보므로 InitialState에 일회성 효과를 두지 않는다는 저작 규칙이 있다."
  - "InitialState를 지정 상태에서 시작하는 순정 해법은 엔진이 컴포넌트에 시작 상태 지정을 열 때까지 보류되었다."
---

# 결정 노트 - 2026-09-24-device-statetree-cleanup

- 원본: `.wiki/raw/notes/2026-09-24-device-statetree-cleanup.md`
- 원자료 사본: `.raw/captured/5248383c77f14befbbdef3b1276876e7db56e92b011813b3b79cd304e52ea7aa.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-24-device-statetree-cleanup.md` (source: MANUAL, ingested: 2026-09-24).
- 2026-09-24 커밋 `b32c1f622`·`844010a94`·`f6b4af9d4`·`9e922a15b`·`36fbb4371`·`80e3e370b`·`8f06b1ffc`·`70495c0e7`의 기록이다. Wiki refresh에서 반영 누락으로 찾았고 HEAD `142fab5d6`에서 코드를 다시 읽어 대조한 **정적 조사(빌드·실행 검증 아님)**다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.
- `36fbb4371`까지는 상호작용 계약 작업([[결정 노트 - 2026-09-24-interaction-contract-options-only]])의 WxEditor 빌드가 통과했다. 뒤의 세 커밋은 빌드 기록이 없고, 인게임 동작은 모두 확인하지 않았다.

## 복원 판정(구현 관찰)

- `FWxDeviceExecutionPolicy`를 지우고 `UWxDeviceStateTreeComponent::IsRestoring(Context, Transition)`이 판정한다. `Transition.SourceStateID`가 무효(트리 시작·재시작)면 복원이고, 그 밖에는 컴포넌트가 스냅샷을 따라 요청한 복원 전이(`bRestoringState`)일 때만 복원이다.
- 헤더 계약: 상태 적용(위치·표시)은 복원에서도 실행하고, 일회성 효과(사운드·보상·스폰)는 실제 진입에서만 실행한다.
- `ComponentMove`·`SplineMove`는 같은 `IsRestoring`을 써서 트리 시작에서도 목표 위치로 즉시 맞춘다.
- WxWorld를 참조할 수 없는 다른 도메인 태스크(WxCombat `PlayMontageOnce`, WxInventory `GiveRewards`·`RefillItemCharges`)는 `!Transition.SourceStateID.IsValid()`만 본다.

## 중복 장치 제거(구현 관찰)

- `IsRunning` 오버라이드 삭제. `AWxDevice::GetInteractionOptions`·`GetAcceptedOptions`는 대기 노드 등록만 보고, `EnterState`는 `GetStateTreeRunStatus() != Running`이면 재시작한다. `DisableTick` 호출도 삭제.
- `InitialState` 사전 검증 삭제. `SynchronizeAfterStart`가 `EnterState(InitialState 태그, 복원)`를 요청하고, 실패하면 에러 로그 후 루트 상태를 발행한다.
- 스냅샷의 `FName StateTagName`을 `FGameplayTag StateTag`로 바꿨다.
- `TWxStateTreeWaitRegistry` 템플릿을 지우고 `WxStateTreeTask_WaitForInteraction.cpp` 안에 등록 배열을 뒀다(동작 동일: 핸들 재사용 안 함, 같은 월드 대기만 완료).

## 몽타주 태스크 WxCombat 이관

- WxWorld `FWxStateTreeTask_PlayInteractorMontage`를 지우고 WxCombat `FWxStateTreeTask_PlayMontageOnce`("몽타주 1회 재생")를 추가했다. 노트는 "사용자가 효과 도메인에 두자고 제안해 적용했다"고 적는다(발화 원문 없음).
- 이유: 도메인 사이 의존이 금지라 WxWorld 태스크는 WxCombat 어빌리티를 직접 참조하지 못한다. 선례는 WxInventory `GiveRewards`.
- 권위에서 `GiveAbilityAndActivateOnce`로 1회 발동하고, 스펙이 걷히거나 비활성이면 Succeeded. 비권위 피어는 Running으로 머물러 서버의 다음 상태를 기다리므로, 이 태스크 상태와 다음 상태는 서로 다른 태그 상태여야 한다.
- `AWxDevice::InteractingCharacter`를 바인딩 피커용으로 `VisibleInstanceOnly`로 바꿨다. WxCombat은 `StateTreeModule`·`StateTree` 플러그인 의존을 추가했다.

## InitialState 전이와 다른 도메인 태스크

- 서버의 `InitialState` 복원 전이는 `SourceStateID`가 유효해, `IsRestoring`을 모르는 다른 도메인 태스크가 실제 진입으로 본다(예: `보상 지급` 상자를 InitialState "열림"으로 두면 레벨 시작 때 보상 지급).
- 현재 InitialState 배치는 피스톤(`Device.Piston.Off`)뿐이라 문제가 드러나지 않는다. 필드 주석 규칙: "그 상태에 일회성 효과를 두지 않는다."
- 순정 해법 `FStartParameters::SelectStateOverrideArgs`는 `UStateTreeComponent::StartTree`가 넘길 길이 없어, 엔진이 시작 상태 지정을 열 때까지 보류한다(2026-09-24 사용자 결정, 발화 원문 없음).
- 복원까지 재시작으로 바꾸는 안은 `나이아가라 스폰` 중복과 선택 실패 시 Failed 정지 때문에 기각했다.

## 연출 태스크

- 레벨 시퀀스 재생: 카메라 컷은 장치 당사자를 조종하는 피어에서만 켠다. 재생은 모든 피어가 하므로 상태 종료 시점은 그대로다.
- 스포너 발동: 판정을 `IsRestoring`으로 바꿔 장치 복원 전이에서도 스폰하지 않는다.
- 사운드 재생: `bPlayOnRestore`를 지우고 라이브 발동 1회 원샷으로 정리했다. 상태를 떠나도 소리가 멈추지 않으므로 루프 사운드는 넣지 않는다.

## 미결정·충돌

- InitialState 시작 상태 지정은 엔진 지원 전까지 보류 상태이며, 그동안 InitialState에 일회성 효과를 두지 않는 저작 규칙에 의존한다.
- `Content` 문자열 검색으로는 두 몽타주 태스크 사용 에셋이 나오지 않았으나 FName 분리 등 검색 한계가 있다.

## 관련 주제

- [[상호작용과 장치]]
- [[모듈 구조와 코드 정리]]
- [[레벨 디자인]]

## 핵심 주장

- 2026-09-24 이후 장치 StateTree의 복원 판정은 UWxDeviceStateTreeComponent::IsRestoring이 SourceStateID 무효 또는 bRestoringState로 내린다. ^c1
- 상호작용자 몽타주 재생은 WxWorld 태스크에서 WxCombat FWxStateTreeTask_PlayMontageOnce로 옮겨졌다. ^c2
- IsRestoring을 모르는 다른 도메인 태스크는 서버의 InitialState 전이를 실제 진입으로 보므로 InitialState에 일회성 효과를 두지 않는다는 저작 규칙이 있다. ^c3
- InitialState를 지정 상태에서 시작하는 순정 해법은 엔진이 컴포넌트에 시작 상태 지정을 열 때까지 보류되었다. ^c4
