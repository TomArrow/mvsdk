code

equ trap_Printf											  -1	; G_PRINT 
equ trap_Error											  -2	; G_ERROR 
equ trap_Milliseconds									  -3	; G_MILLISECONDS 
equ trap_Cvar_Register									  -4	; G_CVAR_REGISTER 
equ trap_Cvar_Update									  -5	; G_CVAR_UPDATE 
equ trap_Cvar_Set										  -6	; G_CVAR_SET 
equ trap_Cvar_VariableIntegerValue						  -7	; G_CVAR_VARIABLE_INTEGER_VALUE 
equ trap_Cvar_VariableStringBuffer						  -8	; G_CVAR_VARIABLE_STRING_BUFFER 
equ trap_Argc											  -9	; G_ARGC 
equ trap_Argv											 -10	; G_ARGV 
equ trap_FS_FOpenFile									 -11	; G_FS_FOPEN_FILE 
equ trap_FS_Read										 -12	; G_FS_READ 
equ trap_FS_Write										 -13	; G_FS_WRITE 
equ trap_FS_FCloseFile									 -14	; G_FS_FCLOSE_FILE 
equ trap_SendConsoleCommand								 -15	; G_SEND_CONSOLE_COMMAND 
equ trap_LocateGameData									 -16	; G_LOCATE_GAME_DATA 
equ trap_DropClient										 -17	; G_DROP_CLIENT 
equ trap_SendServerCommand								 -18	; G_SEND_SERVER_COMMAND 
equ trap_SetConfigstring								 -19	; G_SET_CONFIGSTRING 
equ trap_GetConfigstring								 -20	; G_GET_CONFIGSTRING 
equ trap_GetUserinfo									 -21	; G_GET_USERINFO 
equ trap_SetUserinfo									 -22	; G_SET_USERINFO 
equ trap_GetServerinfo									 -23	; G_GET_SERVERINFO 
equ trap_SetBrushModel									 -24	; G_SET_BRUSH_MODEL 
equ trap_Trace											 -25	; G_TRACE 
equ trap_PointContents									 -26	; G_POINT_CONTENTS 
equ trap_InPVS											 -27	; G_IN_PVS 
equ trap_InPVSIgnorePortals								 -28	; G_IN_PVS_IGNORE_PORTALS 
equ trap_AdjustAreaPortalState							 -29	; G_ADJUST_AREA_PORTAL_STATE 
equ trap_AreasConnected									 -30	; G_AREAS_CONNECTED 
equ trap_LinkEntity										 -31	; G_LINKENTITY 
equ trap_UnlinkEntity									 -32	; G_UNLINKENTITY 
equ trap_EntitiesInBox									 -33	; G_ENTITIES_IN_BOX 
equ trap_EntityContact									 -34	; G_ENTITY_CONTACT 
equ trap_BotAllocateClient								 -35	; G_BOT_ALLOCATE_CLIENT 
equ trap_BotFreeClient									 -36	; G_BOT_FREE_CLIENT 
equ trap_GetUsercmd										 -37	; G_GET_USERCMD 
equ trap_GetEntityToken									 -38	; G_GET_ENTITY_TOKEN 
equ trap_FS_GetFileList									 -39	; G_FS_GETFILELIST 
equ trap_DebugPolygonCreate								 -40	; G_DEBUG_POLYGON_CREATE 
equ trap_DebugPolygonDelete								 -41	; G_DEBUG_POLYGON_DELETE 
equ trap_RealTime										 -42	; G_REAL_TIME 
equ trap_SnapVector										 -43	; G_SNAPVECTOR 
equ trap_TraceCapsule									 -44	; G_TRACECAPSULE 
equ trap_EntityContactCapsule							 -45	; G_ENTITY_CONTACTCAPSULE 
equ trap_SP_RegisterServer								 -46	; SP_REGISTER_SERVER_CMD 
equ trap_SP_GetStringTextString							 -47	; SP_GETSTRINGTEXTSTRING 
equ trap_ROFF_Clean										 -48	; G_ROFF_CLEAN 
equ trap_ROFF_UpdateEntities							 -49	; G_ROFF_UPDATE_ENTITIES 
equ trap_ROFF_Cache										 -50	; G_ROFF_CACHE 
equ trap_ROFF_Play										 -51	; G_ROFF_PLAY 
equ trap_ROFF_Purge_Ent									 -52	; G_ROFF_PURGE_ENT 
equ trap_BotLibSetup									-201	; BOTLIB_SETUP 
equ trap_BotLibShutdown									-202	; BOTLIB_SHUTDOWN 
equ trap_BotLibVarSet									-203	; BOTLIB_LIBVAR_SET 
equ trap_BotLibVarGet									-204	; BOTLIB_LIBVAR_GET 
equ trap_BotLibDefine									-205	; BOTLIB_PC_ADD_GLOBAL_DEFINE 
equ trap_BotLibStartFrame								-206	; BOTLIB_START_FRAME 
equ trap_BotLibLoadMap									-207	; BOTLIB_LOAD_MAP 
equ trap_BotLibUpdateEntity								-208	; BOTLIB_UPDATENTITY 
equ trap_BotLibTest										-209	; BOTLIB_TEST 
equ trap_BotGetSnapshotEntity							-210	; BOTLIB_GET_SNAPSHOT_ENTITY 
equ trap_BotGetServerCommand							-211	; BOTLIB_GET_CONSOLE_MESSAGE 
equ trap_BotUserCommand									-212	; BOTLIB_USER_COMMAND 
equ trap_AAS_EntityInfo									-304	; BOTLIB_AAS_ENTITY_INFO 
equ trap_AAS_Initialized								-305	; BOTLIB_AAS_INITIALIZED 
equ trap_AAS_PresenceTypeBoundingBox					-306	; BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX 
equ trap_AAS_Time										-307	; BOTLIB_AAS_TIME 
equ trap_AAS_PointAreaNum								-308	; BOTLIB_AAS_POINT_AREA_NUM 
equ trap_AAS_PointReachabilityAreaIndex					-578	; BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX 
equ trap_AAS_TraceAreas									-309	; BOTLIB_AAS_TRACE_AREAS 
equ trap_AAS_BBoxAreas									-302	; BOTLIB_AAS_BBOX_AREAS 
equ trap_AAS_AreaInfo									-303	; BOTLIB_AAS_AREA_INFO 
equ trap_AAS_PointContents								-310	; BOTLIB_AAS_POINT_CONTENTS 
equ trap_AAS_NextBSPEntity								-311	; BOTLIB_AAS_NEXT_BSP_ENTITY 
equ trap_AAS_ValueForBSPEpairKey						-312	; BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY 
equ trap_AAS_VectorForBSPEpairKey						-313	; BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY 
equ trap_AAS_FloatForBSPEpairKey						-314	; BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY 
equ trap_AAS_IntForBSPEpairKey							-315	; BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY 
equ trap_AAS_AreaReachability							-316	; BOTLIB_AAS_AREA_REACHABILITY 
equ trap_AAS_AreaTravelTimeToGoalArea					-317	; BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA 
equ trap_AAS_EnableRoutingArea							-301	; BOTLIB_AAS_ENABLE_ROUTING_AREA 
equ trap_AAS_PredictRoute								-577	; BOTLIB_AAS_PREDICT_ROUTE 
equ trap_AAS_AlternativeRouteGoals						-576	; BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL 
equ trap_AAS_Swimming									-318	; BOTLIB_AAS_SWIMMING 
equ trap_AAS_PredictClientMovement						-319	; BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT 
equ trap_EA_Say											-401	; BOTLIB_EA_SAY 
equ trap_EA_SayTeam										-402	; BOTLIB_EA_SAY_TEAM 
equ trap_EA_Command										-403	; BOTLIB_EA_COMMAND 
equ trap_EA_Action										-404	; BOTLIB_EA_ACTION 
equ trap_EA_Gesture										-405	; BOTLIB_EA_GESTURE 
equ trap_EA_Talk										-406	; BOTLIB_EA_TALK 
equ trap_EA_Attack										-407	; BOTLIB_EA_ATTACK 
equ trap_EA_Alt_Attack									-408	; BOTLIB_EA_ALT_ATTACK 
equ trap_EA_ForcePower									-409	; BOTLIB_EA_FORCEPOWER 
equ trap_EA_Use											-410	; BOTLIB_EA_USE 
equ trap_EA_Respawn										-411	; BOTLIB_EA_RESPAWN 
equ trap_EA_Crouch										-412	; BOTLIB_EA_CROUCH 
equ trap_EA_MoveUp										-413	; BOTLIB_EA_MOVE_UP 
equ trap_EA_MoveDown									-414	; BOTLIB_EA_MOVE_DOWN 
equ trap_EA_MoveForward									-415	; BOTLIB_EA_MOVE_FORWARD 
equ trap_EA_MoveBack									-416	; BOTLIB_EA_MOVE_BACK 
equ trap_EA_MoveLeft									-417	; BOTLIB_EA_MOVE_LEFT 
equ trap_EA_MoveRight									-418	; BOTLIB_EA_MOVE_RIGHT 
equ trap_EA_SelectWeapon								-419	; BOTLIB_EA_SELECT_WEAPON 
equ trap_EA_Jump										-420	; BOTLIB_EA_JUMP 
equ trap_EA_DelayedJump									-421	; BOTLIB_EA_DELAYED_JUMP 
equ trap_EA_Move										-422	; BOTLIB_EA_MOVE 
equ trap_EA_View										-423	; BOTLIB_EA_VIEW 
equ trap_EA_EndRegular									-424	; BOTLIB_EA_END_REGULAR 
equ trap_EA_GetInput									-425	; BOTLIB_EA_GET_INPUT 
equ trap_EA_ResetInput									-426	; BOTLIB_EA_RESET_INPUT 
equ trap_BotLoadCharacter								-501	; BOTLIB_AI_LOAD_CHARACTER 
equ trap_BotFreeCharacter								-502	; BOTLIB_AI_FREE_CHARACTER 
equ trap_Characteristic_Float							-503	; BOTLIB_AI_CHARACTERISTIC_FLOAT 
equ trap_Characteristic_BFloat							-504	; BOTLIB_AI_CHARACTERISTIC_BFLOAT 
equ trap_Characteristic_Integer							-505	; BOTLIB_AI_CHARACTERISTIC_INTEGER 
equ trap_Characteristic_BInteger						-506	; BOTLIB_AI_CHARACTERISTIC_BINTEGER 
equ trap_Characteristic_String							-507	; BOTLIB_AI_CHARACTERISTIC_STRING 
equ trap_BotAllocChatState								-508	; BOTLIB_AI_ALLOC_CHAT_STATE 
equ trap_BotFreeChatState								-509	; BOTLIB_AI_FREE_CHAT_STATE 
equ trap_BotQueueConsoleMessage							-510	; BOTLIB_AI_QUEUE_CONSOLE_MESSAGE 
equ trap_BotRemoveConsoleMessage						-511	; BOTLIB_AI_REMOVE_CONSOLE_MESSAGE 
equ trap_BotNextConsoleMessage							-512	; BOTLIB_AI_NEXT_CONSOLE_MESSAGE 
equ trap_BotNumConsoleMessages							-513	; BOTLIB_AI_NUM_CONSOLE_MESSAGE 
equ trap_BotInitialChat									-514	; BOTLIB_AI_INITIAL_CHAT 
equ trap_BotNumInitialChats								-570	; BOTLIB_AI_NUM_INITIAL_CHATS 
equ trap_BotReplyChat									-515	; BOTLIB_AI_REPLY_CHAT 
equ trap_BotChatLength									-516	; BOTLIB_AI_CHAT_LENGTH 
equ trap_BotEnterChat									-517	; BOTLIB_AI_ENTER_CHAT 
equ trap_BotGetChatMessage								-571	; BOTLIB_AI_GET_CHAT_MESSAGE 
equ trap_StringContains									-518	; BOTLIB_AI_STRING_CONTAINS 
equ trap_BotFindMatch									-519	; BOTLIB_AI_FIND_MATCH 
equ trap_BotMatchVariable								-520	; BOTLIB_AI_MATCH_VARIABLE 
equ trap_UnifyWhiteSpaces								-521	; BOTLIB_AI_UNIFY_WHITE_SPACES 
equ trap_BotReplaceSynonyms								-522	; BOTLIB_AI_REPLACE_SYNONYMS 
equ trap_BotLoadChatFile								-523	; BOTLIB_AI_LOAD_CHAT_FILE 
equ trap_BotSetChatGender								-524	; BOTLIB_AI_SET_CHAT_GENDER 
equ trap_BotSetChatName									-525	; BOTLIB_AI_SET_CHAT_NAME 
equ trap_BotResetGoalState								-526	; BOTLIB_AI_RESET_GOAL_STATE 
equ trap_BotResetAvoidGoals								-527	; BOTLIB_AI_RESET_AVOID_GOALS 
equ trap_BotRemoveFromAvoidGoals						-572	; BOTLIB_AI_REMOVE_FROM_AVOID_GOALS 
equ trap_BotPushGoal									-528	; BOTLIB_AI_PUSH_GOAL 
equ trap_BotPopGoal										-529	; BOTLIB_AI_POP_GOAL 
equ trap_BotEmptyGoalStack								-530	; BOTLIB_AI_EMPTY_GOAL_STACK 
equ trap_BotDumpAvoidGoals								-531	; BOTLIB_AI_DUMP_AVOID_GOALS 
equ trap_BotDumpGoalStack								-532	; BOTLIB_AI_DUMP_GOAL_STACK 
equ trap_BotGoalName									-533	; BOTLIB_AI_GOAL_NAME 
equ trap_BotGetTopGoal									-534	; BOTLIB_AI_GET_TOP_GOAL 
equ trap_BotGetSecondGoal								-535	; BOTLIB_AI_GET_SECOND_GOAL 
equ trap_BotChooseLTGItem								-536	; BOTLIB_AI_CHOOSE_LTG_ITEM 
equ trap_BotChooseNBGItem								-537	; BOTLIB_AI_CHOOSE_NBG_ITEM 
equ trap_BotTouchingGoal								-538	; BOTLIB_AI_TOUCHING_GOAL 
equ trap_BotItemGoalInVisButNotVisible					-539	; BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE 
equ trap_BotGetLevelItemGoal							-540	; BOTLIB_AI_GET_LEVEL_ITEM_GOAL 
equ trap_BotGetNextCampSpotGoal							-568	; BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL 
equ trap_BotGetMapLocationGoal							-569	; BOTLIB_AI_GET_MAP_LOCATION_GOAL 
equ trap_BotAvoidGoalTime								-541	; BOTLIB_AI_AVOID_GOAL_TIME 
equ trap_BotSetAvoidGoalTime							-574	; BOTLIB_AI_SET_AVOID_GOAL_TIME 
equ trap_BotInitLevelItems								-542	; BOTLIB_AI_INIT_LEVEL_ITEMS 
equ trap_BotUpdateEntityItems							-543	; BOTLIB_AI_UPDATE_ENTITY_ITEMS 
equ trap_BotLoadItemWeights								-544	; BOTLIB_AI_LOAD_ITEM_WEIGHTS 
equ trap_BotFreeItemWeights								-545	; BOTLIB_AI_FREE_ITEM_WEIGHTS 
equ trap_BotInterbreedGoalFuzzyLogic					-566	; BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC 
equ trap_BotSaveGoalFuzzyLogic							-546	; BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC 
equ trap_BotMutateGoalFuzzyLogic						-567	; BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC 
equ trap_BotAllocGoalState								-547	; BOTLIB_AI_ALLOC_GOAL_STATE 
equ trap_BotFreeGoalState								-548	; BOTLIB_AI_FREE_GOAL_STATE 
equ trap_BotResetMoveState								-549	; BOTLIB_AI_RESET_MOVE_STATE 
equ trap_BotAddAvoidSpot								-575	; BOTLIB_AI_ADD_AVOID_SPOT 
equ trap_BotMoveToGoal									-550	; BOTLIB_AI_MOVE_TO_GOAL 
equ trap_BotMoveInDirection								-551	; BOTLIB_AI_MOVE_IN_DIRECTION 
equ trap_BotResetAvoidReach								-552	; BOTLIB_AI_RESET_AVOID_REACH 
equ trap_BotResetLastAvoidReach							-553	; BOTLIB_AI_RESET_LAST_AVOID_REACH 
equ trap_BotReachabilityArea							-554	; BOTLIB_AI_REACHABILITY_AREA 
equ trap_BotMovementViewTarget							-555	; BOTLIB_AI_MOVEMENT_VIEW_TARGET 
equ trap_BotPredictVisiblePosition						-573	; BOTLIB_AI_PREDICT_VISIBLE_POSITION 
equ trap_BotAllocMoveState								-556	; BOTLIB_AI_ALLOC_MOVE_STATE 
equ trap_BotFreeMoveState								-557	; BOTLIB_AI_FREE_MOVE_STATE 
equ trap_BotInitMoveState								-558	; BOTLIB_AI_INIT_MOVE_STATE 
equ trap_BotChooseBestFightWeapon						-559	; BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON 
equ trap_BotGetWeaponInfo								-560	; BOTLIB_AI_GET_WEAPON_INFO 
equ trap_BotLoadWeaponWeights							-561	; BOTLIB_AI_LOAD_WEAPON_WEIGHTS 
equ trap_BotAllocWeaponState							-562	; BOTLIB_AI_ALLOC_WEAPON_STATE 
equ trap_BotFreeWeaponState								-563	; BOTLIB_AI_FREE_WEAPON_STATE 
equ trap_BotResetWeaponState							-564	; BOTLIB_AI_RESET_WEAPON_STATE 
equ trap_GeneticParentsAndChildSelection				-565	; BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION 
equ trap_PC_LoadSource									-579	; BOTLIB_PC_LOAD_SOURCE 
equ trap_PC_FreeSource									-580	; BOTLIB_PC_FREE_SOURCE 
equ trap_PC_ReadToken									-581	; BOTLIB_PC_READ_TOKEN 
equ trap_PC_SourceFileAndLine							-582	; BOTLIB_PC_SOURCE_FILE_AND_LINE 
equ trap_G2_ListModelBones								-583	; G_G2_LISTBONES 
equ trap_G2_ListModelSurfaces							-584	; G_G2_LISTSURFACES 
equ trap_G2_HaveWeGhoul2Models							-585	; G_G2_HAVEWEGHOULMODELS 
equ trap_G2_SetGhoul2ModelIndexes						-586	; G_G2_SETMODELS 
equ trap_G2API_GetBoltMatrix							-587	; G_G2_GETBOLT 
equ trap_G2API_GetBoltMatrix_NoReconstruct				-588	; G_G2_GETBOLT_NOREC 

equ trap_G2API_InitGhoul2Model_1_02						-589	; G_G2_INITGHOUL2MODEL 
equ trap_G2API_AddBolt_1_02								-590	; G_G2_ADDBOLT 
equ trap_G2API_SetBoltInfo_1_02							-591	; G_G2_SETBOLTINFO 
equ trap_G2API_SetBoneAngles_1_02						-592	; G_G2_ANGLEOVERRIDE 
equ trap_G2API_SetBoneAnim_1_02							-593	; G_G2_PLAYANIM 
equ trap_G2API_GetGLAName_1_02							-594	; G_G2_GETGLANAME 
equ trap_G2API_CopyGhoul2Instance_1_02					-595	; G_G2_COPYGHOUL2INSTANCE 
equ trap_G2API_CopySpecificGhoul2Model_1_02				-596	; G_G2_COPYSPECIFICGHOUL2MODEL 
equ trap_G2API_DuplicateGhoul2Instance_1_02				-597	; G_G2_DUPLICATEGHOUL2INSTANCE 
equ trap_G2API_HasGhoul2ModelOnIndex_1_02				-598	; G_G2_HASGHOUL2MODELONINDEX 
equ trap_G2API_RemoveGhoul2Model_1_02					-599	; G_G2_REMOVEGHOUL2MODEL 
equ trap_G2API_CleanGhoul2Models_1_02					-600	; G_G2_CLEANMODELS 

equ trap_G2API_GetBoltMatrix_NoRecNoRot_1_04			-589	; G_G2_GETBOLT_NOREC_NOROT 
equ trap_G2API_InitGhoul2Model_1_04						-590	; G_G2_INITGHOUL2MODEL 
equ trap_G2API_AddBolt_1_04								-591	; G_G2_ADDBOLT 
equ trap_G2API_SetBoltInfo_1_04							-592	; G_G2_SETBOLTINFO 
equ trap_G2API_SetBoneAngles_1_04						-593	; G_G2_ANGLEOVERRIDE 
equ trap_G2API_SetBoneAnim_1_04							-594	; G_G2_PLAYANIM 
equ trap_G2API_GetGLAName_1_04							-595	; G_G2_GETGLANAME 
equ trap_G2API_CopyGhoul2Instance_1_04					-596	; G_G2_COPYGHOUL2INSTANCE 
equ trap_G2API_CopySpecificGhoul2Model_1_04				-597	; G_G2_COPYSPECIFICGHOUL2MODEL 
equ trap_G2API_DuplicateGhoul2Instance_1_04				-598	; G_G2_DUPLICATEGHOUL2INSTANCE 
equ trap_G2API_HasGhoul2ModelOnIndex_1_04				-599	; G_G2_HASGHOUL2MODELONINDEX 
equ trap_G2API_RemoveGhoul2Model_1_04					-600	; G_G2_REMOVEGHOUL2MODEL 
equ trap_G2API_CleanGhoul2Models_1_04					-601	; G_G2_CLEANMODELS 
equ trap_G2API_CollisionDetect_1_04						-602	; G_G2_COLLISIONDETECT 


; hardcoded functions
equ memset												-101	; G_MEMSET 
equ memcpy												-102	; G_MEMCPY 
equ strncpy												-103	; G_STRNCPY 
equ sin													-104	; G_SIN 
equ cos													-105	; G_COS 
equ atan2												-106	; G_ATAN2 
equ sqrt												-107	; G_SQRT 
equ matrixmultiply										-108	; G_MATRIXMULTIPLY 
equ anglevectors										-109	; G_ANGLEVECTORS 
equ perpendicularvector									-110	; G_PERPENDICULARVECTOR 
equ floor												-111	; G_FLOOR 
equ ceil												-112	; G_CEIL 
equ acos												-115	; G_ACOS 
equ asin												-116	; G_ASIN 

; JK2MV Syscalls [Universal]
equ trap_MVAPI_ControlFixes								-704	;MVAPI_CONTROL_FIXES
equ trap_MVAPI_GetVersion								-705    ;MVAPI_GET_VERSION
equ trap_FS_FLock										-709	;MVAPI_FS_FLOCK
equ trap_MVAPI_SetVersion								-710	;MVAPI_SET_VERSION
equ trap_MVAPI_Print									-714	;MVAPI_PRINT

; JK2MV Syscalls [Game]
equ trap_MVAPI_SendConnectionlessPacket					-701	;G_MVAPI_SEND_CONNECTIONLESSPACKET
equ trap_MVAPI_GetConnectionlessPacket					-702	;G_MVAPI_GET_CONNECTIONLESSPACKET
equ trap_MVAPI_LocateGameData							-703	;G_MVAPI_LOCATE_GAME_DATA
equ trap_MVAPI_DisableStructConversion					-706	;G_MVAPI_DISABLE_STRUCT_CONVERSION
equ trap_MVAPI_ResetServerTime							-711	;G_MVAPI_RESET_SERVER_TIME
equ trap_MVAPI_EnablePlayerSnapshots					-712	;G_MVAPI_ENABLE_PLAYERSNAPSHOTS
equ trap_MVAPI_EnableSubmodelBypass				        -713	;G_MVAPI_ENABLE_SUBMODELBYPASS


equ trap_G_COOL_API_SetBrushModelContentFlags	        -1101	;G_COOL_API_SETBRUSHMODELCONTENTFLAGS
equ trap_G_COOL_API_PlayerUserCmdAdd	        		-1102	;G_COOL_API_PLAYERUSERCMD_ADD
equ trap_G_COOL_API_PlayerUserCmdRemove	        		-1103	;G_COOL_API_PLAYERUSERCMD_REMOVE
equ trap_G_COOL_API_PlayerUserCmdClear	        		-1104	;G_COOL_API_PLAYERUSERCMD_CLEAR
equ trap_G_COOL_API_PlayerUserCmdGet	        		-1105	;G_COOL_API_PLAYERUSERCMD_GET
equ trap_G_COOL_API_PlayerUserCmdGetCount        		-1106	;G_COOL_API_PLAYERUSERCMD_GETCOUNT
equ trap_G_COOL_API_NonEpsilonTrace						-1107	;G_COOL_API_NONEPSILONTRACE 
equ trap_G_COOL_API_NonEpsilonTraceCapsule				-1108	;G_COOL_API_NONEPSILONTRACE_CAPSULE 

; COOL API DB Calls
equ trap_G_COOL_API_DB_EscapeString						-1201	;G_COOL_API_DB_ESCAPESTRING
equ trap_G_COOL_API_DB_AddRequest						-1202	;G_COOL_API_DB_ADDREQUEST
equ trap_G_COOL_API_DB_NextResponse						-1203	;G_COOL_API_DB_NEXTRESPONSE
equ trap_G_COOL_API_DB_GetReference						-1204	;G_COOL_API_DB_GETREFERENCE
equ trap_G_COOL_API_DB_NextRow							-1205	;G_COOL_API_DB_NEXTROW
equ trap_G_COOL_API_DB_GetInt							-1206	;G_COOL_API_DB_GETINT
equ trap_G_COOL_API_DB_GetFloat							-1207	;G_COOL_API_DB_GETFLOAT
equ trap_G_COOL_API_DB_GetString						-1208	;G_COOL_API_DB_GETSTRING
equ trap_G_COOL_API_DB_AddRequestTyped					-1209	;G_COOL_API_DB_ADDREQUEST_TYPED

equ trap_G_COOL_API_DB_AddPreparedStatement				-1210	;G_COOL_API_DB_ADDPREPAREDSTATEMENT
equ trap_G_COOL_API_DB_PreparedBindString				-1211	;G_COOL_API_DB_PREPAREDBINDSTRING
equ trap_G_COOL_API_DB_PreparedBindFloat				-1212	;G_COOL_API_DB_PREPAREDBINDFLOAT
equ trap_G_COOL_API_DB_PreparedBindInt					-1213	;G_COOL_API_DB_PREPAREDBINDINT
equ trap_G_COOL_API_DB_PreparedBindBinary				-1214	;G_COOL_API_DB_PREPAREDBINDBINARY
equ trap_G_COOL_API_DB_FinishAndSendPreparedStatement	-1215	;G_COOL_API_DB_FINISHANDSENDPREPAREDSTATEMENT
equ trap_G_COOL_API_DB_GetBinary						-1216	;G_COOL_API_DB_GETBINARY
equ trap_G_COOL_API_DB_PreparedBindNull					-1217	;G_COOL_API_DB_PREPAREDBINDNULL
equ trap_G_COOL_API_DB_GetMoreResults					-1218	;G_COOL_API_DB_GETMORERESULTS
