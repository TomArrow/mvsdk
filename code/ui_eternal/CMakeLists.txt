# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

set(ETERNAL_UI_HEADERS
	"${SRCDir}/ui_eternal/cgame/animtable.h"
	"${SRCDir}/ui_eternal/game/anims.h"
	"${SRCDir}/ui_eternal/game/bg_local.h"
	"${SRCDir}/ui_eternal/game/bg_public.h"
	"${SRCDir}/ui_eternal/game/bg_saga.h"
	"${SRCDir}/ui_eternal/game/bg_vehicles.h"
	"${SRCDir}/ui_eternal/game/bg_weapons.h"
	"${SRCDir}/ui_eternal/game/surfaceflags.h"
	"${SRCDir}/ui_eternal/game/teams.h"
	"${SRCDir}/ui_eternal/game/w_saber.h"
	"${SRCDir}/ui_eternal/ghoul2/G2.h"
	"${SRCDir}/ui_eternal/qcommon/disablewarnings.h"
	"${SRCDir}/ui_eternal/qcommon/game_version.h"
	"${SRCDir}/ui_eternal/qcommon/qfiles.h"
	"${SRCDir}/ui_eternal/qcommon/q_color.h"
	"${SRCDir}/ui_eternal/qcommon/q_math.h"
	"${SRCDir}/ui_eternal/qcommon/q_platform.h"
	"${SRCDir}/ui_eternal/qcommon/q_shared.h"
	"${SRCDir}/ui_eternal/qcommon/q_string.h"
	"${SRCDir}/ui_eternal/qcommon/tags.h"
	"${SRCDir}/ui_eternal/qcommon/safe/gsl.h"
	"${SRCDir}/ui_eternal/qcommon/safe/sscanf.h"
	"${SRCDir}/ui_eternal/qcommon/safe/string.h"
	"${SRCDir}/ui_eternal/qcommon/safe/gsl/gsl-lite.h"
	"${SRCDir}/ui_eternal/rd-common/mdx_format.h"
	"${SRCDir}/ui_eternal/rd-common/tr_types.h"
	"${SRCDir}/ui_eternal/ui/keycodes.h"
	"${SRCDir}/ui_eternal/ui/menudef.h"
	"${SRCDir}/ui_eternal/ui/ui_force.h"
	"${SRCDir}/ui_eternal/ui/ui_local.h"
	"${SRCDir}/ui_eternal/ui/ui_public.h"
	"${SRCDir}/ui_eternal/ui/ui_shared.h"
	"${SRCDir}/ui_eternal/ui/ui_xcvar.h"
	"${SRCDir}/ui_eternal/ui/ui_xdocs.h"
)

set(ETERNAL_UI_SOURCES
	"${SRCDir}/ui_eternal/ui/ui_main.c" # keep this on top (qvm compiler needs VM_Main as the first compiled function)
	"${SRCDir}/ui_eternal/game/bg_misc.c"
	"${SRCDir}/ui_eternal/game/bg_saberLoad.c"
	"${SRCDir}/ui_eternal/game/bg_saga.c"
	"${SRCDir}/ui_eternal/game/bg_vehicleLoad.c"
	"${SRCDir}/ui_eternal/game/bg_weapons.c"
	"${SRCDir}/ui_eternal/qcommon/q_color.c"
	"${SRCDir}/ui_eternal/qcommon/q_math.c"
	"${SRCDir}/ui_eternal/qcommon/q_shared.c"
	"${SRCDir}/ui_eternal/qcommon/q_string.c"
	"${SRCDir}/ui_eternal/ui/ui_atoms.c"
	"${SRCDir}/ui_eternal/ui/ui_cvar.c"
	"${SRCDir}/ui_eternal/ui/ui_force.c"
	"${SRCDir}/ui_eternal/ui/ui_gameinfo.c"
	"${SRCDir}/ui_eternal/ui/ui_saber.c"
	"${SRCDir}/ui_eternal/ui/ui_shared.c"
)

set(ETERNAL_UI_FILES
	${ETERNAL_UI_HEADERS}
	${ETERNAL_UI_SOURCES}
	"${SRCDir}/ui_eternal/ui/ui_syscalls.c"
)

add_library(${ETERNAL_UI} SHARED ${ETERNAL_UI_FILES})

# Defines
set(ETERNAL_UI_Defines ${ETERNAL_UI_Defines} "JK2_UI")
set(ETERNAL_UI_IncludeDirs
	"${SRCDir}/ui_eternal"
	${MVGLOBALIncludeDirs}
)

set_target_properties(${ETERNAL_UI} PROPERTIES PREFIX "") # remove "lib" prefix for .so/.dylib files

set_property(TARGET ${ETERNAL_UI} APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:${GlobalDefines} ${ETERNAL_UI_Defines} ${DebugDefines}>
  $<$<CONFIG:Release>:${GlobalDefines} ${ETERNAL_UI_Defines} ${ReleaseDefines}>
  $<$<CONFIG:RelWithDebInfo>:${GlobalDefines} ${ETERNAL_UI_Defines} ${ReleaseDefines}>
)

set_target_properties(${ETERNAL_UI} PROPERTIES INCLUDE_DIRECTORIES "${ETERNAL_UI_IncludeDirs}")
set_target_properties(${ETERNAL_UI} PROPERTIES PROJECT_LABEL "Eternal UI Module")
if(InMV)
	set_property(TARGET ${ETERNAL_UI} PROPERTY FOLDER "Modules")
endif()

if(InMV)
	if(BuildPortableVersion)
		if(WIN32)
			install(TARGETS ${ETERNAL_UI} RUNTIME DESTINATION "eternaljk2")
			install(FILES $<TARGET_PDB_FILE:${ETERNAL_UI}> DESTINATION "eternaljk2" OPTIONAL)
		else()
			install(TARGETS ${ETERNAL_UI} LIBRARY DESTINATION "eternaljk2")
		endif()
	else()
		if(WIN32)
			install(TARGETS ${ETERNAL_UI} RUNTIME DESTINATION "eternaljk2")
			install(FILES $<TARGET_PDB_FILE:${ETERNAL_UI}> DESTINATION "eternaljk2" OPTIONAL)
		elseif(APPLE)
			install(TARGETS ${ETERNAL_UI} LIBRARY DESTINATION "${MVMP}.app/Contents/MacOS/eternaljk2")
		else()
			# TODO
		endif()
	endif()
endif()
