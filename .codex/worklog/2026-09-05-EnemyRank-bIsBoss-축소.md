# EnemyRank bIsBoss 축소

## 계획

- `UWxEnemyComponent`의 보스 여부 단일 소유자를 `EWxEnemyRank` 대신 `bIsBoss`로 변경하고 기존 `IsBoss()` 공개 계약과 보스 HUD 동작은 유지한다.
- 기존 블루프린트에 직렬화된 `EnemyRank=Boss` 값을 잃지 않도록 레거시 프로퍼티와 `PostLoad` 이관 경로를 임시로 유지한다.
- 현 단계에서는 호환 enum과 타입 파일을 삭제하지 않는다. `BP_Boss`를 새 프로퍼티로 리세이브한 뒤 별도 축소 단계에서 제거한다.
- 관련 심볼과 포맷을 검사하고 UE 5.8 `WxEditor` Win64 Development 빌드로 검증한다.

## 완료

- `UWxEnemyComponent`에 `bIsBoss`를 추가하고 `IsBoss()`가 이 값만 반환하도록 변경했다.
- 외부 사용이 없던 `GetEnemyRank()`를 제거했다.
- 기존 BP의 `EnemyRank=Boss`를 `PostLoad()`에서 `bIsBoss=true`로 이관하고 레거시 값을 `Normal`로 비워, 리세이브 후 재이관되지 않도록 했다.
- 후속 전체 통합에서 관련 BP 리세이브를 마친 뒤 `EWxEnemyRank`, `EnemyRank`, `WxEnemyTypes`를 제거했다.
- 관련 심볼 검색 결과 신규 런타임 판정은 `bIsBoss`만 사용하고, `EnemyRank`는 이관 코드에만 남아 있음을 확인했다.
- `git diff --check`에서 공백 오류가 없음을 확인했다(기존 줄바꿈 변환 경고만 출력).
- UE 5.8 `WxEditor` Win64 Development 빌드 성공: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-05_003254_242_6944.log`.
