
#include "cg_local.h"
#include "../ui/ui_shared.h"

void CG_DebugFieldsCommand() {

	const char* s;
	int count,countMax = (trap_Argc()-4)/4;
	int i,j;
	int bits;
	int flags;
	cgs.debugState.debugVarCount = 0;
	if (!cgs.isTommyTernal || trap_Argc() < 4) {
		return;
	}
	s = CG_Argv(3);
	count = atoi(s);
	if (count > countMax) {
		if (cg_developer.integer) {
			CG_Printf("^3CG_DebugFieldsCommand: %d fields expected, only %d fields gotten.\n", count, countMax);
		}
		count = countMax;
	}
	for (i = 0; i < count; i++) {
		bits = atoi(CG_Argv(4+i*4+2));
		flags = atoi(CG_Argv(4+i*4+3));
		s = CG_Argv(4 + i * 4 + 1); // field name
		// see if we can find a field name like that
		for (j = 0; j < debugFieldCount; j++) {
			if (!strcmp(debugFields[j].name, s)) {
				// found a match.
				s = CG_Argv(4 + i * 4); // display name
				cgs.debugState.debugVars[cgs.debugState.debugVarCount].field = &debugFields[j];
				cgs.debugState.debugVars[cgs.debugState.debugVarCount].flags = flags;
				Q_strncpyz(cgs.debugState.debugVars[cgs.debugState.debugVarCount].name,s,sizeof(cgs.debugState.debugVars[cgs.debugState.debugVarCount].name));
				if (cg_developer.integer && debugFields[j].bits != bits) {
					CG_Printf("^3CG_DebugFieldsCommand: field %s: %d bits expected, but %d fields sent?\n", debugFields[j].name, debugFields[j].bits, bits);
				}
				cgs.debugState.debugVarCount++;
				break;
			}
		}
	}
}

floatint_t CG_GetDebugVar(debugField_t* field, entityState_t* es) {
	floatint_t retVal = { 0 };
	int* source;
	if (!es || !field) {
		return retVal;
	}
	source = (int*)((byte*)es + field->offset);
	retVal.i = *source;
	return retVal;
}

#define DEBUGDRAWAVGSIZE 128
typedef struct avgBuffer_s {
	floatint_t		avgBuffers[DEBUGDRAWAVGSIZE];
	unsigned int	count;
} avgBuffer_t;

void CG_DebugDraw() {
	//static char output[8192];
	static avgBuffer_t avgBuffers[MAXDEBUGVARS];
	int i,j;
	const float scale = 0.4f;
	int textHeight = CG_Text_Height("A", scale, FONT_NONE);
	int yOffset = 0;
	debugVar_t* var;
	floatint_t val;
	if (!cg_drawDebugFancy.integer || !cgs.isTommyTernal || !cgs.debugState.ent || !cgs.debugState.debugVarCount) {
		return;
	}
	//output[0] = '\0';
	var = cgs.debugState.debugVars;
	for (i = 0; i < cgs.debugState.debugVarCount; i++, var++) {
		if (!var->field) {
			continue;
		}
		val = CG_GetDebugVar(var->field, &cgs.debugState.ent->currentState);

		CG_Text_Paint(150, 250+ yOffset, scale, colorWhite, va("^7^0^7%s:", var->name), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_NONE, NULL);
		avgBuffers[i].avgBuffers[avgBuffers[i].count++ % DEBUGDRAWAVGSIZE] = val;
		if (var->field->bits == 0) {
			CG_Text_Paint(250, 250 + yOffset, scale, colorWhite, va("%.2f", val.f), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_NONE, NULL);
		}
		else {
			CG_Text_Paint(250, 250 + yOffset, scale, colorWhite, va("%d", val.i), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_NONE, NULL);
		}
		if (avgBuffers[i].count >= DEBUGDRAWAVGSIZE) {
			floatint_t sum = { 0 };
			for (j = 0; j < DEBUGDRAWAVGSIZE; j++) {
				if (var->field->bits == 0) {
					sum.f += avgBuffers[i].avgBuffers[j].f;
				}
				else {
					sum.i += avgBuffers[i].avgBuffers[j].i;
				}
			}
			if (var->field->bits == 0) {
				CG_Text_Paint(350, 250 + yOffset, scale, colorWhite, va("%.2f", sum.f/(float)DEBUGDRAWAVGSIZE), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_NONE, NULL);
			}
			else {
				CG_Text_Paint(350, 250 + yOffset, scale, colorWhite, va("%.2f", (float)sum.i/(float)DEBUGDRAWAVGSIZE), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_NONE, NULL);
			}
		}
		yOffset += textHeight;
	}
}
float CG_COOL_API_Benchmark(const int flags, const int param1, const int param2, const int param3, float* multiResultArr, const int multiResultArrSize) {
	floatint_t result;
	if (!(coolApi & COOL_APIFEATURE_BENCHMARKING)) {
		return -1;
	}
	result.i = trap_CG_COOL_API_Benchmark(flags, param1, param2, param3, multiResultArr, multiResultArrSize);
	return result.f;
}



