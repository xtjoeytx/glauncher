# The hook library
add_library(dll SHARED dllmain.cpp)
target_link_libraries(dll PRIVATE lib_detours ws2_32)
set_target_properties(dll PROPERTIES LINK_FLAGS /EXPORT:DetourFinishHelperProcess,@1,NONAME)
set_target_properties(dll PROPERTIES OUTPUT_NAME "GLauncher")
