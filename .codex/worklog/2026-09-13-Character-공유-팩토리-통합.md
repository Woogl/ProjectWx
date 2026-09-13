# Character 공유 팩토리 통합

## 계획

- 사용자가 지정한 `GetOrCreate(ASC, DisplaySource)` 방식으로 공유 Character 뷰모델의 조회·생성·최초 초기화를 팩토리에 통합한다.
- ASC가 없으면 nullptr, 기존 공유본이 있으면 초기화 없이 반환하고 새로 생성했을 때만 표시 소스로 초기화한다.
- 플레이어 리졸버와 네임플레이트의 중복 획득 코드를 팩토리 호출로 교체한다. 보스 디스플레이의 자체 소유·재초기화 동작은 유지한다.
- 호출 경로와 diff를 검토하고 런처 정보에서 UE 5.8 경로를 조회해 WxEditor Win64 Development 빌드를 검증한다.

## 완료

- `UWxViewModel_Character::GetOrCreate(UAbilitySystemComponent* InASC, const UObject* InDisplaySource)`에 ASC 기준 공유 조회·생성·최초 초기화를 통합했다.
- 플레이어 리졸버와 네임플레이트를 팩토리 호출로 교체했다. 기존 공유본은 초기화 없이 반환하므로 표시 데이터 및 진행 중인 이미지 요청을 유지한다.
- 보스 디스플레이의 자체 소유 뷰모델과 재초기화 경로는 유지했다.
- 호출 경로 및 diff 검토 완료, `git diff --check` 통과.
- 최초 제한 환경 빌드는 컴파일 출력 없이 종료되었고 UBT 사용자 로그 접근 거부를 확인했다. build-doctor를 샌드박스 외 권한으로 실행하여 UE 5.8의 WxEditor Win64 Development 빌드 성공(종료 코드 0)을 확인했다.
- 빌드 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-13_191210_467_4904.log`.
- 에디터 실행 및 런타임 UI 검증은 수행하지 않았다.
