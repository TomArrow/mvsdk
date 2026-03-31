
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
			CG_Printf("^3CG_DebugFieldsCommand: %d fields expected, only %d fields gotten.\n");
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

void CG_DebugDraw() {
	static char output[8192];
	int i;
	debugVar_t* var;
	floatint_t val;
	if (!cg_drawDebugFancy.integer || !cgs.isTommyTernal || !cgs.debugState.ent || !cgs.debugState.debugVarCount) {
		return;
	}
	output[0] = '\0';
	var = cgs.debugState.debugVars;
	for (i = 0; i < cgs.debugState.debugVarCount; i++, var++) {
		if (!var->field) {
			continue;
		}
		val = CG_GetDebugVar(var->field, &cgs.debugState.ent->currentState);
		if (var->field->bits == 0) {
			Q_strcat(output,sizeof(output),va("^7^0^7%s: %f\n",var->name, val.f));
		}
		else {
			Q_strcat(output, sizeof(output), va("^7^0^7%s: %d\n", var->name, val.i));
		}
	}
	CG_Text_Paint(150,250,0.4f,colorWhite, output,0,0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_NONE,NULL);
}

