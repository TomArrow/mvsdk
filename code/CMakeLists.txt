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

# Ensure qvms target is built with main build
add_custom_target(all_qvms DEPENDS qvms)


	file(RELATIVE_PATH MVSDK_CGAME_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/cgame")
	file(TO_NATIVE_PATH "${MVSDK_CGAME_PATH}" MVSDK_CGAME_PATH)
	
	file(RELATIVE_PATH MVSDK_GAME_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/game")
	file(TO_NATIVE_PATH "${MVSDK_GAME_PATH}" MVSDK_GAME_PATH)
	
	file(RELATIVE_PATH MVSDK_UI_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${SRCDir}/ui")
	file(TO_NATIVE_PATH "${MVSDK_UI_PATH}" MVSDK_UI_PATH)
	
	file(RELATIVE_PATH MVSDK_BIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/vm_build" "${CMAKE_BINARY_DIR}")
	file(TO_NATIVE_PATH "${MVSDK_BIN_PATH}" MVSDK_BIN_PATH)
	
add_custom_target(qvms
	# Create required directories first
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/vm_build"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/vm_temp"
	
	COMMAND $<TARGET_FILE:${LCC}>
		"-DQ3_VM" "-DJK2_GAME" "-DMISSIONPACK" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVGAMEVMSOURCES_NATIVE}


	COMMAND $<TARGET_FILE:${ASM}>
		"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/jk2mpgame.qvm" ${MVGAMEASMFILES} ${MVGAMEASM}

	COMMAND $<TARGET_FILE:${LCC}>
		"-DQ3_VM" "-DJK2_CGAME" "-DMISSIONPACK" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${MVSDK_CGAME_PATH}" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVCGAMEVMSOURCES_NATIVE}

	COMMAND $<TARGET_FILE:${ASM}>
		"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/cgame.qvm" ${MVCGAMEASMFILES} ${MVCGAMEASM}

	COMMAND $<TARGET_FILE:${LCC}>
		"-DQ3_VM" "-DJK2_UI" "-S" "-Wf-target=bytecode" "-Wf-g" "-I${SRCDir}/cgame" "-I${MVSDK_GAME_PATH}" "-I${MVSDK_UI_PATH}" "-I${MVSDK_BIN_PATH}" ${MVUIVMSOURCES_NATIVE}

	COMMAND $<TARGET_FILE:${ASM}>
		"-vq3" "-m" "-v" "-o" "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/ui.qvm" ${MVUIASMFILES} ${MVUIASM}

	WORKING_DIRECTORY
		"${CMAKE_CURRENT_BINARY_DIR}/vm_build"

	DEPENDS
		${LCC} ${CPP} ${RCC} ${ASM}
)

# Create jk2pro-bins.pk3 with QVM files and mv.info
set(AssetsFiles
	"vm/jk2mpgame.qvm"
	"vm/jk2mpgame.map"
	"vm/cgame.qvm"
	"vm/cgame.map"
	"vm/ui.qvm"
	"vm/ui.map"
	"mv.info"
)

set(AssetsSources
	"$<IF:$<CONFIG:Release>,${CMAKE_BINARY_DIR}/out/Release/base/jk2mpgame.qvm,${CMAKE_BINARY_DIR}/out/$<CONFIG>/base/jk2mpgame.qvm>"
	"$<IF:$<CONFIG:Release>,${CMAKE_BINARY_DIR}/out/Release/base/jk2mpgame.map,${CMAKE_BINARY_DIR}/out/$<CONFIG>/base/jk2mpgame.map>"
	"$<IF:$<CONFIG:Release>,${CMAKE_BINARY_DIR}/out/Release/base/cgame.qvm,${CMAKE_BINARY_DIR}/out/$<CONFIG>/base/cgame.qvm>"
	"$<IF:$<CONFIG:Release>,${CMAKE_BINARY_DIR}/out/Release/base/cgame.map,${CMAKE_BINARY_DIR}/out/$<CONFIG>/base/cgame.map>"
	"${CMAKE_CURRENT_BINARY_DIR}/vm/ui.qvm"
	"${CMAKE_CURRENT_BINARY_DIR}/vm/ui.map"
	"${SRCDir}/../mv.info"
)

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/out/$<CONFIG>/eternaljk2/jk2pro-bins.pk3"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/out/$<CONFIG>/eternaljk2"
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm"
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/jk2mpgame.qvm" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm/"
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/jk2mpgame.map" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm/"
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/cgame.qvm" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm/"
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/cgame.map" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm/"
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/ui.qvm" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm/"
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/vm_temp/ui.map" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/vm/"
	COMMAND "${CMAKE_COMMAND}" -E copy "${SRCDir}/../mv.info" "${CMAKE_CURRENT_BINARY_DIR}/pk3_build/"
	COMMAND "${CMAKE_COMMAND}" -E tar "cf" "${CMAKE_BINARY_DIR}/out/$<CONFIG>/eternaljk2/jk2pro-bins.pk3" --format=zip
		"vm/jk2mpgame.qvm" "vm/jk2mpgame.map" "vm/cgame.qvm" "vm/cgame.map" "vm/ui.qvm" "vm/ui.map" "mv.info"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pk3_build"
	DEPENDS qvms
	COMMENT "Building jk2pro-bins.pk3 in eternaljk2 directory"
)

add_custom_target(jk2pro-bins ALL DEPENDS "${CMAKE_BINARY_DIR}/out/$<CONFIG>/eternaljk2/jk2pro-bins.pk3")

