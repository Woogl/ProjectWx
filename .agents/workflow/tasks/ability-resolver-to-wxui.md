# Ability Resolver를 WxUI로 이동

- 날짜: 2026-09-23
- 요청: `UWxViewModelResolver_Ability`를 WxUI로 이동.
- 조사: 참조하는 Ability·AbilitySystem VM은 이미 WxUI에 있고 필요한 GAS·MVVM 의존성도 등록되어 있다.
- 구현: 사용자 추가 지시에 따라 WxUI의 WxViewModel_Ability.h/.cpp에 Resolver 선언·구현을 통합하고 export를 WXUI_API로 변경했다. 기존 별도 Resolver 파일은 제거했으며 동작은 유지한다.
- 호환성: DefaultEngine.ini에 `/Script/WxGame.WxViewModelResolver_Ability` → `/Script/WxUI.WxViewModelResolver_Ability` ClassRedirect를 추가했다. 에셋 재저장·실행 확인 전 제거하지 않는다.
  - 2026-09-24 제거: 사용자 지시("오래된 리디렉터 제거 진행합시다")로 `DefaultEngine.ini`의 `[CoreRedirects]`를 모두 비웠다. 리다이렉트가 없어도 참조 WBP 6개(`WBP_Ability`·`WBP_PlayerSkills`·`WBP_ItemQuickSlot`·`WBP_QuestTracker`·`WBP_QuestObjective`·`WBP_DialogueScreen`)가 누락 클래스 경고 없이 로드되고 컴파일 오류·경고가 0이었다. 인게임 표시는 확인하지 않았다. 근거: [Nameplate 작업 자료](nameplate-manager.md)의 "오래된 CoreRedirects 제거" 절.
- 검증: diff 공백 검사 통과. 최종 통합 코드의 WxEditor Win64 Development 빌드 성공(UHT·WxViewModel_Ability.cpp 컴파일·WxUI 링크, exit 0). 로그: Saved/Logs/BuildDoctor/build_2026-09-23_114057_482_45528.log.
- 미확인: 기존 WBP 로드와 실제 슬롯 표시. 인간 리뷰·실행 수용은 아직 받지 않았다.

## 후속 검토: 나머지 VM의 WxUI 이동 가능성

**아래 의존성 추가 제안은 사용자 원칙 확정으로 철회했다.** Wx 기능 모듈 간 신규 의존성은 WxCore를 제외하면 금지하며, WxCore를 게임 로직·과도한 중계 계약의 집합으로 만들지 않는다. 도메인 연결은 WxGame에 유지한다. Dialogue의 순수 표시 데이터 분리는 [별도 작업](dialogue-presentation-vm.md)에서 진행한다.

2026-09-23 사용자 요청으로 현재 HEAD `58f01692c` 및 작업 트리의 WxGame/MVVM 전체 14개 소스 파일과 관련 Build.cs·플러그인 선언을 정적으로 확인했다. 이동 구현은 요청받지 않았으며 소스·Wiki는 수정하지 않았다.

| 대상 | 판단 | 필요한 변경 |
|---|---|---|
| QuestObjective | 추가 의존성 없이 가능 | 공개 헤더·구현 이동, WXUI_API, 클래스 경로 리다이렉트 |
| Dialogue + Resolver | 도메인 의존성 추가 시 가능 | WxUI → WxDialogue |
| Quest + Resolver | 도메인 의존성 추가 시 가능 | WxUI → WxQuest; QuestObjective와 함께 배치 가능 |
| Inventory + Inventory/Item Resolver | 도메인 의존성 추가 시 가능 | WxUI → WxInventory; 공개 헤더가 ItemDefinition을 포함하므로 Public 의존성 필요 |
| InteractionList + Resolver | 도메인 의존성 추가 시 가능 | WxUI → WxWorld |
| BossDisplay + BossCharacter Resolver | 현재 형태의 단순 이동 불가 | WxGame의 AWxEnemyCharacter 및 정적 보스 교전 이벤트 의존성을 먼저 분리 |

현재 네 도메인은 WxUI나 WxGame을 의존하지 않아 위 방향 추가 자체로 순환은 생기지 않는다. Build.cs와 WxUI.uplugin 모두 갱신해야 한다. 다만 WxUI가 공용 UI 기반에서 도메인 화면 조립까지 책임지는 모듈로 넓어지는 설계 선택이다. 기존 InteractionList·Quest 주석은 현행 경계를 설명하며 기술적 이동 불가를 뜻하지 않는다.

제안: UI를 한 모듈로 모으려는 목적이면 보스 계열을 제외한 위 대상을 이동할 수 있다. 공용 UI 독립성을 유지하려면 QuestObjective만 우선 이동하고 나머지 조립은 유지한다. BossCharacter Resolver만 단독 이동하면 BossDisplay를 통해 WxGame 역의존성이 생기므로 해결되지 않는다.

> 후속(2026-09-23): 보스 계열의 위 제약은 [보스 표시 VM 단순화](boss-display-simplification.md)에서 해소되었다. BossDisplay VM과 정적 교전 이벤트를 제거했고, 현재 구조는 모델(`UWxBattleSubsystem`)·연결(WxGame 리졸버)·VM(WxUI `UWxViewModel_Character`)이다.

이동 시 Resolver는 해당 VM 파일에 통합하는 사용자 관례를 유지한다. 모듈 경로가 바뀌는 모든 UCLASS의 리다이렉트, C++ 빌드, 영향받는 WBP 로드·컴파일을 검증해야 한다. 이번 검토는 빌드·에셋 실행 검증이 아니다.
