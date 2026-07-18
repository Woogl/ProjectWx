// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * 프로젝트 전체에서 사용하는 Gameplay Tag 선언부.
 * 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성.
 *
 * 네이밍: 점(.) 구분자를 언더스코어(_)로 치환하여 변수명 사용.
 * 예) "State.Dead" → State_Dead
 */
namespace WxGameplayTags
{
	// ── State ──────────────────────────────────────────────────────────────

	/** 사망 상태. HandleDeath 시 ASC에 부여되며, 부활 시 제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	/** 래그돌 상태. 사망 어빌리티가 서버에서 부여하며, 전 머신의 캐릭터가 감지해 물리 전환. 해제 없이 시체에 유지 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Ragdoll);

	/** 그로기 상태. DP가 MaxDP에 도달 시 부여 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);

	/** 락온 상태. 락온 어빌리티 활성 중 부여, Look 입력 억제 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockOn);

	/** 락온 피대상 표시. 로컬 플레이어가 이 액터를 락온 중일 때 로컬로만 부여(복제 안 함). 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockedOn);

	/** 전투 상태. 적 AI가 플레이어를 인식하면 서버에서 부여, 추적 종료 시 제거. 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InCombat);

	/** 무적 상태. WxAnimNotifyState_Invincible이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	
	/** 가드 판정 활성 상태 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guard);

	/** 퍼펙트 가드 판정 구간. ANS_PerfectGuard가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectGuard);

	/** 피격 반응 상태. WxAbility_HitReact가 ActivationOwnedTags로 활성 중에만 자동 부여 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_HitReact);

	/** 슈퍼 아머 상태. 소유 중인 어빌리티는 HitReact로 캔슬되지 않음 (HitReact의 ActivationBlockedTags) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SuperArmor);

	/** 처형(앞잡·뒤잡) 연출 진행 상태. WxAbility_Finisher가 ActivationOwnedTags로 발행. 연출 중 상호작용 재입력을 막기 위해 WxAbility_Interact가 ActivationBlockedTags로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Finisher);

	// ── Event ─────────────────────────────────────────────────────────────

	/** 피격 이벤트 부모 카테고리. ExecCalc 필터링용. Guard 어빌리티가 자식 태그를 모두 수신하기 위해 부모로도 구독 (직접 dispatch 금지) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	/** 일반 피격 이벤트. HitReact 어빌리티가 기본 HitReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Normal);

	/** 넉백 피격 이벤트. HitReact 어빌리티가 Knockback 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockBack);

	/** 넉다운 피격 이벤트. HitReact 어빌리티가 Knockdown 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockDown);

	/** 넉업 피격 이벤트. HitReact 어빌리티가 Knockup 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockUp);

	/** 패리 피격 이벤트. 공격이 퍼펙트 가드로 막힌 공격자에게 송출, HitReact 어빌리티가 ParryReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Parry);

	/** 피니셔(앞잡) 짝 피격 이벤트. 피니셔 대상 적에게 송출, HitReact 어빌리티가 FinisherMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Finisher);

	/** 백스탭(뒤잡) 짝 피격 이벤트. 백스탭 대상 적에게 송출, HitReact 어빌리티가 BackstabMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Backstab);

	/** 회피 성공 시 발생하는 이벤트. 무적 구간에서 대미지를 회피했을 때 발송 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DodgeSuccess);

	/** 퍼펙트 가드 성공 시 발생하는 이벤트. Guard 어빌리티가 MP 회복 및 HitReact를 처리 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	/** 소비 아이템 사용 이벤트. 마시기 몽타주의 노티파이가 송출, UseItem 어빌리티가 수신해 아이템을 실제 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UseItem);

	/** 상호작용 발동 이벤트. 상호작용 입력 시 소유 클라가 선택 대상을 실어 서버에 요청하고, 서버가 플레이어 ASC로 송출(OptionalObject=선택 컴포넌트), WxAbility_Interact가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/** 피니셔(앞잡) 발동 이벤트. 그로기 적 상호작용 시 플레이어 ASC로 송출(Target=적), WxAbility_Finisher가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher);

	/** 백스탭(뒤잡) 발동 이벤트. 미인지 적 후방 상호작용 시 플레이어 ASC로 송출(Target=적), WxAbility_Finisher가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Backstab);

	/** 역경직(히트 스톱) 이벤트. WxExecCalc_Damage가 무적 회피가 아닌 적중에 공격자 ASC로 송출(EventMagnitude=정지 시간), 재생 중인 공격 어빌리티가 자기 몽타주를 잠깐 정지 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitStop);

	// ── Gimmick ───────────────────────────────────────────────────────────
	// 각 태그는 해당 기믹의 권위 상태 값(복제·SaveGame)이자, GimmickStateTree 로 보내는
	// 진입 이벤트(상태의 Required Event to Enter 와 매칭)를 겸한다.

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Door_Close);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Door_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_AtStart);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_AtEnd);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_SpawnConsole_Idle);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_SpawnConsole_Spawned);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_AlarmConsole_Idle);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_AlarmConsole_Alarmed);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CutsceneTrigger_Idle);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CutsceneTrigger_Playing);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_TreasureChest_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_TreasureChest_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CheckPoint_Unlit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CheckPoint_Lit);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_LaserCorridor_Active);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_LaserCorridor_Deactivated);

	/** 복원 진입 마커. 세이브 복원 시 호스트가 상태 태그 이벤트와 함께 발행해, 일회성 효과(보상·스폰·FX)를 발동하지 않고 스냅으로 처리하게 한다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Restore);

	// ── ANS ───────────────────────────────────────────────────────────────

	/** 무기 콜리전 활성 구간. ANS_WeaponCollision이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_WeaponCollision);

	/** 콤보 입력 수용 구간. ANS_ComboWindow가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_ComboWindow);

	// ── GameplayCue ──────────────────────────────────────────────────────

	/** 데미지 Cue. 임팩트 위치에 스파크/사운드 재생, 데미지 플로터 출력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage);

	/** 퍼펙트 가드 성공 Cue. 임팩트 위치에 스파크/사운드 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);

	/** ATK 버프 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_BuffATK);

	/** Exceed 버프 지속 Cue. 활성 동안 캐릭터의 무기에 Niagara 이펙트를 부착 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);

	/** 화상 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Burn);

	/** 공격 텔레그래프(선딜 표시) Cue. 색상별로 나뉘며, AttackTelegraph 노티파이가 권위에서 발행. RawMagnitude에 차징 길이를 실어 전달 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Red);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Yellow);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Blue);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Purple);

	// ── Damage ────────────────────────────────────────────────────────────

	/** 치명타 판정 결과 태그 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	/** 가드 불가 공격 태그. 가드·퍼펙트 가드를 무시하고 풀 대미지 적용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Unblockable);

	/** 패리 피격 유발 공격 태그. 이 공격이 퍼펙트 가드로 막히면 공격자에게 Event.HitReact.Parry 송출 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_ParryHitReact);

	// ── Ability ───────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_UseItem);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_5);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_6);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_7);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_8);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_9);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_10);

	// ── SetByCaller ──────────────────────────────────────────────────────

	/** 지속시간 Duration SetByCaller 키. Cooldown/NoCooldown/InfiniteMP/DrainDP, Groggy 등 Duration 모디파이어에서 공용으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	/** UP 회복량 SetByCaller 키. WxEffect_RecoverResource의 UP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_UP);

	/** MP 회복량 SetByCaller 키. WxEffect_RecoverResource의 MP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_MP);

	/** DP 반사량 SetByCaller 키. 퍼펙트 가드에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ReflectDP);

	/** 공격력 계수 SetByCaller 키. WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	/** 원시 대미지 SetByCaller 키. 양수일 때 ATK/DEF/Coeff/크리를 우회하고 평탄 값으로 대미지 처리 (환경 대미지). 가드/퍼펙트 가드/HitReact는 정상 동작 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_RawDamage);

	/** 역경직(히트 스톱) 지속시간 SetByCaller 키. WxExecCalc_Damage가 적중 시 이 값을 실어 공격자에게 Event.HitStop을 발동한다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_HitStop);

	// ── Input ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_LockOn);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_UseItem);

	// ── UI ────────────────────────────────────────────────────────────────

	/** HUD 레이어 (플레이어 체력 바 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (아이템 획득 알림 등, 메뉴 아래) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	/** 메뉴 레이어 (인벤토리, 설정 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	/** 모달 레이어 (확인 창 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/** CommonUI 액션: 인벤토리 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Inventory);

	/** CommonUI 액션: 메인 메뉴 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_MainMenu);

	/** CommonUI 액션: 자유 커서 홀드. HUD가 Pressed/Released로 나눠 수신해 입력 모드를 전환한다. 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_FreeCursor);
}
