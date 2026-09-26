---
type: source
title: "결정 노트 - 2026-09-23-boss-battle-three-layer"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "보스"
  - "UI"
  - "MVVM"
summary: "보스 표시를 UWxBattleSubsystem·WxGame 리졸버·WxUI Character VM 세 층으로 재구성하고 보스 식별을 IdentityTags로 바꾼 2026-09-23 결정"
source_type: decision-note
source_id: src-78c4cedfc439fa0f50a7
sha256: 14d52454d36745148fb80fabf1c78c036caace5d70b2ed2dd833251b94d566be
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-boss-battle-three-layer.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-boss-battle-three-layer.md"
raw_copy: ".raw/captured/14d52454d36745148fb80fabf1c78c036caace5d70b2ed2dd833251b94d566be.md"
claim_ids:
  - clm-ee4d82d3a7-c1
  - clm-ee4d82d3a7-c2
  - clm-ee4d82d3a7-c3
  - clm-ee4d82d3a7-c4
key_claims:
  - "2026-09-23 사용자 결정으로 보스 표시는 모델(UWxBattleSubsystem)·연결(WxGame 리졸버)·VM(WxUI UWxViewModel_Character) 세 층으로 나뉘었다."
  - "2026-09-23 결정으로 보스 식별은 bIsBoss 플래그 대신 AWxCharacterBase::IdentityTags의 Character.Boss 태그로 한다."
  - "UWxBattleSubsystem은 먼저 교전한 보스를 현재 보스로 유지하고 현재 보스가 바뀔 때만 OnCurrentBossChanged를 발행한다."
  - "보스 표시 세 층 구조는 빌드와 WBP 컴파일까지 확인됐지만 보스 콘텐츠가 없어 인게임 표시는 검증되지 않았다."
---

# 결정 노트 - 2026-09-23-boss-battle-three-layer

- 원본: `.wiki/raw/notes/2026-09-23-boss-battle-three-layer.md`
- 원자료 사본: `.raw/captured/14d52454d36745148fb80fabf1c78c036caace5d70b2ed2dd833251b94d566be.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-23-boss-battle-three-layer.md`(제목 "보스 표시 세 층 구조와 전투 서브시스템").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-23`, 태그 `wx, ui, combat, game, lifecycle, architecture`.
- 성격: 2026-09-23 **사용자 확정 결정**과 커밋 `ac723132d`·`4352e9100`의 **정적 확인**, 로컬 UE 5.8 엔진 소스 확인 사실, 알려진 기존 문제, WBP 전환 도구 기록. 조사 시점 기준이라 현재 코드와 다를 수 있다.
- 대체한 이전 구조: `UWxViewModel_BossDisplay`가 정적 보스 교전 이벤트 구독·후보 목록·액터 순회·월드 필터를 모두 떠안던 방식.

## 확정 결정 (사용자)

노트는 결정을 요약 목록으로 기록한다(따옴표 원문 없음).

- VM은 전부 WxUI에 모은다. 도메인 데이터가 필요한 표시는 세 층으로 나눈다.
  - **모델**: WxGame·도메인. VM·MVVM을 모른다.
  - **연결**: WxGame 리졸버. VM이 아니다.
  - **VM**: WxUI. 순수 표시만 담는다.
- 보스 식별은 bool 플래그·전용 클래스 대신 `AWxCharacterBase::IdentityTags`(`Character.*` GameplayTag)로 한다.
- 보스전 상황은 월드 서브시스템 `UWxBattleSubsystem`이 순수 모델로 소유한다. UIManager는 보스 VM을 모른다.
- 전용 위젯 C++ 클래스를 만들지 않고 기존 `WBP_Nameplate_Boss`를 쓴다. `UWxNameplateComponent`는 머리 위 표시용이라 HUD 게시 역할을 얹지 않는다.

## 구현 관찰 (정적)

- WxCore에 `Character.Boss` 태그 추가(`Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`).
- `AWxCharacterBase::IdentityTags`(EditDefaultsOnly, Categories=Character): `PostInitializeComponents`에서 태그마다 `SetLooseGameplayTagCount(Tag, 1)`로 ASC에 loose 태그를 올린다. 재실행돼도 결과가 같다.
- `AWxEnemyCharacter`: `RefreshEngagement`·`EndPlay`가 `State.Engaged`를 갱신한 뒤 `UWxBattleSubsystem::NotifyEngagementChanged`를 호출한다. `bIsBoss`·`IsBoss`·정적 `OnAnyBossEngagementChanged`는 제거.
- `UWxBattleSubsystem`: `Character.Boss`를 가진 캐릭터만 교전 순서 목록에 모으고, 보스 판정은 추가 시점에만 한다. `GetCurrentBoss()`는 맨 앞 보스, `OnCurrentBossChanged`는 현재 보스가 바뀔 때만 발행. 먼저 교전한 보스를 유지하고 빠지면 다음으로 넘어간다. 교전 상태는 머신마다 계산되므로 복제 없이 각 머신이 모은다.
- `WxViewModelResolver_BossCharacter`: `CreateInstance`가 위젯을 Outer로 WxUI `UWxViewModel_Character`를 만들고 `FDelegate::CreateWeakLambda(VM, ...)`로 서브시스템을 구독, 현재 보스를 즉시 한 번 반영한다. `DestroyInstance`는 `RemoveAll(ViewModel)`로 그 VM의 구독만 끊는다. 리졸버는 위젯 클래스가 공유하는 const 객체라 상태를 들지 않는다(08c73f513의 `RemoveAll(this)` 버그 재발 방지).
- `WxViewModel_BossDisplay` 삭제. `WBP_Nameplate_Boss`의 VM `VM_BossCharacter`를 `UWxViewModel_Character`·Resolver 생성으로 바꾸고 5개 바인딩에서 `Character` 단계를 뺐다. 부모는 `UserWidget` 유지.

## 검증 범위

- WxEditor Development **빌드 성공**.
- 새 프로세스에서 WBP 로드와 경고를 오류로 취급한 컴파일 통과(`Saved/Logs/BossNameplateVerify.log`).
- **인게임 표시는 미검증**. 보스 콘텐츠가 없고 `BP_Boss`는 e0e3ecc51에서 삭제됐다.

## 엔진 확인 사실 (로컬 UE 5.8 소스)

- 다른 서브시스템의 `Initialize` 안에서 `GetSubsystem`을 부르면 요청한 서브시스템을 즉시 초기화하므로 `InitializeDependency`는 불필요하다.
- 스트리밍 레벨이 숨겨졌다 다시 보이면 `bActorInitialized=false`가 되고 재추가 시 `PostInitializeComponents`가 다시 실행된다.
- MVVM 리졸버는 위젯 클래스가 공유하고 뷰마다 `CreateInstance`/`DestroyInstance`를 부른다.

## 미결정·충돌 (알려진 기존 문제, 미수정)

- `AWxCharacterBase::PostInitializeComponents`의 사망·래그돌 `RegisterGameplayTagEvent(...).AddUObject` 구독이 레벨이 다시 보일 때 중복된다. 그러면 `OnDeath`가 두 번 발행되고 `AWxEnemyCharacter::HandleOwnerDeath`가 두 번 돌아 보상이 두 번 지급될 수 있다. 노트는 별도 작업 대상으로 남겼다.

## 도구: `WxMVVMToolset.SetBindingSourcePath` (커밋 ac723132d)

- ArgumentName이 비면 바인딩 소스 경로를 바꾸고(기존 변환 함수는 제거), 지정하면 Source→Destination 변환 함수 인자 하나의 경로만 바꾼다. 경로 형식은 "뷰모델이름.필드[.필드...]" 또는 "Self.필드". Python에서도 호출 가능. D→S 방향은 미지원.
- 함정: VM 클래스를 먼저 지운 채 WBP를 로드하면 스켈레톤에 VM 프로퍼티가 없어 `Could not find root property` ensure가 난다. VM 재지정(`ReparentViewModel`) → 한 번 컴파일 → 경로 변경 순서가 깨끗하다.

## 관련 주제

- [[보스 전투]]
- [[UI 표시 구조]]
- [[에디터 도구]]

## 핵심 주장

- 2026-09-23 사용자 결정으로 보스 표시는 모델(UWxBattleSubsystem)·연결(WxGame 리졸버)·VM(WxUI UWxViewModel_Character) 세 층으로 나뉘었다. ^c1
- 2026-09-23 결정으로 보스 식별은 bIsBoss 플래그 대신 AWxCharacterBase::IdentityTags의 Character.Boss 태그로 한다. ^c2
- UWxBattleSubsystem은 먼저 교전한 보스를 현재 보스로 유지하고 현재 보스가 바뀔 때만 OnCurrentBossChanged를 발행한다. ^c3
- 보스 표시 세 층 구조는 빌드와 WBP 컴파일까지 확인됐지만 보스 콘텐츠가 없어 인게임 표시는 검증되지 않았다. ^c4
