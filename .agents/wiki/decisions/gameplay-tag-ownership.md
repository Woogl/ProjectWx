# 네이티브 GameplayTag 선언은 WxCore 단독

상태: current · 범위: 코드 구조 결정 · 결정일 2026-09-21

2026-09-21 사용자가 WxCore 코드 리뷰의 "‘태그는 이 파일에만 작성’ 규약이 이미 깨져 있다" 항목에 대해 “관련해서 원칙대로 태그 정의를 옮겨주세요”라고 확정했다. 이 문서는 해당 대화의 수기 기록이며 별도 대화 내보내기 파일은 없다. 규약 문구를 예외를 인정하는 쪽으로 고치는 대안은 채택하지 않았다. 미결정으로 추적하던 Q-004(WxCombat 태그 선언의 WxCore 통합 여부)는 이 결정으로 종료했다.

- 프로젝트의 C++ Native GameplayTag는 [WxGameplayTags.h](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h)와 [WxGameplayTags.cpp](../../../Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp)에만 선언·정의한다.
- 도메인 플러그인은 자체 태그 네임스페이스를 두지 않는다. 한 모듈 안에서만 쓰는 태그도 WxCore에 둔다.
- 헤더 첫 줄의 "태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성" 규약을 예외 없이 유지한다.

적용 결과 `WxCombatGameplayTags::Effect_IgnoreAbilityTags` 선언·정의를 `WxGameplayTags`로 옮기고 `WxCombatGameplayTags` 네임스페이스를 제거했다. 태그 문자열 `Effect.IgnoreAbilityTags`는 그대로이므로 에셋 참조와 부여·소비 동작은 바뀌지 않는다. 관련: [WxCore](../modules/WxCore.md), [WxCombat](../modules/WxCombat.md).
