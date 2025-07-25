# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

set(MVUIHEADERS
	"${SRCDir}/api/mvapi.h"
	"${SRCDir}/api/mvmenu.h"

	"${SRCDir}/ui/keycodes.h"
	"${SRCDir}/ui/ui_force.h"
	"${SRCDir}/ui/ui_local.h"
	"${SRCDir}/ui/ui_public.h"
	"${SRCDir}/ui/ui_shared.h"
	"${SRCDir}/ui/ui_skin.h"
	"${SRCDir}/ui/ui_multiversion.h"
	"${SRCDir}/ui/ui_multiversion_syscalls.h"

	"${SRCDir}/game/bg_weapons.h"
	"${SRCDir}/game/q_shared.h"
	"${SRCDir}/game/bg_multiversion.h"
)

set(MVUISOURCES
	"${SRCDir}/ui/ui_main.c" # keep this on top (qvm compiler needs VM_Main as the first compiled function)
	"${SRCDir}/ui/ui_atoms.c"
	"${SRCDir}/ui/ui_force.c"
	"${SRCDir}/ui/ui_gameinfo.c"
	"${SRCDir}/ui/ui_saber.c"
	"${SRCDir}/ui/ui_shared.c"
	"${SRCDir}/ui/ui_skin.c"
	"${SRCDir}/ui/ui_multiversion.c"
	"${SRCDir}/ui/ui_saber.c"

	"${SRCDir}/game/bg_misc.c"
	"${SRCDir}/game/bg_weapons.c"
	"${SRCDir}/game/q_math.c"
	"${SRCDir}/game/q_shared.c"
	"${SRCDir}/ui/ui_multiversion_syscalls.c"
	"${SRCDir}/game/bg_multiversion.c"
)

set(MVUIASM
	"${SRCDir}/ui/ui_syscalls.asm"
	PARENT_SCOPE
)

set(MVUIFILES
	${MVUIHEADERS}
	${MVUISOURCES}
	"${SRCDir}/ui/ui_syscalls.c"
)

set(MVUIVMSOURCES ${MVUISOURCES}
	"${SRCDir}/game/bg_lib.c"
	PARENT_SCOPE
)

# Explicitly set the target name to ui_x64 for correct DLL naming
set(MVUI ui_x64)

add_library(${MVUI} SHARED ${MVUIFILES})

message(STATUS "[CMAKE] ui target: ${MVUI}, output: ${CMAKE_BINARY_DIR}/out/$<CONFIG>/")

# Ensure all module DLLs are output to the main directory alongside the executable
set_target_properties(${MVUI} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/$<CONFIG>"
)

# Defines
set(MVUIDefines ${MVUIDefines} "JK2_UI")
set(MVUIIncludeDirs ${MVGLOBALIncludeDirs})

set_target_properties(${MVUI} PROPERTIES PREFIX "") # remove "lib" prefix for .so/.dylib files

set_property(TARGET ${MVUI} APPEND PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:${GlobalDefines} ${MVUIDefines} ${DebugDefines}>
  $<$<CONFIG:Release>:${GlobalDefines} ${MVUIDefines} ${ReleaseDefines}>
  $<$<CONFIG:RelWithDebInfo>:${GlobalDefines} ${MVUIDefines} ${ReleaseDefines}>
)

set_target_properties(${MVUI} PROPERTIES INCLUDE_DIRECTORIES "${MVUIIncludeDirs}")
set_target_properties(${MVUI} PROPERTIES PROJECT_LABEL "UI Module")
if(InMV)
	set_property(TARGET ${MVUI} PROPERTY FOLDER "Modules")
	if(WIN32)
		# Build directly to main directory - no copy needed
	elseif(APPLE)
		install(TARGETS ${MVUI} LIBRARY DESTINATION "${MVMP}.app/Contents/MacOS")
	else()
		install(TARGETS ${MVUI} LIBRARY DESTINATION "lib")
	endif()
endif()

if(InMV)
	if(BuildPortableVersion)
		if(WIN32)
			install(TARGETS ${MVUI} RUNTIME DESTINATION "base")
			install(FILES $<TARGET_PDB_FILE:${MVUI}> DESTINATION "." OPTIONAL)
		else()
			install(TARGETS ${MVUI} LIBRARY DESTINATION "base")
		endif()
	else()
		if(WIN32)
			install(TARGETS ${MVUI} RUNTIME DESTINATION "base")
			install(FILES $<TARGET_PDB_FILE:${MVUI}> DESTINATION "." OPTIONAL)
		elseif(APPLE)
			install(TARGETS ${MVUI} LIBRARY DESTINATION "${MVMP}.app/Contents/MacOS/base")
		else()
			# TODO
		endif()
	endif()
endif()
