/*-------------------------------------------------------------------------------

	BARONY
	File: monster_succubus.cpp
	Desc: implements all of the succubus monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"

void initSuccubus(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = true;
	my->initMonster(190);
	my->z = -1;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 70;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants
		    const bool boss =
		        rng.rand() % 50 == 0 &&
		        !my->flags[USERFLAG2] &&
		        !myStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS];

			if ( !strcmp(myStats->name, "Marishka") || !strcmp(myStats->name, "Aleera")
				|| !strcmp(myStats->name, "Verona") )
			{
				myStats->setAttribute("special_npc", "bram succubi");
			}
		    else if ( (boss || (*cvar_summonBosses && conductGameChallenges[CONDUCT_CHEATS_ENABLED])) && myStats->leader_uid == 0 )
			{
				myStats->setAttribute("special_npc", "lilith");
				strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
				my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
				myStats->DEX = 10;
				for ( c = 0; c < 2; c++ )
				{
					Entity* entity = summonMonster(SUCCUBUS, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
						if ( Stat* followerStats = entity->getStats() )
						{
							followerStats->leader_uid = entity->parent;
						}
						entity->seedEntityRNG(rng.getU32());
					}
				}
				newItem(MASK_MOUTH_ROSE, EXCELLENT, -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
				customItemsToGenerate -= 1;
			}

			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			newItem(SPELLBOOK_CHARM_MONSTER, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					if ( myStats->getAttribute("special_npc") == "lilith" && rng.rand() % 4 > 0 )
					{
						newItem(MAGICSTAFF_CHARM, EXCELLENT, -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory); // 75% chance
					}
					else if ( rng.rand() % 10 == 0 )
					{
						newItem(MAGICSTAFF_CHARM, static_cast<Status>(DECREPIT + rng.rand() % 2), -1 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory); // 10% chance
					}
					else if ( rng.rand() % 10 == 0 )
					{
						newItem(MASK_MASQUERADE, WORN, -2 + rng.rand() % 3, 1, rng.rand(), false, &myStats->inventory);
					}
					break;
				default:
					break;
			}
		}
	}

	if ( my->sprite == MonsterData_t::monsterDataEntries[SUCCUBUS].specialNPCs["lilith"].baseModel )
	{
		my->focalz = -3;
		my->focalx = 1;
	}


	// torso
	Entity* entity = newEntity(my->sprite == 1126 ? 1129 : 191, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SUCCUBUS][1][0]; // 0
	entity->focaly = limbs[SUCCUBUS][1][1]; // 0
	entity->focalz = limbs[SUCCUBUS][1][2]; // 0
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(my->sprite == 1126 ? 1128 : 195, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SUCCUBUS][2][0]; // 1
	entity->focaly = limbs[SUCCUBUS][2][1]; // 0
	entity->focalz = limbs[SUCCUBUS][2][2]; // 2
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(my->sprite == 1126 ? 1127 : 194, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SUCCUBUS][3][0]; // 1
	entity->focaly = limbs[SUCCUBUS][3][1]; // 0
	entity->focalz = limbs[SUCCUBUS][3][2]; // 2
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(my->sprite == 1126 ? 1124 : 193, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SUCCUBUS][4][0]; // -.25
	entity->focaly = limbs[SUCCUBUS][4][1]; // 0
	entity->focalz = limbs[SUCCUBUS][4][2]; // 1.5
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(my->sprite == 1126 ? 1122 : 192, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SUCCUBUS][5][0]; // -.25
	entity->focaly = limbs[SUCCUBUS][5][1]; // 0
	entity->focalz = limbs[SUCCUBUS][5][2]; // 1.5
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SUCCUBUS][6][0]; // 
	entity->focaly = limbs[SUCCUBUS][6][1]; // 
	entity->focalz = limbs[SUCCUBUS][6][2]; // 
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SUCCUBUS][7][0]; // 
	entity->focaly = limbs[SUCCUBUS][7][1]; // 
	entity->focalz = limbs[SUCCUBUS][7][2]; // 
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// cloak
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SUCCUBUS][8][0]; // 0
	entity->focaly = limbs[SUCCUBUS][8][1]; // 0
	entity->focalz = limbs[SUCCUBUS][8][2]; // 4
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// helmet
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SUCCUBUS][9][0]; // 0
	entity->focaly = limbs[SUCCUBUS][9][1]; // 0
	entity->focalz = limbs[SUCCUBUS][9][2]; // -2
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// mask
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SUCCUBUS][10][0]; // 0
	entity->focaly = limbs[SUCCUBUS][10][1]; // 0
	entity->focalz = limbs[SUCCUBUS][10][2]; // .5
	entity->behavior = &actSuccubusLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actSuccubusLimb(Entity* my)
{
	my->actMonsterLimb();
}

void succubusDie(Entity* my)
{
	for ( int c = 0; c < 10; c++ )
	{
		Entity* gib = spawnGib(my);
	    if (c < 6) {
	        if (my->sprite == 1126) {
	            gib->sprite = 1124 + c;
	        } else {
	            gib->sprite = 190 + c;
	        }
	        gib->skill[5] = 1; // poof
	    }
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 71, 255);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define SUCCUBUSWALKSPEED .25

void succubusMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, *entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;
	bool wearingring = false;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != nullptr )
		{
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		}
		if ( myStats->cloak != nullptr )
		{
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < LIMB_HUMANOID_TORSO )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			my->flags[BLOCKSIGHT] = true;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < LIMB_HUMANOID_TORSO )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityBodypart(my, bodypart);
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				bodypart++;
			}
		}

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 1.5;
		}
		else
		{
			my->z = -1;
		}
	}

	Entity* shieldarm = nullptr;
	Entity* helmet = nullptr;
	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 && bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			// don't let the creatures's yaw move the casting arm
		}
		else
		{
			entity->yaw = my->yaw;
		}
		if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		{
			my->humanoidAnimateWalk(entity, node, bodypart, SUCCUBUSWALKSPEED, dist, 0.1);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( my->monsterAttack > 0 )
				{
					my->handleWeaponArmAttack(weaponarm);
					if ( my->monsterAttack != MONSTER_POSE_MELEE_WINDUP2 && my->monsterAttack != 2 )
					{
						// flare out the weapon arm to match neutral arm position. 
						// breaks the horizontal chop attack animation so we skip it.
						weaponarm->roll = -PI / 32;
					}
				}
			}
			else if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->pitch = entity->fskill[0];
			}

			my->humanoidAnimateWalk(entity, node, bodypart, SUCCUBUSWALKSPEED, dist, 0.1);

			if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->fskill[0] = entity->pitch;
				entity->roll = my->roll - fabs(entity->pitch) / 2;
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// torso
			case LIMB_HUMANOID_TORSO:
				my->setHumanoidLimbOffset(entity, SUCCUBUS, LIMB_HUMANOID_TORSO);
				break;
				// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1126 ? 1128 : 195;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_RIGHT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				my->setHumanoidLimbOffset(entity, SUCCUBUS, LIMB_HUMANOID_RIGHTLEG);
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = my->sprite == 1126 ? 1127 : 194;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_LEFT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				my->setHumanoidLimbOffset(entity, SUCCUBUS, LIMB_HUMANOID_LEFTLEG);
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				node_t* weaponNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterAttack == 0) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SUCCUBUS][4][0] - 0.25; // 0
						entity->focaly = limbs[SUCCUBUS][4][1] - 0.25; // 0
						entity->focalz = limbs[SUCCUBUS][4][2]; // 2
						entity->sprite = my->sprite == 1126 ? 1124 : 193;
						if ( my->monsterAttack == 0 )
						{
							entity->roll = -PI / 16;
						}
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SUCCUBUS][4][0];
						entity->focaly = limbs[SUCCUBUS][4][1];
						entity->focalz = limbs[SUCCUBUS][4][2];
						entity->sprite = my->sprite == 1126 ? 1125 : 623;
					}
				}
				my->setHumanoidLimbOffset(entity, SUCCUBUS, LIMB_HUMANOID_RIGHTARM);
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			}
			// left arm
			case LIMB_HUMANOID_LEFTARM:
			{
				shieldarm = entity;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						// if weapon invisible, relax arm.
						entity->focalx = limbs[SUCCUBUS][5][0] - 0.25; // 0
						entity->focaly = limbs[SUCCUBUS][5][1] + 0.25; // 0
						entity->focalz = limbs[SUCCUBUS][5][2]; // 2
						entity->sprite = my->sprite == 1126 ? 1122 : 192;
						entity->roll = PI / 16;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SUCCUBUS][5][0];
						entity->focaly = limbs[SUCCUBUS][5][1];
						entity->focalz = limbs[SUCCUBUS][5][2];
						entity->sprite = my->sprite == 1126 ? 1123 : 622;
					}
				}
				my->setHumanoidLimbOffset(entity, SUCCUBUS, LIMB_HUMANOID_LEFTARM);
				if ( my->monsterDefend && my->monsterAttack == 0 )
				{
					MONSTER_SHIELDYAW = PI / 5;
				}
				else
				{
					MONSTER_SHIELDYAW = 0;
				}
				entity->yaw += MONSTER_SHIELDYAW;
				break;
			}
			// weapon
			case LIMB_HUMANOID_WEAPON:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->sprite = itemModel(myStats->weapon);
						if ( itemCategory(myStats->weapon) == SPELLBOOK )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( weaponarm != nullptr )
				{
					my->handleHumanoidWeaponLimb(entity, weaponarm);
				}
				break;
			}
			// shield
			case LIMB_HUMANOID_SHIELD:
			{
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shield == nullptr )
					{
						entity->flags[INVISIBLE] = true;
						entity->sprite = 0;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->shield);
						if ( itemTypeIsQuiver(myStats->shield->type) )
						{
							entity->handleQuiverThirdPersonModel(*myStats);
						}
					}
					if ( myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->handleHumanoidShieldLimb(entity, shieldarm);
				break;
			// cloak
			case LIMB_HUMANOID_CLOAK:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->cloak == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->cloak);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
				// helm
			case LIMB_HUMANOID_HELMET:
				helmet = entity;
				entity->focalx = limbs[SUCCUBUS][9][0]; // 0
				entity->focaly = limbs[SUCCUBUS][9][1]; // 0
				entity->focalz = limbs[SUCCUBUS][9][2]; // -2
				entity->pitch = my->pitch;
				entity->roll = 0;
				if ( multiplayer != CLIENT )
				{
					entity->sprite = itemModel(myStats->helmet);
					if ( myStats->helmet == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->setHelmetLimbOffset(entity);
				break;
				// mask
			case LIMB_HUMANOID_MASK:
				entity->focalx = limbs[SUCCUBUS][10][0]; // 0
				entity->focaly = limbs[SUCCUBUS][10][1]; // 0
				entity->focalz = limbs[SUCCUBUS][10][2]; // .5
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				if ( multiplayer != CLIENT )
				{
					bool hasSteelHelm = false;
					/*if ( myStats->helmet )
					{
						if ( myStats->helmet->type == STEEL_HELM
							|| myStats->helmet->type == CRYSTAL_HELM
							|| myStats->helmet->type == ARTIFACT_HELM )
						{
							hasSteelHelm = true;
						}
					}*/
					if ( myStats->mask == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring || hasSteelHelm ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( myStats->mask != nullptr )
					{
						if ( myStats->mask->type == TOOL_GLASSES )
						{
							entity->sprite = 165; // GlassesWorn.vox
						}
						else if ( myStats->mask->type == MONOCLE )
						{
							entity->sprite = 1196; // monocleWorn.vox
						}
						else
						{
							entity->sprite = itemModel(myStats->mask);
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( entity->sprite != 165 && entity->sprite != 1196 )
				{
					if ( entity->sprite == items[MASK_SHAMAN].index )
					{
						entity->roll = 0;
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else if ( EquipmentModelOffsets.modelOffsetExists(SUCCUBUS, entity->sprite) )
					{
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[SUCCUBUS][10][0] + .35; // .35
						entity->focaly = limbs[SUCCUBUS][10][1] - 2; // -2
						entity->focalz = limbs[SUCCUBUS][10][2]; // .5
					}
				}
				else
				{
					entity->focalx = limbs[SUCCUBUS][10][0] + .25; // .25
					entity->focaly = limbs[SUCCUBUS][10][1] - 2.25; // -2.25
					entity->focalz = limbs[SUCCUBUS][10][2]; // .5

					if ( entity->sprite == 1196 )
					{
						entity->focalx -= .25;
					}
				}
				break;
			}
		}
	}
	// rotate shield a bit
	node_t* shieldNode = list_Node(&my->children, LIMB_HUMANOID_SHIELD);
	if ( shieldNode )
	{
		Entity* shieldEntity = (Entity*)shieldNode->element;
		if ( shieldEntity->sprite != items[TOOL_TORCH].index && shieldEntity->sprite != items[TOOL_LANTERN].index && shieldEntity->sprite != items[TOOL_CRYSTALSHARD].index )
		{
			shieldEntity->yaw -= PI / 6;
		}
	}
	if ( MONSTER_ATTACK > 0 && MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 )
	{
		MONSTER_ATTACKTIME++;
	}
	else if ( MONSTER_ATTACK == 0 )
	{
		MONSTER_ATTACKTIME = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}

void Entity::succubusChooseWeapon(const Entity* target, double dist)
{

	Stat *myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( monsterSpecialState != 0 && monsterSpecialTimer != 0 )
	{
		return;
	}

	if ( monsterSpecialTimer == 0 && (ticks % 10 == 0) && monsterAttack == 0 )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		// try to charm enemy.
		int specialRoll = -1;
		int bonusFromHP = 0;
		specialRoll = local_rng.rand() % 40;
		if ( myStats->HP <= myStats->MAXHP * 0.8 )
		{
			bonusFromHP += 1; // +2.5% chance if on low health
		}
		if ( myStats->HP <= myStats->MAXHP * 0.4 )
		{
			bonusFromHP += 1; // +extra 2.5% chance if on lower health
		}

		int requiredRoll = (1 + bonusFromHP + (targetStats->EFFECTS[EFF_CONFUSED] ? 4 : 0)
			+ (targetStats->EFFECTS[EFF_DRUNK] ? 2 : 0)); // +2.5% base, + extra if target is inebriated

		if ( dist < 40 )
		{
			requiredRoll += 1; //+extra 2.5% chance if dist is smaller.
		}

		if ( specialRoll < requiredRoll )
		{
			node_t* node = nullptr;
			node = itemNodeInInventory(myStats, -1, SPELLBOOK);
			if ( node != nullptr )
			{
				swapMonsterWeaponWithInventoryItem(this, myStats, node, true, true);
				monsterSpecialState = SUCCUBUS_CHARM;
				return;
			}
		}
	}
}