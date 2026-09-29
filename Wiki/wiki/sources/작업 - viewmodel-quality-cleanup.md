---
type: source
title: "작업 - viewmodel-quality-cleanup"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "MVVM"
  - "WxUI"
summary: "ViewModel 코드 품질 조사 결과로 Setter/Getter·바인딩 없는 필드·베이스 VM 클래스를 지우고 공유 VM 조회를 직접 쓰기로 바꾼 2026-09-29 완료 작업 기록"
source_type: task-record
source_id: src-f597b4c874bdc54d6da1
sha256: c51b0688a5ba707ec1dfbee28c57e41c0a97359abc875b6b7354f38945ceac6a
authority: primary
independence_key: ".agents/workflow/tasks/viewmodel-quality-cleanup.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/viewmodel-quality-cleanup.md"
raw_copy: ".raw/captured/c51b0688a5ba707ec1dfbee28c57e41c0a97359abc875b6b7354f38945ceac6a.md"
claim_ids:
  - clm-c51b0688a5-c1
  - clm-c51b0688a5-c2
  - clm-c51b0688a5-c3
  - clm-c51b0688a5-c4
key_claims:
  - "2026-09-29 VM 코드 품질 정리로 베이스 UWxViewModel이 삭제돼 WxUI VM 14개가 UMVVMViewModelBase를 직접 상속하고, BeginDestroy의 가상 Deinitialize가 없어졌으며 WxViewModelUtils에는 RequestImageAsync만 남았다."
  - "같은 정리에서 Setter/Getter 보일러플레이트, Ability VM의 비용 최대치 구독, 바인딩이 없는 VM 필드(CanActivate·CooldownDuration·IsAttributeEmpty·Portrait·bHasSubtitle·bHasActiveQuest·bClamped 등)가 지워졌고 Ability VM은 CheckCost만 부른다."
  - "woogle은 2026-09-29 공유 VM 조회 방식 검토 뒤 MVVM 원칙에 맞춘 재설계를 새 일감으로 넘기고 이 일감은 Outer 키와 FindObjectWithOuter 직접 쓰기 구조로 완료하게 했다."
  - "VM 코드 품질 정리는 빌드, 위젯 BP 88개 헤드리스 컴파일, 헤드리스 게임 임시 자동화로 AI가 확인했고 woogle이 2026-09-29 코드 리뷰를 통과시켜 체크리스트 5/5로 완료됐다."
---

# 작업 - viewmodel-quality-cleanup

- 원본: `.agents/workflow/tasks/viewmodel-quality-cleanup.md`
- 원자료 사본: `.raw/captured/c51b0688a5ba707ec1dfbee28c57e41c0a97359abc875b6b7354f38945ceac6a.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

woogle이 2026-09-29 "ViewModel 관련 코드를 읽고 코드 품질을 개선할 수 있는 부분을 조사해주세요."라고 요청한 뒤 조사 결과 중 2·3·4·5·6·8번과 베이스 VM 제거(9번)를 고친 작업 기록이다. 코드 리뷰 중 공유 VM 조회 방식을 검토하다 MVVM 원칙에 맞춘 재설계를 새 일감([[작업 - viewmodel-mvvm-redesign]])으로 넘겼다. 상태는 완료(체크리스트 5/5 통과)다.

## 결정(woogle 2026-09-29)

- 범위: 조사 2·3·4·5·8번은 이번에, 1번(AbilitySystem VM 생성 경로 하나로)은 별도 작업, 7번(InteractionList VM의 WxUI 이동)은 나중으로 뺐다. 6번(바인딩 없는 필드)은 "6번은 지금은 안쓰는게 맞고, 나중에 필요해지면 그 때 구현하죠"로 이번 범위에 넣어 지운다.
- 베이스 VM 제거: woogle이 "WxViewModelBase 클래스를 지우고 GetOrCreateViewModel<T> 유틸 함수를 만들까요?"라고 제안했고 AI 추천에 "네 추천대로 하세요"로 승인했다.
- 공유 VM 조회: 코드 리뷰 중 Outer 방식·전용 서브시스템·Global Collection·MVVM 원칙을 비교한 뒤 "아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다."로 이 일감은 Outer 키 + `FindObjectWithOuter` 구조로 마쳤다.
- 완료 직후 woogle이 `GetOrCreate` 두 번째 인자의 `&&`가 읽기 어렵다고 하자 AI가 직접 쓰기를 추천했고 "네. 그렇게 합시다."로 공용 `WxViewModel::GetOrCreate<T>` 템플릿을 없앴다.

## 구현 결과

- Ability·Effect·Attribute VM의 Setter/Getter 지정자와 한 줄 Get/Set 쌍을 지우고 `UE_MVVM_SET_PROPERTY_VALUE`를 직접 쓴다(파생 필드를 함께 갱신하는 `SetMaxRecharges`·`SetStackCount`는 private으로 남김, `ActiveEffectViewModels` Getter는 지연 구독 때문에 유지).
- Ability VM의 비용 최대치(`CostMaxAttribute`) 구독을 지워 비용 자원 하나만 구독한다. 직접 비교·통지하던 곳을 `UE_MVVM_SET_PROPERTY_VALUE`로, 남은 시간 계산을 `FActiveGameplayEffect::GetTimeRemaining`으로 바꿨다.
- 바인딩이 없는 필드(Content 문자열 검색 0건)를 지웠다: Ability `CanActivate`·`CooldownDuration`, Attribute `IsAttributeEmpty`, Character `Portrait`(초상화 인자 포함), Subtitle `bHasSubtitle`, Quest `bHasActiveQuest`, Indicator `bClamped`(`SetCameraDistance`로), ConversionLibrary `Conv_TagRequirementsToVisibility`. `CanActivate`가 없어져 Ability VM은 `CheckCost`만 부른다(`RefreshCheckCost`).
- 베이스 VM `UWxViewModel`을 지우고 VM 14개가 엔진 `UMVVMViewModelBase`를 직접 상속한다. `BeginDestroy`의 가상 `Deinitialize`를 없앴고(구독은 약참조, 타이머는 UObject에 묶임), 직접 부르는 Character·Effect·InteractionList의 `Deinitialize`만 일반 함수로 남겼다. WxUI `WxViewModelUtils`에는 `RequestImageAsync`만 남는다.
- 완료 시점 공유 조회: Character·AbilitySystem·Inventory의 `GetOrCreate`가 `IsValid(소스)` → `FindObjectWithOuter(소스, StaticClass())` → 없으면 `NewObject`·`Initialize`를 직접 쓴다. 이 구조는 [[작업 - viewmodel-mvvm-redesign]]에서 다시 바뀌었다.

## 검증 범위

- AI: build-doctor Development 빌드 성공(컴파일 경고 0), 헤드리스 위젯 BP 88개 컴파일 성공(영향 16개 포함), 프로젝트 블루프린트 571개 중 570개 성공(실패 EUB_SnapToActor와 BP_ItemPickup 경고는 기존 문제).
- AI 헤드리스 게임 임시 자동화(nullrhi 75개·RenderOffscreen 76개 통과): 스킬 슬롯·쿨다운·CheckCost·어트리뷰트 비율·버프 스택·아이콘 비동기 로드·보스 바 Deinitialize·자막·퀘스트·인디케이터·인벤토리 탭 필터, 그리고 공유 조회 테스트.
- 사람: woogle 2026-09-29 코드 리뷰 통과("테스트 결과 문제 없네요. 완료하고 제출합시다").

## 관련 주제

- [[UI 표시 구조]]

## 핵심 주장

- 2026-09-29 VM 코드 품질 정리로 베이스 UWxViewModel이 삭제돼 WxUI VM 14개가 UMVVMViewModelBase를 직접 상속하고, BeginDestroy의 가상 Deinitialize가 없어졌으며 WxViewModelUtils에는 RequestImageAsync만 남았다. ^c1
- 같은 정리에서 Setter/Getter 보일러플레이트, Ability VM의 비용 최대치 구독, 바인딩이 없는 VM 필드(CanActivate·CooldownDuration·IsAttributeEmpty·Portrait·bHasSubtitle·bHasActiveQuest·bClamped 등)가 지워졌고 Ability VM은 CheckCost만 부른다. ^c2
- woogle은 2026-09-29 공유 VM 조회 방식 검토 뒤 MVVM 원칙에 맞춘 재설계를 새 일감으로 넘기고 이 일감은 Outer 키와 FindObjectWithOuter 직접 쓰기 구조로 완료하게 했다. ^c3
- VM 코드 품질 정리는 빌드, 위젯 BP 88개 헤드리스 컴파일, 헤드리스 게임 임시 자동화로 AI가 확인했고 woogle이 2026-09-29 코드 리뷰를 통과시켜 체크리스트 5/5로 완료됐다. ^c4
