# WxCombat 구간형 Gameplay Effect 소유 해제

## 계획

- 구간형 Gameplay Effect 적용 시 네트워크에서 안정적으로 식별할 수 있는 `SourceObject`를 EffectContext에 기록한다.
- Gameplay Effect 정의와 적용 출처가 모두 일치하는 인스턴스만 제거하는 공용 해제 경로를 추가한다.
- AnimNotifyState는 에셋에 저장된 노티파이 객체, 컷신 태스크는 LevelSequence 에셋을 출처로 사용해 서로의 동일 클래스 효과를 제거하지 않도록 한다.
- 출처가 없거나 네트워크에서 식별할 수 없으면 효과를 적용하지 않아 해제할 수 없는 상태가 생기지 않도록 한다.
- 기존 광역 클래스 해제 호출이 사라졌는지 확인하고 UE 5.8의 WxEditor(Development) 타겟을 빌드해 검증한다.

## 완료

- `UWxCombatLibrary::ApplyEffect`가 네트워크 식별 가능한 `SourceObject`를 EffectContext에 기록하도록 확장했다.
- Gameplay Effect 정의와 출처가 모두 일치하는 활성 효과만 제거하는 `UWxCombatLibrary::RemoveEffect`를 추가했다.
- `UWxAnimNotifyState_ApplyGameplayEffect`는 해당 노티파이 객체를, `UWxAbilityTask_PlaySkillCutscene`은 LevelSequence 에셋을 출처로 사용하도록 전환했다.
- 컷신이 적용 전에 취소되는 정상 경로에서는 null 출처를 조용히 무시하고, 네트워크 미지원 출처만 `ensure`로 진단하도록 했다.
- `WxCombat`에서 `RemoveActiveGameplayEffectBySourceEffect` 호출이 사라졌고 `git diff --check`를 통과했다.
- UE 5.8.2로 `WxEditor Win64 Development`를 컴파일·링크했으며 `Result: Succeeded`를 확인했다. 빌드 로그는 `.Codex/skills/build-doctor/logs/build_2026-08-30_164343.log`에 저장했다.

## 후속 상태

- 사용자 요청에 따라 이 문서의 SourceObject 기반 구현은 전부 원복했다.
- 소스는 구현 전 상태로 복원됐으며, 동일 클래스 효과를 함께 제거할 수 있는 최초 이슈는 다시 미해결 상태이다.
- 원복 내역과 검증 결과는 `2026-08-30-WxCombat-SourceObject-방식-원복.md`에 기록했다.
