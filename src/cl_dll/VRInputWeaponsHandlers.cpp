
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "VRInput.h"
#include "eiface.h"

namespace
{
	WEAPON* g_pHolsteredWeapon{ nullptr };

#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

}  // namespace

namespace VR
{
	namespace Input
	{
		void Weapons::HandleFire(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
				{
					// don't +attack if in menu!
					if (g_vrInput.IsInGame() && !g_vrInput.IsInMenu())
					{
						ClientCmd("+attack");
					}
				}
				else
				{
					ClientCmd("-attack");
				}
			}
		}

		void Weapons::HandleAltFire(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+attack2");
				else
					ClientCmd("-attack2");
			}
		}

		void Weapons::HandleReload(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bChanged)
			{
				if (data.bState)
					ClientCmd("+reload");
				else
					ClientCmd("-reload");
			}
		}

		void Weapons::HandleHolster(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				if (gHUD.m_Ammo.m_pWeapon)
				{
					// Select hand
					ServerCmd("weapon_barehand");
					PlaySound("common/wpn_select.wav", 1);
					g_pHolsteredWeapon = gHUD.m_Ammo.m_pWeapon;
					gHUD.m_Ammo.m_pWeapon = nullptr;
				}
				else if (g_pHolsteredWeapon)
				{
					// Select holstered weapon
					extern WEAPON* gpActiveSel;
					gpActiveSel = gHUD.m_Ammo.m_pWeapon = g_pHolsteredWeapon;
					g_pHolsteredWeapon = nullptr;
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}
				else
				{
					// No holstered weapon, select any weapon
					gHUD.m_Ammo.UserCmd_NextWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}
			}
		}

		void Weapons::HandleNext(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				if (gHUD.m_Ammo.IsCurrentWeaponLastWeapon())
				{
					// Select hand when current weapon is last weapon
					ServerCmd("weapon_barehand");
					PlaySound("common/wpn_select.wav", 1);
					//PlaySound("common/wpn_hudon.wav", 1);
					gHUD.m_Ammo.m_pWeapon = nullptr;
				}
				else
				{
					gHUD.m_Ammo.UserCmd_NextWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}
			}
		}

		void Weapons::HandlePrevious(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			if (data.bActive && data.bState && data.bChanged)
			{
				if (gHUD.m_Ammo.IsCurrentWeaponFirstWeapon())
				{
					// Select hand when current weapon is first weapon
					ServerCmd("weapon_barehand");
					PlaySound("common/wpn_select.wav", 1);
					//PlaySound("common/wpn_moveselect.wav", 1);
					gHUD.m_Ammo.m_pWeapon = nullptr;
				}
				else
				{
					gHUD.m_Ammo.UserCmd_PrevWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}
			}
		}

		void Weapons::HandleSelect(const vr::InputDigitalActionData_t& data, const std::string& action)
		{
			// TODO: Weapon selection wheel
			// For now, this just calls HandleNext
			HandleNext(data, action);
		}
	}  // namespace Input
}  // namespace VR
