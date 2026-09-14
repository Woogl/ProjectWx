# ApplyDamage 입력 자동화

## 계획

- 사용자가 승인한 내부 판별 방식으로 ApplyDamage의 SourceAbility와 DamageLevel 인자를 제거한다.
- Causer가 WxProjectileBase이면 저장된 발사 레벨과 독립 패시브 판정을 사용하고 나머지는 공격자 ASC의 현재 어빌리티와 레벨(없으면 1)을 사용한다.
- 호출부를 기존 4개 인자로 복원하고 UE 5.8 WxEditor Development 빌드 및 기존 레벨 회귀 테스트로 검증한다.

## 완료

- ApplyDamage 선언·정의 및 호출부 4곳을 기존 4인자로 복원했다. 호출부의 출처 조회 중복도 제거했다.
- 내부에서 Causer를 AWxProjectileBase로 판별한다. 투사체는 GetProjectileLevel()과 nullptr 어빌리티, 나머지는 Source ASC의 AnimatingAbility와 그 레벨(없으면 1)을 사용한다.
- 발사 레벨 저장·반사 시 유지·패시브 발동 키 보존은 유지했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-15_021736_341_29268.log
- 기존 Wx.Combat.Projectile.DamageLevel 테스트 성공: C:/Wx/Saved/Logs/ApplyDamageAutomaticLevelTest.log. 이 테스트는 타격·연결 피해 GE의 레벨 전달을 검사하며 실제 전투나 내부 액터 판별의 통합 테스트는 아니다.
- 호출부 검색 및 git diff --check 통과. 실제 Blueprint 노드 갱신과 전투 재현은 미실시다.
