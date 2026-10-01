// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/** 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성. */
namespace WxGameplayTags
{
	/** 캐릭터 BP의 IdentityTags 로 지정한다. 전투 서브시스템이 교전 중인 보스를 가릴 때 읽는다. */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Boss);

	/**
	 * 대화 세션 컴포넌트가 시작·종료에 맞춰 폰 ASC에 loose 태그로 발행한다.
	 * WxAbility_Interact가 ActivationBlockedTags로 사용해 대화 중 프롬프트 표시·상호작용을 닫는다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dialogue);

	/**
	 * UWxAbility_Death가 서버에서 loose 태그로 발행한다(TagOnly 복제).
	 * AWxCharacterBase가 구독해 전 머신에서 래그돌로 전환한다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Ragdoll);

	/**
	 * 소환물을 보유한 주인에게 붙는다. 소환물 컴포넌트가 종류마다 하나를 선언하고, 살아 있는 소환물이 서버에서 주인 ASC 에 하나씩 쌓는다(복제).
	 * 같은 입력을 쓰는 소환·명령 스킬의 발동 조건이며, 부모 Master 로 물으면 종류를 가리지 않는다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Master_Minion);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Master_Doppelganger);

	// GE가 부여하는 태그. 애셋 태그로도 사용한다.

	/** WxEffect_Invincible이 부여하며, 구간을 연 쪽(노티파이 구간·스킬 컷신 컴포넌트의 세션·처형의 활성 구간)이 수명을 쥔다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Invincible);

	/**
	 * 가드 어빌리티가 WxEffect_GuardReduction으로 부여하고 종료에서 걷는다.
	 * SP 고갈로 가드가 깨질 때는 리액션 어빌리티가 가드를 끊어 같은 경로로 걷힌다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_GuardReduction);

	/** 가드 몽타주의 노티파이 구간이 WxEffect_PerfectGuard로 부여하고 구간 끝에서 걷어낸다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_PerfectGuard);

	/** SP를 소모하면 WxEffect_Exhaust가 일정 시간 부여한다. SP 자연 회복의 억제 조건 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Exhausted);

	/** 궁극기가 WxEffect_SuperArmor로 활성 구간만큼 부여한다. 대미지는 그대로 들어오고 경직(HitReact)만 막힌다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SuperArmor);

	/**
	 * WxEffect_HitStop이 적중마다 무기·투사체의 InstigatorHitStop·VictimHitStop만큼 부여한다.
	 * 있는 동안 WxHitStopComponent가 액터의 CustomTimeDilation을 낮춘다. GE 수명은 월드 시간을 따른다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_HitStop);

	/** 스킬 컷신 세션 동안 WxEffect_SkillCutscene이 모든 플레이어에게 부여한다. 입력형 어빌리티 발동을 막는다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SkillCutscene);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_IgnoreAggro);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_IgnoreCosts);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_IgnoreAbilityActivationTags);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_InAir);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Sprint);

	/** 대미지 테이블이 저작하는 피격 반응 종류. Event.Hit의 TargetTags 페이로드로 전달한다. */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_Normal);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_KnockBack);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_KnockDown);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_KnockUp);

	/**
	 * UWxAbilityBase가 ActionPhase가 바뀔 때마다 자기 ASC에 로컬로 보낸다(복제 없음).
	 * 태그·쿨다운·코스트 변화는 싣지 않는다.
	 * 어빌리티 발동 트리거로 사용하지 않는다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_ActionPhaseChanged);

	/**
	 * 피격 이벤트. 대미지 파이프라인이 서버에서 피격자 ASC에 히트마다 한 번 보낸다.
	 * 공격이 요청한 반응 종류는 TargetTags의 HitReact.* 페이로드로 전달한다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit);
	
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Parry);

	/**
	 * 이 히트로 SP가 바닥나 가드가 깨졌다. 대미지 파이프라인이 서버에서 판정해 Event.Hit 대신 이것을 보낸다.
	 * 클라의 복제 SP는 트리거보다 늦게 도착하므로 판정을 클라에서 다시 하면 서버와 갈린다.
	 * 가드하지 않은 대상에게는 받아 줄 어빌리티가 없어 가드 중인 대상에게만 보낸다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_GuardBreak);

	/**
	 * 적중 이벤트의 공격자 몫. 대미지 파이프라인이 서버에서 공격자 ASC에 히트마다 한 번 보낸다.
	 * EventMagnitude는 최종 대미지이고, ContextHandle에 그 히트를 낸 어빌리티가 실려 있다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DamageDealt);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/** 적 상호작용이 서버에서 플레이어 ASC에 보내는 처형 트리거. 앞잡·뒤잡 모두 같은 연출이다. */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);

	/** 소비 아이템의 스택 차감과 Usable 프래그먼트의 사용 효과 적용을 실행한다. */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UseItem);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ApplyFinisherDamage);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PlayFinisherVictimMontage);


	// 장치의 State Tree 상태값이다. C++ 에서는 읽거나 쓰지 않는다.

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Idle);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Pressed);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Door_Close);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Door_Open);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_Inactive);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_Idle);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_Moving);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_TreasureChest_Closed);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_TreasureChest_Open);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Unlit);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Lit);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Resting);
	
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Piston_On);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Piston_Off);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_DamageFloater);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_GhostTrail);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Doppelganger);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Red);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Yellow);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Blue);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Purple);
	
	/** 대미지 ExecCalc가 판정해 스펙에 붙이는 결과 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	/** 대미지 ExecCalc가 판정해 스펙에 붙이는 결과 — 가드 히트의 SP 차감이 이 히트로 0에 닿았다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_GuardBreak);

	/** 대미지 ExecCalc가 판정해 스펙에 붙이는 결과 — 퍼펙트 가드로 받았다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_PerfectGuarded);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanCritical);

	/** 가드로 막을 수 있는 공격. 이 태그가 없으면 일반 가드도 퍼펙트 가드도 뚫는다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanGuard);

	/** 패리가 성립하는 공격. 이 공격이 퍼펙트 가드로 막히면 공격자가 Event.Hit.Parry로 역경직에 걸린다 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanParry);
	
	/**
	 * 어빌리티는 자신을 가리키는 식별 태그 Ability.X를 정확히 하나 갖고, AssetTags와 ActivationOwnedTags 양쪽에 넣는다.
	 * 곧 "Ability.X = 그 어빌리티가 지금 활성화 중이다"가 성립한다.
	*/

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);

	/**
	 * 캐릭터가 스스로 하는 동작. 서로 막고(BlockAbilitiesWithTag에 Ability.Action), 본동작 → 콤보 창 → 후딜 단계를 밟는다.
	 * 반응·처형·질주·락온·패시브는 속하지 않는다.
	 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack_Light);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack_Heavy);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack_Air);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack_DodgeCounter);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill_1);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill_2);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill_3);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill_4);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Ultimate);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Dodge);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Guard);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_UseItem);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Interact);

	/** 적 캐릭터 전용 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_1);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_2);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_3);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_4);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_5);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_6);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_7);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_8);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Pattern_9);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_LockOn);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive);

	/** 플레이어 캐릭터, 적 공용 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_GuardReact);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Groggy);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Death);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Finisher);

	/** 밖에서 주입된 일회성 몽타주 연출 중. 적의 처형 어포던스가 이걸로 닫힌다 — 처형 당하기는 기상까지가 그 구간이다. */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_PlayMontageOnce);

	/**
	 * 쿨다운 식별 태그. 순정 CheckCooldown·쿨다운 조회 API가 이 태그로 쿨다운을 식별한다.
	 * 어빌리티가 CooldownTags로 골라 공용 쿨다운 GE의 스펙에 붙인다. 같은 태그를 고른 어빌리티끼리 쿨다운을 나눠 쓴다.
	 * 적은 쿨다운을 쓰지 않는다 — 패턴 간격은 BT가 잡는다.
	 */

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dodge);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_1);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_2);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_3);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ultimate);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Magnitude);
	
	/** WxEffect_HitStop·WxEffect_Cooldown의 DurationMagnitude에서 사용 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	/** WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	/** WxEffect_MoveSpeedScale이 MOV 어트리뷰트에 곱하는 배율 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeedScale);
	
	/** 아이템 정의의 분류. 인벤토리 VM 이 탭 필터 키로도 쓴다. */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Equipment);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Consumable);
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Currency);

	/** HUD 레이어 (플레이어 체력 바 등) */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (메뉴 아래) */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/** HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Inventory);

	/** HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_MainMenu);

	/** HUD가 Pressed/Released로 나눠 수신해 입력 모드를 전환한다. 키 매핑은 CommonUI Input Settings에서 지정 */
	WXGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_FreeCursor);
}
