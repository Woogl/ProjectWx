// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/** 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성. */
namespace WxGameplayTags
{
	// ── State ──────────────────────────────────────────────────────────────
	
	/** 로컬 플레이어가 이 액터를 락온 중일 때 로컬로만 부여(복제 안 함). 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockedOn);

	/** 적 AI가 플레이어를 인식하면 서버에서 부여, 추적 종료 시 제거. 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InCombat);

	/** 처형(앞잡·뒤잡) 피대상 표시. 연출 동안 WxAbility_Finisher가 대상 ASC에 권위 발행하며, 대상이 이 태그로 자기 처형 어포던스를 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_BeingFinished);

	/** 대화 세션 컴포넌트가 시작·종료에 맞춰 폰 ASC에 loose 태그로 발행. WxAbility_Interact가 ActivationBlockedTags로 사용해 대화 중 프롬프트 표시·상호작용을 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dialogue);

	// ── Effect ──────────────────────────────────────────────────────────────
	
	// GE가 부여하는 태그. 애셋 태그로도 사용한다.

	/** WxEffect_Invincible이 부여하며, 구간을 연 쪽(노티파이 구간·컷신 태스크·처형의 활성 구간)이 수명을 쥔다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Invincible);

	/** 가드 어빌리티가 WxEffect_Guard로 부여하되, SP 고갈로 가드가 깨지면 어빌리티가 도는 중에도 뗀다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Guard);

	/** 가드 몽타주의 노티파이 구간이 WxEffect_PerfectGuard로 부여하고 구간 끝에서 걷어낸다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_PerfectGuard);

	/** SP를 소모하면 WxEffect_Exhaust가 일정 시간 부여한다. SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Exhausted);

	/** 궁극기가 WxEffect_SuperArmor로 활성 구간만큼 부여한다. 대미지는 그대로 들어오고 경직(HitReact)만 막힌다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SuperArmor);

	// ── Movement ──────────────────────────────────────────────────────────────
	
	/** WxCharacterMovementComponent가 낙하 모드 진입·이탈에 맞춰 각 머신에서 부여/제거. 점프 공격의 콤보 세트 진입 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_InAir);
	
	/** WxAbility_Sprint가 활성 중 실제로 이동할 때만 부여. SP 소모 GE의 발동 조건이자 SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Sprint);

	// ── Event ──────────────────────────────────────────────────────────────
	
	/**
	 * 피격 이벤트. 대미지 파이프라인이 서버에서 피격자 ASC에 히트마다 한 번 보낸다.
	 * 자식이 그 히트가 요청한 반응 종류다 — 반응이 있으면 자식을, 반응 없는 평타는 부모를 이벤트 태그로 보낸다.
	 * 패리 반동·처형 짝 피격처럼 대미지 없는 반응 요청도 같은 계층에 EventMagnitude 0으로 싣는다.
	 * 구독은 부모 매칭 API로 한다 — 정확 매칭(GenericGameplayEventCallbacks, WaitGameplayEvent 기본값)은 자식으로 나가는 반응 히트를 놓친다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Normal);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_KnockBack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_KnockDown);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_KnockUp);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Parry);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Finisher);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Backstab);

	/** 무적 구간에서 대미지를 회피했을 때 발송 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DodgeSuccess);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UseItem);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/** 발동 장치가 연결 장치의 트리에 보내는 기본 이벤트. 목적지가 여럿인 장치는 버튼마다 다른 태그를 저작한다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Device_Triggered);

	/** 피니셔(앞잡) 발동 이벤트*/
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher);

	/** 백스탭(뒤잡) 발동 이벤트 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Backstab);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ragdoll);
	
	// ── Device ──────────────────────────────────────────────────────────────

	// 장치의 State Tree 상태값이며 곧 세이브 슬롯에 저장된다.
	// 코드에서 읽거나 쓰는 값은 아니지만 태그는 여기서 정의한다.
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Idle);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Pressed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Locked);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Door_Close);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Door_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_Inactive);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_1F);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_2F);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_TreasureChest_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_TreasureChest_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Unlit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Lit);
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Piston_On);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Piston_Off);

	// ── GameplayCue ──────────────────────────────────────────────────────────────
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_DamageFloater);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_GhostTrail);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);

	/** 공격 경고(선딜 표시) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Red);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Yellow);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Blue);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Purple);
	
	// ── Damage ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanCritical);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Unblockable);

	/** 패리 피격 유발 공격. 이 공격이 퍼펙트 가드로 막히면 공격자에게 Event.Hit.Parry 송출 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_ParryHitReact);
	
	// ── Ability ──────────────────────────────────────────────────────────────
	
	/**
	 * 어빌리티는 자신을 가리키는 식별 태그 Ability.X를 정확히 하나 갖고, AssetTags와 ActivationOwnedTags 양쪽에 넣는다.
	 * 곧 "Ability.X = 그 어빌리티가 지금 활성화 중이다"가 성립한다.
	 * 어빌리티의 성질(배타 그룹 등)은 태그가 아니라 EWxAbilityActivationGroup(WxCombat)으로 선언한다.
	*/

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);

	/** 플레이어 캐릭터용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Air);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_DodgeCounter);
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
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Finisher);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_LockOn);
	
	/** 플레이어 캐릭터, 적 공용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Groggy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Death);

	/** 적 캐릭터용 */
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

	// ── SetByCaller ──────────────────────────────────────────────────────────────

	/** 지속시간 Duration SetByCaller 키. NoCooldown/InfiniteMP/DrainDP 등 Duration 모디파이어에서 공용으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	/** UP 회복량 SetByCaller 키. WxEffect_RecoverResource의 UP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_UP);

	/** MP 회복량 SetByCaller 키. WxEffect_RecoverResource의 MP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_MP);

	/** DP 가산량 SetByCaller 키. WxEffect_AddDP의 DP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_DP);

	/** 공격력 계수 SetByCaller 키. WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	/** 원시 대미지 SetByCaller 키. 양수일 때 ATK/DEF/Coeff/크리를 우회하고 평탄 값으로 대미지 처리 (환경 대미지). 가드/퍼펙트 가드/HitReact는 정상 동작 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_RawDamage);

	/** 이동 속도 배율 SetByCaller 키. WxEffect_MoveSpeedScale이 SPD 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeedScale);
	
	// ── UI ──────────────────────────────────────────────────────────────

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
