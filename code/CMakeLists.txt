# Make sure the user is not executing this script directly
if(NOT InMVSDK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMVSDK)

# mv_setup.h preparation
set(MVGLOBALIncludeDirs ${MVGLOBALIncludeDirs} ${CMAKE_BINARY_DIR})
configure_file("mvsdk_setup.h.in" "${CMAKE_BINARY_DIR}/mvsdk_setup.h" @ONLY)

add_subdirectory("game")
add_subdirectory("cgame")
add_subdirectory("ui")


	file(RELATIVE_PATH MVSDK_CGAME_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/cgame")
	file(TO_NATIVE_PATH "${MVSDK_CGAME_PATH}" MVSDK_CGAME_PATH)
	
	file(RELATIVE_PATH MVSDK_GAME_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/game")
	file(TO_NATIVE_PATH "${MVSDK_GAME_PATH}" MVSDK_GAME_PATH)
	
	file(RELATIVE_PATH MVSDK_UI_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/ui")
	file(TO_NATIVE_PATH "${MVSDK_UI_PATH}" MVSDK_UI_PATH)
	
	file(RELATIVE_PATH MVSDK_BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${CMAKE_BINARY_DIR}")
	file(TO_NATIVE_PATH "${MVSDK_BIN_PATH}" MVSDK_BIN_PATH)
	
add_custom_target(qvms
	COMMAND $<TARGET_FILE:${LCC}>
		"-DQ3_VM" "-DJK2_GAME" "-DMISSIONPACK" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVGAMEVMSOURCES_NATIVE}

	COMMAND $<TARGET_FILE:${ASM}>
		"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm/jk2mpgame.qvm" ${MVGAMEASMFILES} ${MVGAMEASM}

	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_BINARY_DIR}/vm/jk2mpgame.qvm"
		"${CMAKE_SOURCE_DIR}/base/jk2mpgame.qvm"

	COMMAND $<TARGET_FILE:${LCC}>
		"-DQ3_VM" "-DJK2_CGAME" "-DMISSIONPACK" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${MVSDK_CGAME_PATH}" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVCGAMEVMSOURCES_NATIVE}

	COMMAND $<TARGET_FILE:${ASM}>
		"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm/cgame.qvm" ${MVCGAMEASMFILES} ${MVCGAMEASM}

	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_BINARY_DIR}/vm/cgame.qvm"
		"${CMAKE_SOURCE_DIR}/base/cgame.qvm"

	COMMAND $<TARGET_FILE:${LCC}>
		"-DQ3_VM" "-DJK2_UI" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${SRCDir}/cgame" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVUIVMSOURCES_NATIVE}

	COMMAND $<TARGET_FILE:${ASM}>
		"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm/ui.qvm" ${MVUIASMFILES} ${MVUIASM}

	WORKING_DIRECTORY
		"${CMAKE_CURRENT_BINARY_DIR}/vm_build"

	DEPENDS
		${LCC} ${CPP} ${RCC} ${ASM}
)


