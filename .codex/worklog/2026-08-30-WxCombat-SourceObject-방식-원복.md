# WxCombat SourceObject 방식 원복

## 계획

- `UWxCombatLibrary`에 추가한 `SourceObject` 인자, 출처 유효성 검사, 정의·출처 쿼리 해제 함수를 제거한다.
- AnimNotifyState와 컷신 태스크를 변경 전의 `ApplyEffect` 및 `RemoveActiveGameplayEffectBySourceEffect` 호출로 복원한다.
- 이번 SourceObject 구현에 포함된 파일만 수정하고 기존 사용자 변경과 리뷰 문서는 보존한다.
- SourceObject 관련 코드가 남지 않았는지 확인하고 UE 5.8의 WxEditor(Development) 타겟을 빌드한다.
- 원복 후 동일 클래스 효과를 함께 제거할 수 있는 기존 이슈가 다시 미해결 상태임을 작업 기록에 명시한다.

## 완료

- `UWxCombatLibrary`의 `SourceObject` 인자, `IsValidEffectSource`, `RemoveEffect`와 정의·출처 쿼리 구현을 제거했다.
- AnimNotifyState와 컷신 태스크를 변경 전의 적용·해제 호출로 복원했다.
- 네 소스 파일의 Git 필터 적용 해시가 모두 HEAD와 일치하며, 최종 `UnrealEditor-WxCombat.dll`에 SourceObject 방식의 고유 마커가 없음을 확인했다.
- UE 5.8.2로 `WxEditor Win64 Development`를 재링크했으며 `Result: Succeeded`를 확인했다. 빌드 로그는 `.Codex/skills/build-doctor/logs/build_2026-08-30_165953.log`에 저장했다.
- 동일 클래스 효과를 함께 제거할 수 있는 최초 리뷰 이슈는 미해결 상태로 남는다.
