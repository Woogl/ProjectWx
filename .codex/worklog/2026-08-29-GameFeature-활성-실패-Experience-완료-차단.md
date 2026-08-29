# GameFeature 활성 실패 Experience 완료 차단

## 계획

- GameFeature 활성화 콜백의 실패를 누적하고, 모든 요청의 콜백이 돌아온 뒤 실패 상태로 전환한다.
- 실패한 Experience는 액션 실행, HUD 발행, 로드 완료 이벤트 브로드캐스트를 하지 않는다.
- 성공한 GameFeature 활성 요청은 실패 경로에서 해제해 부분 활성 상태가 남지 않게 한다.
- 사용자가 런타임 확인을 맡으므로 빌드는 실행하지 않고 정적 검토만 수행한다.

## 완료

- GameFeature 활성 콜백의 실패를 누적하고, 마지막 콜백에서 실패 상태로 전환하도록 수정했다.
- 실패 시 Experience 액션 실행·HUD 발행·로드 완료 이벤트 브로드캐스트를 모두 건너뛴다.
- 같은 Experience 가 성공적으로 활성화한 GameFeature 요청만 해제해 부분 활성 상태가 남지 않게 했다.
- `git diff --check` 및 UE 5.8 GameFeature 완료 델리게이트 시그니처를 정적 검토했다.
- 사용자 요청에 따라 WxEditor(Development) 빌드와 런타임 PIE 확인은 실행하지 않았다.
