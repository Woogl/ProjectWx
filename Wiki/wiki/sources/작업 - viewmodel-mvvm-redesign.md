---
type: source
title: "작업 - viewmodel-mvvm-redesign"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "MVVM"
  - "WxUI"
  - "설계-원칙"
summary: "woogle이 채택한 UI 설계 원칙에 따라 플레이어 공유 VM을 로컬 PC가 엔진 Global Collection에 등록하고 Inventory VM을 WxUI로 옮기며 FindObjectWithOuter 조회를 없앤 2026-09-30 완료 작업 기록"
source_type: task-record
source_id: src-f614d7f1449876a92d86
sha256: a73f18be2decf23caf539dcffc0d834b142f03cb27c3ef381aac9901a7336a88
authority: primary
independence_key: ".agents/workflow/tasks/viewmodel-mvvm-redesign.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/viewmodel-mvvm-redesign.md"
raw_copy: ".raw/captured/a73f18be2decf23caf539dcffc0d834b142f03cb27c3ef381aac9901a7336a88.md"
claim_ids:
  - clm-a73f18be2d-c1
  - clm-a73f18be2d-c2
  - clm-a73f18be2d-c3
  - clm-a73f18be2d-c4
  - clm-a73f18be2d-c5
key_claims:
  - "woogle은 2026-09-30 모델→뷰모델→뷰 층, 모든 뷰모델·위젯의 WxUI 배치와 도메인 타입 금지, 조립 층(PC·리졸버·네임플레이트 관리자) 역할 분담, 소스를 열쇠로 뷰모델을 찾는 저장소 금지를 담은 UI 설계 원칙을 프로젝트 원칙으로 채택했다."
  - "2026-09-30 재설계 뒤 로컬 AWxPlayerController가 Character·Inventory 뷰모델을 한 번 만들어 엔진 Global Collection에 VM_PlayerCharacter·VM_Inventory로 등록하고 EndPlay에서 빼며, UWxViewModel_Inventory는 WxUI로 옮겨졌고 인벤토리·PlayerCharacter·AbilitySystem 리졸버는 삭제됐다."
  - "같은 재설계에서 WBP는 플레이어 공유 VM을 이름 문자열 대신 WxUI UWxViewModelResolver_Player로 받고, 인벤토리 탭 키는 Item.Category.* 게임플레이 태그이며, MVVM 코드의 FindObjectWithOuter 조회는 모두 사라졌다."
  - "자막 뷰모델(StateTree 노드의 직접 호출)과 WxGame의 InteractionList 뷰모델은 2026-09-30 UI 설계 원칙의 알려진 예외로 후속 일감에 남았다."
  - "MVVM 재설계는 빌드, 위젯 BP 88개 헤드리스 컴파일, 리슨 서버·단독 헤드리스 에디터 PIE 임시 테스트로 AI가 확인했고 woogle이 2026-09-29 코드 리뷰와 HUD·인벤토리 표시를 통과시켜 체크리스트 11/11로 완료됐다."
---

# 작업 - viewmodel-mvvm-redesign

- 원본: `.agents/workflow/tasks/viewmodel-mvvm-redesign.md`
- 원자료 사본: `.raw/captured/a73f18be2decf23caf539dcffc0d834b142f03cb27c3ef381aac9901a7336a88.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

viewmodel-quality-cleanup 코드 리뷰에서 나온 MVVM 재설계를 새 일감으로 만든 작업 기록이다(woogle 2026-09-29). 정하기에서 Q1~Q3은 woogle이 "가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요."로 AI에 맡겼고, 2026-09-30 원칙 논의에서 UI 설계 원칙을 채택하며 Q1·Q2·Q5의 처음 답을 바꿨다. 상태는 완료(체크리스트 11/11 통과)다. 원칙 전문은 원자료 사본의 구현 계획 절에 있다.

## UI 설계 원칙(woogle 채택 2026-09-30, 요약)

- woogle 원문: "네 이것을 우리 프로젝트의 UI 설계 원칙으로 합시다."
- 층: 모델 → 뷰모델 → 뷰. 모델은 뷰모델을, 뷰모델은 위젯을 모른다. 조립 층(PC, 리졸버, 네임플레이트 관리 컴포넌트)이 모델을 관찰해 뷰모델에 값을 넣고 뷰에 넣으며 수명을 맞춘다.
- 모듈: 모든 뷰모델 클래스와 위젯은 WxUI에 두고 도메인 타입을 쓰지 않는다(엔진 타입은 직접 관찰 가능). 도메인 플러그인은 모델, WxGame은 조립 층이다.
- 역할 분담: 위젯마다 만드는 뷰모델(Quest·Dialogue·보스 바)은 리졸버가, 플레이어 공유 뷰모델(Character·Inventory)은 로컬 PC가, 월드 공간 위젯의 뷰모델(적 네임플레이트)은 위젯을 만든 관리자가 만든다.
- 소유와 수명: 부모가 자식을 UPROPERTY로 소유하고 Outer는 소유가 아니다. 소스(모델)를 열쇠로 뷰모델을 찾는 저장소는 두지 않는다. 로컬 `AWxPlayerController`가 Character·Inventory 뷰모델을 한 번만 만들어 엔진 Global Collection에 `VM_PlayerCharacter`·`VM_Inventory`로 등록하고(Outer는 컬렉션, `Super::BeginPlay` 앞 등록·`EndPlay` 제거, 원격 PC는 등록 안 함), 빙의가 바뀌면 같은 Character 뷰모델을 새 ASC로 다시 초기화한다. 로컬 플레이어는 하나로 전제한다("화면 분할은 대비 안해도 되요").
- 조회: 조회로 얻는 뷰모델은 Global Collection에 둔 것뿐이다. WBP는 플레이어 리졸버 `UWxViewModelResolver_Player`(WxUI)로 받아 이름 문자열을 쓰지 않는다(2026-09-30 코드 리뷰 중 결정: "VM_PlayerCharacter, VM_Inventory 관련해서 리졸버를 만들면 이름 문자열 이슈 해결되지 않나요?" → "네. 그렇게 합시다"). 코드는 각 뷰모델 클래스의 정적 조회 함수로 받는다.
- 그 밖에 받는 법(Context·PropertyPath·목록 확장·Manual), 리졸버는 배달만 하고 상태를 두지 않음, 공유 기준, 위젯에는 바인딩·변환 함수만 두고 입력은 `Request~` 명령과 네이티브 델리게이트로 보냄, 뷰모델은 모델을 약참조로 듦이 원칙에 적혀 있다.
- 알려진 예외(후속 일감): 자막은 StateTree 노드가 뷰모델을 직접 호출해 층 원칙에 어긋나고("퀘스트나 자막은 나중에 봅시다."), InteractionList 뷰모델은 WxGame에 있고 도메인 타입을 써 모듈 원칙에 어긋난다.
- woogle 2026-09-30 보류 판단: "1번은 나중에 문제가 되었을 때 고민합시다. 2번은 방식 정해두죠. 3번은 필요할 때 mcp 툴 만드세요. 4번은 필요한 뷰모델에서 그렇게 쓰면 될 것 같아요. 5번은 작업할 때 문제가 되는 수준이라면 그렇게 하세요. 6번도 작업하면서 해결하죠."

## 질문과 바뀐 결정

- Q1 루트 VM 모듈: 처음에는 WxGame에 두기로 했으나, 뷰모델을 모두 WxUI에 두기로 하면서 Inventory VM도 WxUI로 옮겼다.
- Q2 보관: 처음 계획(PC Outer + 루트 리졸버)은 Outer가 GC에서 객체를 살려 두지 않아 HUD가 없는 틈에 VM이 수거될 수 있어, 엔진 Global Collection에 PC가 등록하는 방식으로 바꿨다("그냥 PlayerController가 뷰모델 만들어서 엔진 Global Collection에 등록, 제거 하는게 훨씬 더 단순하겠네요"). `UMVVMGameSubsystem` 상속은 위젯 쪽 조회가 기본 클래스로 고정돼 쓰지 않는다.
- Q3: 적·보스의 Character VM은 그 뷰를 만드는 쪽이 만들어 넣는다. Q4: 공유 VM만 범위에 넣는다.
- Q5: 처음 "만든다"였던 루트 `UWxViewModel_Player`는 공유 VM을 각자 등록하기로 해("B로 하죠.") 만들지 않았다.
- Q6 인벤토리 탭 키: 게임플레이 태그(WxCore `Item.Category.*`)로 바꾼다("태그로 합시다.", 2026-09-30).

## 구현 결과

- `UWxViewModel_Character`: `GetOrCreate`를 지우고 `Initialize(ASC, 이름)`에서 자기 AbilitySystem VM을 만들어 소유한다. `UWxViewModel_AbilitySystem`도 `GetOrCreate`를 지웠다.
- `UWxViewModel_Inventory`를 WxGame에서 WxUI로 옮기고 도메인 타입을 빼 값 받기와 탭 필터만 남겼다. 탭 키는 `Item.Category.*` 태그다. Item 리졸버는 도메인 키를 쓰는 조립 층이라 WxGame 새 파일 `WxViewModelResolver_Item`에 둔다.
- `AWxPlayerController`가 두 VM 등록·재초기화·인벤토리 값 넣기를 맡고, 인벤토리 컴포넌트의 전역 준비·종료 통지(`OnAnyInventoryReady`·`OnAnyInventoryEnded`)는 지웠다. Character VM 초기화와 이펙트 표시 연결은 WxGame 공용 함수로 옮겼다(`InitializeCharacter`를 VM에 두지 않는 이유는 순환 의존·모듈 원칙).
- 삭제: `UWxViewModelResolver_AbilitySystem`·`UWxViewModelResolver_PlayerCharacter`·`UWxViewModelResolver_Inventory`. 새 `UWxViewModelResolver_Player`(WxUI). 네임플레이트 관리자와 보스 바 리졸버는 위젯마다 Character VM을 만든다. MVVM 코드에서 `FindObjectWithOuter`가 모두 사라졌다.
- WBP_Nameplate_Player·WBP_PlayerSkills·WBP_AcquiredItemList·WBP_Inventory가 플레이어 리졸버로 받고, 빙의 해제로 비는 것은 정상 흐름이라 `VM_PlayerCharacter`를 선택적으로 두었다. WBP_Inventory 탭 버튼 그래프는 enum에서 태그로 바뀌었다.

## 검증 범위

- AI: DebugGame·Development 빌드 성공, 위젯 BP 88개 헤드리스 컴파일(0 error·0 warning), 리다이렉트는 WBP 리세이브 뒤 SAFE 판정으로 제거, 지운 리졸버를 참조하는 에셋 0건, `FindObjectWithOuter` 0건·WxUI MVVM의 도메인 include 0건.
- AI 헤드리스 에디터 PIE 임시 테스트: 리슨 서버 + 원격 클라이언트에서 각자 자기 두 VM 등록·서버의 원격 PC 미등록, 단독 PIE에서 등록본 공유·인벤토리 탭 유지·HUD를 걷은 뒤 GC 강제에도 생존, 리졸버 값 반영(쿨다운·UP·아이템), 빙의 교체 경로로 재현한 부활 뒤 같은 Character VM 재초기화, 맵 이동 뒤 재등록과 옛 월드 해제, 네임플레이트·보스 바 값. 부활은 사망 어빌리티가 아니라 빙의 교체로 재현했다.
- 사람: woogle 2026-09-29 코드 리뷰, HUD·인벤토리 표시 통과.
- 무관한 기존 경고: AcquiredItemList의 "Cannot add null item into ListView", ST_Quest_Main1의 없는 태그 Quest.Fail.

## 관련 주제

- [[UI 표시 구조]]
- [[모듈 구조와 코드 정리]]
- [[아이템과 회복]]

## 핵심 주장

- woogle은 2026-09-30 모델→뷰모델→뷰 층, 모든 뷰모델·위젯의 WxUI 배치와 도메인 타입 금지, 조립 층(PC·리졸버·네임플레이트 관리자) 역할 분담, 소스를 열쇠로 뷰모델을 찾는 저장소 금지를 담은 UI 설계 원칙을 프로젝트 원칙으로 채택했다. ^c1
- 2026-09-30 재설계 뒤 로컬 AWxPlayerController가 Character·Inventory 뷰모델을 한 번 만들어 엔진 Global Collection에 VM_PlayerCharacter·VM_Inventory로 등록하고 EndPlay에서 빼며, UWxViewModel_Inventory는 WxUI로 옮겨졌고 인벤토리·PlayerCharacter·AbilitySystem 리졸버는 삭제됐다. ^c2
- 같은 재설계에서 WBP는 플레이어 공유 VM을 이름 문자열 대신 WxUI UWxViewModelResolver_Player로 받고, 인벤토리 탭 키는 Item.Category.* 게임플레이 태그이며, MVVM 코드의 FindObjectWithOuter 조회는 모두 사라졌다. ^c3
- 자막 뷰모델(StateTree 노드의 직접 호출)과 WxGame의 InteractionList 뷰모델은 2026-09-30 UI 설계 원칙의 알려진 예외로 후속 일감에 남았다. ^c4
- MVVM 재설계는 빌드, 위젯 BP 88개 헤드리스 컴파일, 리슨 서버·단독 헤드리스 에디터 PIE 임시 테스트로 AI가 확인했고 woogle이 2026-09-29 코드 리뷰와 HUD·인벤토리 표시를 통과시켜 체크리스트 11/11로 완료됐다. ^c5
