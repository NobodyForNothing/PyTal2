#include "EntityList.hpp"

#include "Command.hpp"
#include "Modules/Console.hpp"
#include "Modules/Server.hpp"
#include "Modules/Engine.hpp"
#include "Features/Session.hpp"
#include "Features/Camera.hpp"
#include "Offsets.hpp"
#include "PYTAL.hpp"

#include <algorithm>
#include <cstring>
#include <deque>

EntityList *entityList;

EntityList::EntityList() {
	this->hasLoaded = true;
}
CEntInfo *EntityList::GetEntityInfoByIndex(int index) {
	return reinterpret_cast<CEntInfo *>((uintptr_t)server->m_EntPtrArray + sizeof(CEntInfo) * index);
}
CEntInfo *EntityList::GetEntityInfoByName(const char *name) {
	for (auto index = 0; index < Offsets::NUM_ENT_ENTRIES; ++index) {
		auto info = this->GetEntityInfoByIndex(index);
		if (info->m_pEntity == nullptr) {
			continue;
		}

		auto match = server->GetEntityName(info->m_pEntity);
		if (!match || std::strcmp(match, name) != 0) {
			continue;
		}

		return info;
	}

	return nullptr;
}
CEntInfo *EntityList::GetEntityInfoByClassName(const char *name) {
	for (auto index = 0; index < Offsets::NUM_ENT_ENTRIES; ++index) {
		auto info = this->GetEntityInfoByIndex(index);
		if (info->m_pEntity == nullptr) {
			continue;
		}

		auto match = server->GetEntityClassName(info->m_pEntity);
		if (!match || std::strcmp(match, name) != 0) {
			continue;
		}

		return info;
	}

	return nullptr;
}
int EntityList::GetEntityInfoIndexByHandle(void *entity) {
	if (entity == nullptr) return -1;
	for (auto index = 0; index < Offsets::NUM_ENT_ENTRIES; ++index) {
		auto info = this->GetEntityInfoByIndex(index);
		if (info->m_pEntity != entity) {
			continue;
		}
		return index;
	}
	return -1;
}

IHandleEntity *EntityList::LookupEntity(const CBaseHandle &handle) {
	if ((unsigned)handle.m_Index == (unsigned)Offsets::INVALID_EHANDLE_INDEX)
		return NULL;

	auto pInfo = this->GetEntityInfoByIndex(handle.GetEntryIndex());

	if (pInfo->m_SerialNumber == handle.GetSerialNumber())
		return (IHandleEntity *)pInfo->m_pEntity;
	else
		return NULL;
}

// returns an entity with given targetname/classname
// supports array brackets
CEntInfo *EntityList::QuerySelector(const char *selector) {
	int slen = strlen(selector);
	int entId = 0;
	if (selector[slen - 1] == ']') {
		int openBracket = slen - 2;
		while (openBracket > 0 && selector[openBracket] != '[') {
			openBracket--;
		}
		if (selector[openBracket] == '[') {
			if (slen - openBracket - 2 > 0) {
				std::string entIdStr(selector + openBracket + 1, slen - openBracket - 2);
				entId = std::atoi(entIdStr.c_str());
			}
			slen = openBracket;
		}
	}
	std::string selectorStr(selector, slen);

	// TODO: maybe implement an * wildcard support here as well?

	for (auto i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {

		auto info = entityList->GetEntityInfoByIndex(i);
		if (info->m_pEntity == nullptr) {
			continue;
		}

		auto tname = server->GetEntityName(info->m_pEntity);
		if (!tname || std::strcmp(tname, selectorStr.c_str()) != 0) {
			auto cname = server->GetEntityClassName(info->m_pEntity);
			if (!cname || std::strcmp(cname, selectorStr.c_str()) != 0) {
				continue;
			}
		}

		if (entId == 0) {
			return info;
		} else {
			entId--;
		}
	}

	return NULL;
}


// Commands
static void dumpEntInfo(void *entity) {
	if (entity == nullptr) return;
	auto index = entityList->GetEntityInfoIndexByHandle(entity);
	if (index == -1) return;
	auto info = entityList->GetEntityInfoByIndex(index);

	const char *targetname = server->GetEntityName(entity);
	const char *classname = server->GetEntityClassName(entity);

	if (!targetname) targetname = "<no name>";
	if (!classname) classname = "<no class>";

	console->Print("[%i] ", index);
	console->Msg("(%i) ", info->m_SerialNumber);
	console->Msg("%p", info->m_pEntity);
	console->Print(" -> ");
	console->Msg("%s (%s)\n", targetname, classname);

	ICollideable *coll = &SE(entity)->collision();

	ServerEnt *se = (ServerEnt *)entity;
	Vector origin = se->abs_origin();
	console->Print("origin: ");
	console->Msg("%.2f %.2f %.2f\n", origin.x, origin.y, origin.z);

	QAngle angles = se->abs_angles();
	console->Print("angles: ");
	console->Msg("%.2f %.2f %.2f\n", angles.x, angles.y, angles.z);

	Vector velocity = se->abs_velocity();
	console->Print("velocity: ");
	console->Msg("%.2f %.2f %.2f\n", velocity.x, velocity.y, velocity.z);

	console->Print("flags:  ");
	console->Msg("%08X\n", coll->GetSolidFlags());

	if (coll->GetSolidFlags() & FSOLID_NOT_SOLID) {
		console->Msg(" - FSOLID_NOT_SOLID\n");
	}

	if (coll->GetSolidFlags() & FSOLID_NOT_STANDABLE) {
		console->Msg(" - FSOLID_NOT_STANDABLE\n");
	}

	if (coll->GetSolidFlags() & FSOLID_VOLUME_CONTENTS) {
		console->Msg(" - FSOLID_VOLUME_CONTENTS\n");
	}

	console->Print("solid type:  ");
	console->Msg("%d\n", (int)coll->GetSolid());

	console->Print("collision group:  ");
	console->Msg("0x%08X\n", coll->GetCollisionGroup());
}
