# Effect Savable 태그 변경

## 계획

- 네이티브 C++ 태그 심볼을 `Gameplay_Effect_Persistable`에서 `Effect_Savable`로 변경한다.
- 실제 Gameplay Tag 경로를 `Gameplay.Effect.Persistable`에서 `Effect.Savable`로 변경하고 기존 경로 redirect를 추가한다.
- ASC 참조, 문서, 작업 기록 및 기존 에셋 태그 참조를 갱신한다.
- 잔존 참조를 검사하고 가능한 범위에서 UE 5.8 `WxEditor Development` 빌드를 검증한다.

## 완료

- 네이티브 C++ 태그 심볼을 `Effect_Savable`, 실제 태그 경로를 `Effect.Savable`로 변경하고 ASC의 저장 대상 판정과 제거 필터를 함께 갱신했다.
- `Config/DefaultGameplayTags.ini`에 `Gameplay.Effect.Persistable`에서 `Effect.Savable`로 향하는 Gameplay Tag redirect를 추가해 기존 직렬화 참조의 호환성을 유지했다.
- 관련 문서와 기존 영속화 작업 기록을 새 태그명에 맞췄다. 관련 `.uasset`/`.umap`에서는 구 태그 문자열이 발견되지 않았다.
- UE 5.8 `WxEditor Win64 Development` 빌드가 43개 액션을 수행해 성공했다. 로그는 `.Codex/skills/build-doctor/logs/build_2026-08-31_214955.log`에 남겼다.
- `/Game/Framework/WAS_CoreGameplay` ResavePackages 검증이 `0 error`로 성공했으며 Gameplay Tag 설정·redirect 오류가 없음을 확인했다.
