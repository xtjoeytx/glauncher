# Generate version header from the above
configure_file(
	IConfig.h.in
	${PROJECT_BINARY_DIR}/IConfig.h
)
include_directories(${PROJECT_BINARY_DIR})

add_executable(launcher WIN32 main.cpp IConfig.h.in ${PROJECT_BINARY_DIR}/IConfig.h)
set_target_properties(launcher PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

target_link_options(launcher PRIVATE "/entry:mainCRTStartup")
target_link_libraries(launcher lib_detours)
