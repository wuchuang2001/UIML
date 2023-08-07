
# 递归获取所有源码文件
file(GLOB_RECURSE TOOLS_SRC *.c)

# 传递源码文件到上一级 CMakeLists 作用域
set(TOOLS_MODULE_SRC ${TOOLS_SRC} PARENT_SCOPE)

# 添加 include 目录
set(TOOLS_MODULE_INCL ${CMAKE_CURRENT_LIST_DIR}/include PARENT_SCOPE)
