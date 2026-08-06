# Copyright Woogle. All Rights Reserved.
# 메타휴먼 바디 메시에 UE5 표준 ik 본 7종(ik_foot_root/l/r, ik_hand_root/gun/l/r)을 추가한다.
# FootIK 컨트롤 리그 등 엔진 표준 IK 경로가 ik 본 실존을 전제하므로, 본이 없으면 IK가 죽는다.
# 메타휴먼을 재어셈블하면 바디 메시가 재생성돼 ik 본이 사라지므로, 그때마다 에디터 콘솔에서
#   py "C:/Wx/Tools/AddMetaHumanIkBones.py"
# 로 재실행한다.

import unreal

BODY_MESH_PATH = "/Game/MetaHumans/MH_Player/Body/SKM_MH_Player_BodyMesh"
BODY_SKELETON_PATH = "/Game/MetaHumans/Common/Female/Medium/NormalWeight/Body/metahuman_base_skel"

# 배치는 엔진 관례(IKRigAutoCharacterizer 핀 규칙)를 따른다:
# ik 루트 2종은 root 아래 Identity, ik_foot_l/r은 foot_l/r 레퍼런스와 일치, 손 계열은 hand_r 기준.
IK_BONE_NAMES = ["ik_foot_root", "ik_foot_l", "ik_foot_r", "ik_hand_root", "ik_hand_gun", "ik_hand_l", "ik_hand_r"]
IK_BONE_PARENTS = ["root", "ik_foot_root", "ik_foot_root", "root", "ik_hand_root", "ik_hand_gun", "ik_hand_gun"]


def main():
    mesh = unreal.load_asset(BODY_MESH_PATH)
    if not mesh:
        unreal.log_error("AddMetaHumanIkBones: 바디 메시를 찾을 수 없음: " + BODY_MESH_PATH)
        return

    modifier = unreal.SkeletonModifier()
    if not modifier.set_skeletal_mesh(mesh):
        unreal.log_error("AddMetaHumanIkBones: SetSkeletalMesh 실패")
        return

    existing = set(str(name) for name in modifier.get_all_bone_names())
    present = [name for name in IK_BONE_NAMES if name in existing]
    if len(present) == len(IK_BONE_NAMES):
        unreal.log("AddMetaHumanIkBones: ik 본 7종이 이미 모두 존재하므로 종료")
        return
    if present:
        unreal.log_error("AddMetaHumanIkBones: ik 본이 일부만 존재해 중단(수동 확인 필요): " + ", ".join(present))
        return
    for required in ("root", "foot_l", "foot_r", "hand_l", "hand_r"):
        if required not in existing:
            unreal.log_error("AddMetaHumanIkBones: 기준 본 없음: " + required)
            return

    foot_l = modifier.get_bone_transform("foot_l", True)
    foot_r = modifier.get_bone_transform("foot_r", True)
    hand_l = modifier.get_bone_transform("hand_l", True)
    hand_r = modifier.get_bone_transform("hand_r", True)
    identity = unreal.Transform()
    hand_l_rel_gun = unreal.MathLibrary.make_relative_transform(hand_l, hand_r)

    # 부모가 전부 Identity 글로벌(root 직계 루트들)이라 글로벌 실측값을 로컬로 그대로 쓸 수 있다.
    transforms = [identity, foot_l, foot_r, identity, hand_r, hand_l_rel_gun, identity]

    if not modifier.add_bones(IK_BONE_NAMES, IK_BONE_PARENTS, transforms):
        unreal.log_error("AddMetaHumanIkBones: AddBones 실패")
        return
    if not modifier.commit_skeleton_to_skeletal_mesh():
        unreal.log_error("AddMetaHumanIkBones: CommitSkeletonToSkeletalMesh 실패")
        return

    for asset_path in (BODY_MESH_PATH, BODY_SKELETON_PATH):
        if not unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False):
            unreal.log_warning("AddMetaHumanIkBones: 저장 실패(수동 저장 필요): " + asset_path)

    unreal.log("AddMetaHumanIkBones: ik 본 7종 추가 완료")


main()
