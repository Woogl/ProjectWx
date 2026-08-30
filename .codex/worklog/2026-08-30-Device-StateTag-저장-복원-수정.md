# Device StateTag 저장·복원 수정

## 계획

- `StateTag`를 LSP public property-bag 방식으로 저장해 `FGameplayTag` 내부 값까지 온전히 직렬화한다.
- 체크포인트처럼 상태 전이와 같은 틱에 저장하는 경우 이전 `StateTag`가 기록되지 않도록 저장 직전 권위 StateTree 상태를 동기화한다.
- 새 슬롯 기준으로 문·상자·체크포인트·엘리베이터·피스톤의 상태 저장·로드 경로를 검증한다.
- WxEditor(Development) 타겟을 빌드해 컴파일을 확인한다.

## 완료

- UE 5.8 LSP가 private 속성뿐 아니라 public property-bag에서도 `FGameplayTag` 내부 `TagName`을 복원하지 못하는 것을 실제 직렬화 왕복으로 확인했다.
- Device의 저장·복제 필드를 단일 `FName StateTagName`으로 단순화하고, 외부 `FGameplayTag` API와 StateTree 비교 지점에서만 변환하도록 변경했다.
- `StateTagName`을 LSP public property로 등록하고 저장 포맷을 3으로 올려 기존의 잘못된 슬롯은 새 저장으로 초기화되게 했다.
- 체크포인트 StateTree 태스크의 저장을 다음 월드 틱으로 미뤄, 같은 틱의 새 활성 상태가 컴포넌트에 발행된 뒤 LSP가 플러시되게 했다.
- 실제 WxSave 강제 플러시와 같은 맵 서버 트래블로 문(Open), 상자(Open), 체크포인트(Lit), 엘리베이터(2F), 피스톤(Off)의 저장·복원 및 StateTree 추종 전이 요청을 확인했다. 테스트 중 `Default.sav`는 백업 후 원본으로 복구했다.
- UE 5.8 `WxEditor Win64 Development` 빌드 성공. 로그: `.Codex/skills/build-doctor/logs/build_2026-08-30_170552.log`.
