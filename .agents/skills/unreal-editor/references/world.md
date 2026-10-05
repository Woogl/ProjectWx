# 레벨·월드

## 배치와 저장

- 배치는 `SceneTools.add_to_scene_from_asset`·`add_to_scene_from_class`(`snap_to_ground` 지원)로 한다. 조회는 `SceneTools.find_actors(name:"", tag:"", collision_channels:[], actor_type{refPath})`이고, 앞의 세 인자는 비워도 꼭 넘긴다.
- 월드 파티션 맵의 액터는 맵이 아니라 액터마다 외부 패키지(`Content/__ExternalActors__/...`)에 저장된다. 고치거나 새로 놓은 액터는 **액터 refPath를 `SavePackages`에 넘겨** 저장한다. 맵을 넘기면 맵 패키지만 저장된다.
  - `SceneTools.save_actor`와, 외부 패키지 경로(`/Game/__ExternalActors__/...`)를 받는 `AssetTools.save_assets`·`is_dirty`는 "Asset does not exist"로 실패한다.
  - 액터 삭제는 레벨 저장으로 반영한다. SlateInspector로 레벨 탭을 클릭한 뒤 `PressKey("Ctrl+S")`를 보내면 레벨과 바뀐 외부 액터가 함께 저장된다.
- 배치 인스턴스의 차일드 액터 컴포넌트가 `ChildActorTemplate=None`으로 남아 자식이 설정 없이 스폰될 수 있다(BP 템플릿과 CDO는 정상). 인스턴스 컴포넌트의 `ChildActorClass`를 `"None"`으로 비웠다 클래스로 되돌리면 BP 템플릿을 다시 가리키고 자식이 다시 생긴다. 그 뒤 액터를 저장한다.
- 차일드 액터 템플릿(`…_GEN_VARIABLE.<템플릿명>_CAT`)의 값은 `set_properties`로 바로 쓰고 `compile_blueprint`만으로 배치 인스턴스에 반영된다.

## PCG

- `PCGToolset.ExecuteGraphInstance`는 PCGVolume 액터만 받는다. BP 액터의 PCG 컴포넌트는 `bActivated`를 false로 바꾸면 생성물을 지우고 true로 되돌리면 다시 생성한다. 완료는 `bGenerationInProgress`로 폴링한다.
- 연달아 실행하면 "Failed to call Execute"가 뜬다. 앞 생성이 아직 도는 중이라는 뜻이다.
- PCG는 결과를 캐시해서 시드만 바꾸면 다시 돌지 않을 수 있다. 노드 파라미터를 바꾸거나 `bActivated`를 토글해 강제한다.
- **볼륨을 키워도 결과가 옛 범위에 그대로 남으면 `bExecuteOnGPU`를 의심한다.** 켜진 스포너는 컴퓨트 셰이더가 컴파일되는 동안 생성을 끝내지 못하고 마지막 결과를 유지한다. 같은 그래프의 CPU 갈래까지 함께 멈춘다. 그 노드의 `bExecuteOnGPU`를 껐다 켜면 다시 돈다. 로그의 `Missing cached shader map for kernel ... PCGStaticMeshSpawnerCS, compiling.`이 단서다.
- 인스펙션 데이터는 마지막 실행 시점의 스냅샷이라 뷰포트 캡처와 대조해 확인한다.
- `Get Landscape Data`의 `bMustOverlapSelf`가 켜져 있으면 Z까지 겹침 판정을 해서, 지형보다 높이 그린 스플라인은 경고 없이 투영이 무시된다. 끈다.

## 물

- 물 바디 스플라인은 `WxToolset.WxWaterToolset.SetWaterBodySpline`으로만 쓴다. `ObjectTools`로 `SplineCurves`를 쓰면 에디터가 멈춘다.
- 물 바디가 레벨에 들어오면 에디터가 지형을 파는 WaterBrushManager를 자동으로 만든다. 지형을 이미 수위대로 팠다면 배치하는 동안 `Default__WaterEditorSettings.WaterManagerClassPath`를 비웠다가 되돌린다(메모리만 바뀌고 ini는 그대로다).
- 호수는 스폰 순간 "Failed to triangulate Lake mesh" 경고가 네 번씩 난다. 무해하다. 라벨이 `Water_`인 경고만 문제다.
- 수위는 둘레 지형보다 낮아야 수면이 허공에서 끊기지 않는다.
- 용암처럼 물 바디로 만든 특수 수면:
  - 충돌을 NoCollision으로 두면 강 수면이 안 그려진다. QueryOnly에 모든 채널 Ignore로 둔다. 응답 배열은 맵을 다시 불러와야 하위 충돌 컴포넌트까지 반영된다.
  - 강의 Lake/OceanTransitionMaterial이 남아 있으면 겹치는 곳이 파란 물로 그려진다. 전환 머티리얼을 null로, `OverlapMaterialPriority`를 100으로 둔다.
  - 커스텀 머티리얼은 WaterOpacityMaskFromDepth의 Additional Water Mask에 0을 넣고 OpacityMask를 `saturate(WaterDepth + 24)`로 직접 준다. 1을 넣으면 땅 밑의 물 메시가 가파른 벽을 뚫고 보인다.
  - 물 메시의 발광은 Lumen GI에 빛을 주지 않는다. 둘레를 밝히려면 지형 머티리얼 마스크로 처리한다.
  - 고도가 높은 물은 워터 존의 `bHalfPrecisionTexture`를 꺼야 물가가 고르다.

## 랜드스케이프

- 생성과 높이·가중치 통째 가져오기는 `WxToolset.WxLandscapeToolset`으로 한다.
- Edit Layers 토글은 디테일 패널이 아니라 Landscape 모드의 Manage 패널 하단에만 있다. 디테일 패널 검색에 안 나온다고 꺼진 것이 아니다.
- **지면이 하늘을 비추는 검은 거울처럼 보이면 머티리얼 NaN이다.** 에러 로그가 없고, 캐시를 비운 직후엔 멀쩡하다가 Lumen 카드가 다시 캡처되면 재발해 고쳐졌다고 오판하기 쉽다.
  - 원인 1: Custom 노드 추가 출력으로 낸 노멀. 출력마다 Custom 노드를 나눈다.
  - 원인 2: 랜드스케이프 나나이트를 켰다 끄면 컴포넌트별 머티리얼 인스턴스가 낡은 채 남는다. `LandscapeMaterial`을 다른 머티리얼로 바꿨다 되돌려 재생성한 뒤 저장한다. 같은 그래프 재빌드·recompile로는 안 풀린다.
  - 판정: `r.LumenScene.SurfaceCache.Reset 1`, `r.ResetViewState` 후 1분 넘게 여러 시점을 돌고 PIE로도 본다. 가르는 순서는 `LandscapeMaterial`을 `/Engine/EngineMaterials/WorldGridMaterial`로 대조 → `MP_Normal` 끊기 → 상수 노멀이다.

## 월드 파티션 미니맵

- 굽기는 SlateInspector로 상단 `button "Build"` → `generic "Build World Partition Editor Minimap"`을 클릭한다. 에디터가 맵을 내리고 외부 커맨드렛을 돌린 뒤 다시 올린다.
  - Click 호출은 끝날 때까지 돌아오지 않는다. 백그라운드로 부르고 `Saved/Logs/WorldPartition/WorldPartitionMiniMapBuilder-*.log`의 `[i / N] Processing cells`로 진행을 본다.
  - 도중에 끊으면 실패 모달이 떠 MCP까지 멈춘다. 끝까지 기다린다.
- `AWorldPartitionMiniMapVolume`이 없으면 범위가 런타임 월드 경계로 잡혀, 항상 로드되는 지형이 있으면 수십 km가 되고 지도가 텍스처 한구석에 찍힌다. 지형과 같은 크기의 볼륨을 둔다.
- 해상도는 미니맵 액터의 `WorldUnitsPerPixel`이다. 캡처가 SceneColorHDR라 아주 어두운 알베도는 검은 구멍처럼 보인다.
- 결과는 `EditorAppToolset.CaptureAssetImage`에 `<미니맵 액터 경로>.MinimapTexture`를 주어 보고, `TextureTools.get_size`로 크기를 잰다.
