#include "g_local.h"

void G_ResetClientDebugInfoUpdates() {
	int i;
	for (i = 0; i < level.maxclients; i++) {
		if (!g_entities[i].client || !g_entities[i].inuse) {
			continue;
		}
		g_entities[i].client->pers.lastDebugFieldsUpdateTime = -99999;
	}	
}
void G_ResetDebugVars() {
	level.debugState.debugVarCount = 0;
	G_ResetClientDebugInfoUpdates();
}
void G_ResetBenchmarking() {
	G_COOL_API_Benchmark(0,0,0,0,NULL,0);
}

void G_SetDebugVar(debugField_t* field, int value, float floatValue) {
	entityState_t* es;
	floatint_t fi;
	int* target;
	if (!level.debugState.ent || !field) {
		return;
	}
	es = &level.debugState.ent->s;
	if (field->bits == 0) {
		fi.f = floatValue;
		value = fi.i;
	}
	target = (int*)((byte*)es + field->offset);
	*target = value;
}

debugField_t* G_GetDebugVar(const char* name, int minBits, qboolean isFloat, int flags) {
	int i,j,bestMatchBits,hereBits;
	qboolean isSigned = minBits < 0;
	debugField_t* field = debugFields, * best = NULL;
	qboolean fieldOK;
	minBits = isSigned ? -minBits : minBits;
	bestMatchBits = 9999;
	for (i = 0; i < debugFieldCount; i++, field++) {
		hereBits = field->bits < 0 ? -field->bits : field->bits;
		if (isFloat) {
			if (field->bits != 0) {
				continue;
			}
		}
		else if (!isFloat) {
			if (isSigned && field->bits > 0 || !isSigned && field->bits < 0 || hereBits < minBits) {
				continue;
			}
		}
		fieldOK = qtrue;
		for (j = 0; j < level.debugState.debugVarCount; j++) {
			// check if it's already used
			if (level.debugState.debugVars[j].field == field) {
				fieldOK = qfalse;
				break;
			}
		}
		if (!fieldOK) {
			continue;
		}
		if (isFloat) {
			best = field;
			break;
		}
		else {
			// this match is more efficient than the previous one
			if (hereBits < bestMatchBits || !best) {
				best = field;
				bestMatchBits = hereBits;
			}
		}
	}
	if (best) {
		level.debugState.debugVars[level.debugState.debugVarCount].field = best;
		level.debugState.debugVars[level.debugState.debugVarCount].flags = flags;
		Q_strncpyz(level.debugState.debugVars[level.debugState.debugVarCount].name,name,sizeof(level.debugState.debugVars[level.debugState.debugVarCount].name));
		level.debugState.debugVarCount++;
	}

	return best;
}

void G_InitDebugAntiwallhack() {
	G_ResetDebugVars();
	antiWhDebug.tracesPerSecondCountFloat = G_GetDebugVar("Traces per second (count)", 0, qtrue,0);
	antiWhDebug.pointContentsPerSecondCountFloat = G_GetDebugVar("Point contents per second (count)", 0, qtrue,0);
	antiWhDebug.tracesPerSecondSpeedFloat = G_GetDebugVar("Traces per second (speed)", 0, qtrue,0);
	level.debugState.debug = DEBUG_ANTIWALLHACK;
	G_ResetClientDebugInfoUpdates();
	G_COOL_API_Benchmark(BENCHMARK_SETMEASUREMENTS | BENCHMARK_MEASURE_VMTARGET_GAME | BENCHMARK_MEASURE_TRACES_MARKED,0,0,0,NULL,0);
}


const char* G_DebugCreateInfoPrint() {
	static char text[MAX_STRING_CHARS];
	int stringlength = 0;
	int i, j;
	const char* newEntry;

	text[0] = '\0';
	Com_sprintf(text,sizeof(text),"print \"\" debugFields %d ", level.debugState.debugVarCount);
	stringlength = strlen(text);
	for (i = 0; i < level.debugState.debugVarCount; i++) {
		newEntry = va("\"%s\" \"%s\" %d %d", level.debugState.debugVars[i].name, level.debugState.debugVars[i].field->name,level.debugState.debugVars[i].field->bits, level.debugState.debugVars[i].flags);
		j = strlen(newEntry);
		if (stringlength + j > 1022)
			break;
		Q_strncpyz(text + stringlength, newEntry, sizeof(text) - stringlength);
		stringlength += j;
	}

	return text;
}

void G_DebugHandleState() {
	int i;
	static int g_debugFancyModificationCount = -1;
	if (g_debugFancy.modificationCount != g_debugFancyModificationCount) {
		g_debugFancyModificationCount = g_debugFancy.modificationCount;
		level.debugState.debug = DEBUG_NONE;
		G_ResetBenchmarking();
		if (g_debugFancy.integer && level.debugState.debug != g_debugFancy.integer) {
			switch (g_debugFancy.integer) {
			case DEBUG_ANTIWALLHACK:
				trap_SendServerCommand(-1, "Debugging anti-wallhack.\n");
				G_InitDebugAntiwallhack();
				break;
			default:
				trap_Cvar_Set("g_debugFancy", "0");
				trap_SendServerCommand(-1, "g_debugFancy: Invalid value. Turning off.\n");
				level.debugState.debug = DEBUG_NONE;
				break;
			}
		}
		G_ResetClientDebugInfoUpdates();
		if (!level.debugState.debug) {
			trap_SendServerCommand(-1, "print \"\" debugFields 0");
		}
	}
	
	handleentity:

	if (level.debugState.debug) {
		if (!level.debugState.ent) {
			gentity_t* ent = G_Spawn();
			G_SetClassName(ent, "debugdata");
			//ent->neverFree = qtrue;
			ent->s.eType = ET_INVISIBLE;
			ent->s.modelGhoul2 = 14; // tell tommyternal cgame that this is a debug data object
			ent->s.pos.trType = TR_STATIONARY; // avoid weirdness from interpolation
			ent->s.apos.trType = TR_STATIONARY; // avoid weirdness from interpolation
			ent->s.constantLight = 0; // avoid creating lights
			ent->s.loopSound = 0; // avoid creating sounds
			ent->s.solid = 0; // avoid interpreting as bmoodel
			ent->r.svFlags |= SVF_BROADCAST;
			trap_LinkEntity(ent);
			level.debugState.ent = ent;
		}

	}
	else if(level.debugState.ent) {
		G_FreeEntity(level.debugState.ent);
		level.debugState.ent = NULL;
	}

	// update the current debug info for anyone who needs it.
	if (level.debugState.ent && level.debugState.debug) {
		const char* debugInfoPrint = NULL;
		gentity_t* ent = g_entities;
		for (i = 0; i < level.maxclients; i++, ent++) {
			if (!ent->inuse || !ent->client){
				continue;
			}
			if (level.time - ent->client->pers.lastDebugFieldsUpdateTime > 10000 || ent->client->pers.lastDebugFieldsUpdateTime > level.time) {
				if (!debugInfoPrint) {
					debugInfoPrint = G_DebugCreateInfoPrint();
				}
				trap_SendServerCommand(i,debugInfoPrint);
				ent->client->pers.lastDebugFieldsUpdateTime = level.time;
			}
		}
	}
}


float G_COOL_API_Benchmark(const int flags, const int param1, const int param2, const int param3, float* multiResultArr, const int multiResultArrSize) {
	floatint_t result;
	if (!(coolApi & COOL_APIFEATURE_BENCHMARKING)) {
		return -1;
	}
	result.i = trap_G_COOL_API_Benchmark(flags,param1,param2,param3,multiResultArr,multiResultArrSize);
	return result.f;
}






