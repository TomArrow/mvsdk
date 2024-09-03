
#ifndef G_DEFRAG_H
#define G_DEFRAG_H

#include "q_shared.h"


void G_TurnDefragTargetsIntoTriggers();
qboolean MovementStyleAllowsWeapons(int moveStyle);
void PlayerSnapshotHackValues(qboolean saveState, int clientNum);
void PlayerSnapshotRestoreValues();

#endif
