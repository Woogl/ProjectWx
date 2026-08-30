# State.InCombat 태그 제거

## 계획

- `State.InCombat`의 생산·소비 경로를 제거하고 `AWxEnemyCharacter::bHasAITarget`을 명시적으로 복제되는 단일 인식 상태로 둔다.
- `UWxAIPerceptionComponent::SetTargetActor`의 변경 신호를 `AWxEnemyController`가 현재 폰에 전달하고, 언빙의 전에는 이전 폰의 값을 `false`로 내린다.
- 뒤잡 자격은 `bHasAITarget`을 직접 읽어 판정한다.
- 일반 적 네임플레이트의 범용 `VisibilityRequirements`·ASC 캐시·태그 구독 계층을 제거하고, 게임 통합 계층인 `AWxEnemyCharacter` 한 곳에서 `!Dead && (bHasAITarget || State.LockedOn)`을 계산한다.
- 보스 HUD의 직렬화된 `State.InCombat` 바인딩을 같은 `bHasAITarget` 뷰모델 필드로 교체한다.
- 네이티브 GameplayTag 선언·정의와 모든 소스·에셋 참조를 제거한다.
- 코드와 에셋을 한 빌드로 함께 배포하는 로컬 프로젝트이므로 혼합 버전 호환 구간은 두지 않는다. 롤백은 이 변경의 코드와 보스 위젯 에셋을 함께 되돌리는 단일 단위로 유지한다.
- 관련 참조가 0건인지 확인하고 WxEditor(Development) 타겟을 빌드해 컴파일을 검증한다.

## 완료

- `State.InCombat` 네이티브 태그와 AI Perception의 태그 생산 코드를 제거했다.
- `UWxAIPerceptionComponent::SetTargetActor` 변경을 `AWxEnemyController`가 현재 적 폰의 복제 상태 `bHasAITarget`에 전달하며, `OnUnPossess`에서 이전 폰을 즉시 `false`로 정리한다.
- 뒤잡 판정과 일반 적 네임플레이트 표시를 `AWxEnemyCharacter`의 단일 상태 계산으로 모았다.
- `UWxNameplateComponent`의 범용 표시 요건·ASC 캐시·태그 구독 계층을 제거해 표시/뷰모델 컴포넌트로 단순화했다.
- 보스 뷰모델에 `bHasAITarget` FieldNotify를 추가하고 `WBP_Nameplate_Boss`의 ViewModel 선언 타입과 Visibility 바인딩을 해당 필드 및 엔진 기본 `Conv_BoolToSlateVisibility`로 교체했다.
- `State.InCombat` 소스·UI 자산 참조가 0건임을 확인했고, 보스 위젯은 에디터 컴파일 상태 `BS_UP_TO_DATE`를 확인했다.
- 대상 C++ 변경은 117줄 추가·164줄 삭제로 47줄 순감소했다.
- UE 5.8 `WxEditor Win64 Development` 전체 빌드 성공: `.Codex/skills/build-doctor/logs/build_2026-08-31_023600.log`.
