# Experience 필수 GameFeature 해석 종료 콜백 보호

## 계획

- GameFeature 이름을 URL로 해석하지 못하면 Experience 를 실패 상태로 전환하고, 액션 실행·HUD 발행·로드 완료 이벤트를 차단한다.
- EndPlay 진입 즉시 종료 상태를 기록하고, 이후 도착한 에셋 로드·GameFeature 활성 콜백이 Experience 완료 경로를 타지 않게 한다.
- 기존 GameFeature 비활성화 요청과 정상 로드·종료 경로를 보존한다.
- WxEditor(Development) 타겟을 빌드해 컴파일을 확인한다.

## 완료

- `CollectGameFeaturePluginURLs`가 이름 해석 성공 여부를 반환하게 하고, 하나라도 실패하면 Experience 를 `Failed`로 전환하도록 수정했다.
- URL 해석 실패 시 GameFeature 활성화 요청·Experience 액션 실행·HUD 발행·로드 완료 브로드캐스트를 모두 차단한다.
- EndPlay가 즉시 `Deactivating` 상태로 전환되고, 이후 에셋 로드·GameFeature 활성 콜백은 종료 중인 월드에 작업을 하지 않고 반환한다.
- `WxEditor Win64 Development` 빌드를 성공적으로 완료했다.
