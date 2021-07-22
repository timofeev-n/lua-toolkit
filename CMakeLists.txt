cmake_minimum_required(VERSION 3.16)

project(libluatoolkit
	LANGUAGES CXX C
	VERSION 0.0.1
)

set (LUA_TOOLKIT_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)


maker_initialize(
	WINDOWS
		UNITY_DEACTIVATE_ORIGIN     ON
)


maker_create_target(lua-toolkit
	COMMON 
		BUILD_TYPE	STATIC_LIB
		UNITY_BUILD OFF
		IDE_FOLDER "dependencies"

		SRCS
			"${LUA_TOOLKIT_SRC_ROOT}"
		
		PRIVATE_INCLUDE_DIRS
			"${LUA_TOOLKIT_SRC_ROOT}"

		INCLUDE_DIRS
			"${LUA_TOOLKIT_SRC_ROOT}/include"

		DEFINITIONS
			LUA_TOOLKIT_BUILD
		
		PRECOMPILED_HEADER "${LUA_TOOLKIT_SRC_ROOT}/pch.h"

		LINK_LIBS
			runtime

	WINDOWS
		PRECOMPILED_SOURCE "${LUA_TOOLKIT_SRC_ROOT}/pch.cpp"
)


set (LUA_TOOLKIT_TESTS_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tests/luatoolkit_unittests)

maker_create_target(lua-toolkit_unittests
	COMMON 
		BUILD_TYPE	TEST
		IDE_FOLDER "Tests"

		SRCS
			"${LUA_TOOLKIT_TESTS_SRC_ROOT}"

		PRIVATE_INCLUDE_DIRS
			"${src}"

		PRECOMPILED_HEADER "${LUA_TOOLKIT_TESTS_SRC_ROOT}/pch.h"

		LINK_TARGETS
			lua-toolkit
			gtest
			gmock
			gtest_main

	WINDOWS
		PRECOMPILED_SOURCE "${LUA_TOOLKIT_TESTS_SRC_ROOT}/pch.cpp"

		OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/win"
		OUTPUT_NAME_RELEASE "lua-toolkit_unittests"
		OUTPUT_NAME_DEBUG  "lua-toolkit_unittests_debug"
)
