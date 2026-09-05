# 흰색 중공 대검

레퍼런스 이미지를 참고하여 제작한 비대칭 흰색 대검입니다.

- 전체 길이: 150cm / 최대 폭: 23.31cm / 최대 두께: 4.6cm (손잡이 칼라 포함)
- 칼날 두께: 약 2.4cm / 손잡이 구간: 약 30cm
- 삼각형: 6,250 / UV 채널: 1 / 재질: 4개
- 긴 빈 공간은 실제로 관통된 메시입니다.
- 원점: 손잡이 중앙. 칼날 방향: +Z.
- FBX 7.4 Binary, 단위 메타데이터 포함. Blender 원본은 미터 단위입니다.

## 파일

- SM_WxWhiteHollowGreatsword_150cm.fbx: 정적 메시 및 기본 재질
- WxWhiteHollowGreatsword.blend: 편집 가능한 모델, 재질, 조명, 카메라
- Preview.png / Front.png: 사선 / 정면 렌더
- create_model.py: Blender 모델 생성 스크립트
- Validation.json: 메시 및 FBX 재가져오기 검증 결과

## 언리얼 가져오기

콘텐츠 브라우저로 FBX를 가져와 Static Mesh로 사용합니다. Import Uniform Scale은 1로 두고, 단위 변환을 적용한 상태에서 길이가 150cm인지 확인하세요. 재질 슬롯은 IvoryBlade, SilverEdge, GripUnderlay, WhiteWrap으로 구분됩니다.

흰색과 은색은 재질 기본색으로 제공되며 별도 텍스처는 없습니다. FBX로 전달되는 재질은 Blender 렌더와 다르게 보일 수 있으므로 언리얼에서 Metallic/Roughness를 다음 값으로 맞출 수 있습니다.

| 재질 | Metallic | Roughness |
| --- | --- | --- |
| IvoryBlade | 0.32 | 0.30 |
| SilverEdge | 0.82 | 0.23 |
| GripUnderlay | 0.15 | 0.55 |
| WhiteWrap | 0.00 | 0.64 |

자동 생성 단순 충돌은 구멍을 막을 수 있습니다. 관통 공간까지 정확한 충돌이 필요하면 용도에 맞는 별도 충돌 구성이 필요합니다. 스켈레톤, 애니메이션, 커스텀 충돌 및 LOD는 포함하지 않습니다.

## 에셋 검증

- 닫히지 않은 메시 에지: 0.
- 구멍을 가로지르는 ray 검사: 통과.
- FBX 재가져오기 후 길이: 1.5m.
- 정면 및 사선 미리보기 육안 확인 완료.
- 언리얼 에디터 내부 실제 임포트/렌더는 검증하지 않았습니다.

## 빌드 결과

- 상태: 성공
- 로그: C:/wx/Saved/Logs/BuildDoctor/build_2026-09-06_033137_255_44308.log

## 원인 요약

- UE 5.8 WxEditor Win64 Development 타겟이 최신 상태이며 빌드 성공했습니다. 추가 조치는 없습니다.

## 근거 로그

> Target is up to date
> Result: Succeeded
> BUILD_DOCTOR_EXIT_CODE=0

## 수정 방법

1. 추가 조치 불필요.

## 재실행 명령

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```
