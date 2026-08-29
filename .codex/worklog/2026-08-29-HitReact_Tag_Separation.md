# HitReact 태그 분리

## 계획

- 대미지 테이블에서 저작하는 네 피격 반응 태그를 `Event.Hit.*`에서 `HitReact.*`로 분리한다.
- 피격 GameplayEvent는 일반 피격에 `Event.Hit`을 유지하고, 반응 태그는 이벤트 페이로드로 전달한다.
- `Event.Hit.Parry`와 `Event.Hit.GuardBreak`는 전투 시스템이 발생시키는 이벤트이므로 유지한다.
- 기존 직렬화 에셋의 네 태그 값은 GameplayTag 리다이렉트로 새 태그에 이전한다.
- `WxEditor` Development 빌드로 컴파일을 확인한다.

## 완료

