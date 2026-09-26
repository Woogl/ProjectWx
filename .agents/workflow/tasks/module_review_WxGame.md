# WxGame — 코드 리뷰

> 캐릭터 재초기화·사망 통지·새 게임 선택과 UI 조립 경로를 현재 소스로 대조했다. 아래 검토 범위에서는 새로운 확정적 결함을 찾지 못했다. 기존 세 지적의 수정·검증과 남은 사람 확인은 [WxGame 코드 리뷰 지적 해결](wxgame-review-fixes.md)에 있다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 0 |

## 결과

현재 미해결로 유지할 코드 지적은 없다. 기존 세 지적은 [WxGame 코드 리뷰 지적 해결](wxgame-review-fixes.md)의 수정 내용과 현재 소스를 대조해 해소를 확인했으므로 이전 정적 리뷰 원문을 제거한다.

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Controller/WxNameplateManagerComponent.h`, `Source/WxGame/Controller/WxNameplateManagerComponent.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Battle/WxBattleSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_AbilitySystem.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Quest.cpp`이다.
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`이다. 헤더의 인라인 정의를 검색했다.
- **교차 근거**: 기존 재등록 지적의 해결 여부 확인에 한해 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:48`과 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:58`의 최초 속성·GE 초기화와 누락 스펙 부여 분리를 대조했다.
- **미검토 / 한계**: 2026-09-26 작업 트리의 정적 리뷰이다. 소스 61개는 생성물 제외 `.h`·`.cpp` 개수이며 전 파일 정밀 검토를 뜻하지 않는다. 이번 리뷰에서 빌드·자동화 테스트·게임 실행을 다시 수행하지 않았으며, BP/WBP·DataTable·BT·StateTree 내부 및 멀티플레이 동작은 검증하지 않았다. 빌드·회귀 결과는 [WxGame 코드 리뷰 지적 해결](wxgame-review-fixes.md)의 검증 이력이다.

---
*문서 기준 커밋 `ad0db6de0` · 리뷰일 2026-09-26 · 소스 61파일 — `/module-review`로 갱신*
