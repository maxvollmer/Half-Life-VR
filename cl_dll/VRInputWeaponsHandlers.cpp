
#include <iostream>
#include <filesystem>
#include <string>
#include <functional>

#include "hud.h"
#include "cl_util.h"
#include "openvr/openvr.h"
#include "vr_input.h"
#include "eiface.h"

namespace VR
{
	namespace Input
	{
		void Weapons::HandleFire(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandleFire: state: %i, changed: %i\n", data.bState, data.bChanged);
		}

		void Weapons::HandleAltFire(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandleAltFire: state: %i, changed: %i\n", data.bState, data.bChanged);
		}

		void Weapons::HandleAnalogFire(vr::InputAnalogActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandleAnalogFire: active: %i, x: %f, deltaX: %f\n", data.bActive, data.x, data.deltaX);
		}

		void Weapons::HandleReload(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandleReload: state: %i, changed: %i\n", data.bState, data.bChanged);
		}

		void Weapons::HandleHolster(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandleHolster: state: %i, changed: %i\n", data.bState, data.bChanged);
		}

		void Weapons::HandleNext(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandleNext: state: %i, changed: %i\n", data.bState, data.bChanged);
		}

		void Weapons::HandlePrevious(vr::InputDigitalActionData_t data, const std::string& action)
		{
			//gEngfuncs.Con_DPrintf("HandlePrevious: state: %i, changed: %i\n", data.bState, data.bChanged);
		}
	}
}
