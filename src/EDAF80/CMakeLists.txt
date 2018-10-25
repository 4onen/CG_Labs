cmake_minimum_required (VERSION 3.0)

set (
	COMMON_SOURCES

	"interpolation_modified.cpp"
	"interpolation_modified.hpp"
	"parametric_shapes_modified.cpp"
	"parametric_shapes_modified.hpp"
)

set (
	ASSIGNMENT1_SOURCES

	"assignment1_modified.cpp"
)

set (
	ASSIGNMENT2_SOURCES

	"assignment2_modified.cpp"
	"assignment2_modified.hpp"
)

source_group (
	EDAF80${PATH_SEP}Assignment2

	FILES
	${PROJECT_SOURCE_DIR}/interpolation_modified.cpp
	${PROJECT_SOURCE_DIR}/interpolation_modified.hpp
	${PROJECT_SOURCE_DIR}/assignment2_modified.cpp
	${PROJECT_SOURCE_DIR}/assignment2_modified.hpp
	${PROJECT_SOURCE_DIR}/parametric_shapes_modified.cpp
	${PROJECT_SOURCE_DIR}/parametric_shapes_modified.hpp
	${SHADERS_DIR}/EDAF80/LambertTexture.vert
	${SHADERS_DIR}/EDAF80/LambertTexture.frag
)

set (
	ASSIGNMENT3_SOURCES

	"assignment3_modified.cpp"
	"assignment3_modified.hpp"
)

source_group (
	EDAF80${PATH_SEP}Assignment3

	FILES
	${PROJECT_SOURCE_DIR}/assignment3_modified.cpp
	${PROJECT_SOURCE_DIR}/assignment3_modified.hpp
	${SHADERS_DIR}/EDAF80/LambertTexture.vert
	${SHADERS_DIR}/EDAF80/LambertTexture.frag
)

set (
	ASSIGNMENT4_SOURCES

	"assignment4_modified.cpp"
	"assignment4_modified.hpp"
)

source_group (
	EDAF80${PATH_SEP}Assignment4

	FILES
	${PROJECT_SOURCE_DIR}/assignment4_modified.cpp
	${PROJECT_SOURCE_DIR}/assignment4_modified.hpp
)

set (
	ASSIGNMENT5_SOURCES

	"assignment5/a5_shapes.cpp"
	"assignment5/a5_shapes.hpp"
	"assignment5/texhelper.cpp"
	"assignment5/texhelper.hpp"
	"assignment5/boat.cpp"
	"assignment5/boat.hpp"
	"assignment5/instancedNode.cpp"
	"assignment5/instancedNode.hpp"
	"assignment5/screen_effect.cpp"
	"assignment5/screen_effect.hpp"
	"assignment5/main.cpp"
	"assignment5/main.hpp"
)

source_group (
	EDAF80${PATH_SEP}Assignment5

	FILES
	${PROJECT_SOURCE_DIR}/assignment5_modified.cpp
	${PROJECT_SOURCE_DIR}/assignment5_modified.hpp
)

luggcgl_new_assignment ("EDAF80_Assignment1" "${ASSIGNMENT1_SOURCES}" "${COMMON_SOURCES}")
luggcgl_new_assignment ("EDAF80_Assignment2" "${ASSIGNMENT2_SOURCES}" "${COMMON_SOURCES}")
luggcgl_new_assignment ("EDAF80_Assignment3" "${ASSIGNMENT3_SOURCES}" "${COMMON_SOURCES}")
luggcgl_new_assignment ("EDAF80_Assignment4" "${ASSIGNMENT4_SOURCES}" "${COMMON_SOURCES}")
luggcgl_new_assignment ("EDAF80_Assignment5" "${ASSIGNMENT5_SOURCES}" "${COMMON_SOURCES}")

copy_dlls (EDAF80_Assignment1 "${CMAKE_BINARY_DIR}/src/EDAF80")
