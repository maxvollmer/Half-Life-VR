
// Only to be included by VRPhysicsHelper.cpp

// Helpers for getting BSP models from entities
namespace
{
	// Returns a pointer to a model_t instance holding BSP data for the current map - Max Vollmer, 2018-01-21
	const model_t* GetWorldBSPModel()
	{
		playermove_s* pmove = PM_GetPlayerMove();
		if (pmove != nullptr)
		{
			for (const physent_t& physent : pmove->physents)
			{
				if (physent.info == 0)
				{
					return physent.model;
				}
			}
		}
		return nullptr;
	}

	bool IsValidSpriteName(const char* name)
	{
		if (!name)
			return false;
		std::string_view sname{ name };
		return sname.size() > 4 && sname.find(".spr") == sname.size() - 4;
	}

	const model_t* GetEngineModelArray()
	{
		if (g_EngineModelArrayPointer == nullptr)
		{
			// Get the world model and check that it's valid (loaded)
			const model_t* worldmodel = GetWorldBSPModel();
			if (worldmodel != nullptr && worldmodel->needload == 0)
			{
				g_EngineModelArrayPointer = worldmodel;

				// Walk backwards to find start of array (there might be cached sprites in the array before the first map got loaded)
				while ((g_EngineModelArrayPointer - 1)->type == mod_sprite && IsValidSpriteName((g_EngineModelArrayPointer - 1)->name))
				{
					g_EngineModelArrayPointer--;
				}
			}
		}
		return g_EngineModelArrayPointer;
	}

	const model_t* FindEngineModelByName(const char* name)
	{
		const model_t* modelarray = GetEngineModelArray();
		if (modelarray != nullptr)
		{
			for (size_t i = 0; i < ENGINE_MODEL_ARRAY_SIZE; ++i)
			{
				if (modelarray[i].needload == IS_LOADED && strcmp(modelarray[i].name, name) == 0)
				{
					return modelarray + i;
				}
			}
		}
		return nullptr;
	}

	// Returns a pointer to a model_t instance holding BSP data for this entity's BSP model (if it is a BSP model) - Max Vollmer, 2018-01-21
	const model_t* GetBSPModel(edict_t* pent)
	{
		// Check if entity is world
		if (pent == INDEXENT(0))
			return GetWorldBSPModel();

		const char* modelname = STRING(pent->v.model);

		// Check that the entity has a brush model
		if (modelname != nullptr && modelname[0] == '*')
		{
			return FindEngineModelByName(modelname);
		}

		return nullptr;
	}

	// Returns a pointer to a model_t instance holding BSP data for this entity's BSP model (if it is a BSP model) - Max Vollmer, 2018-01-21
	const model_t* GetBSPModel(CBaseEntity* pEntity)
	{
		return GetBSPModel(pEntity->edict());
	}

	bool IsSolidInPhysicsWorld(edict_t* pent)
	{
		return FClassnameIs(&pent->v, "func_wall") || FClassnameIs(&pent->v, "func_illusionary");
	}

	bool DoesAnyBrushModelNeedLoading(const model_t* const models)
	{
		for (int index = 0; index < gpGlobals->maxEntities; index++)
		{
			edict_t* pent = INDEXENT(index);
			if (FNullEnt(pent) && !FWorldEnt(pent))
				continue;

			if (!IsSolidInPhysicsWorld(pent))
				continue;

			const model_t* const model = GetBSPModel(pent);
			if (model == nullptr || model->needload != 0)
			{
				return true;
			}
		}
		return false;
	}

	bool IsWorldValid(const model_t* world)
	{
		return world != nullptr && world->needload == 0 && world->marksurfaces[0] != nullptr && world->marksurfaces[0]->polys != nullptr && !DoesAnyBrushModelNeedLoading(world);
	}

	bool CompareWorlds(const model_t* world1, const model_t* worl2)
	{
		return world1 == worl2 && world1 != nullptr && FStrEq(world1->name, worl2->name);
	}
}

