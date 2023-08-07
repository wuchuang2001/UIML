
# 子模块添加
add_subdirectory(tools)
add_subdirectory(softbus)
add_subdirectory(conf)

# UIML 静态库部分
add_library(uiml_static OBJECT
    ${TOOLS_MODULE_SRC}
    ${SOFTBUS_MODULE_SRC}
    ${CONFIG_MODULE_SRC}
)

target_include_directories(uiml_static PUBLIC
    ${TOOLS_MODULE_INCL}
    ${SOFTBUS_MODULE_INCL}
    ${CONFIG_MODULE_INCL}
)
