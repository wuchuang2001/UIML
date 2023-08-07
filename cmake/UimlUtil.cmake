
function(uiml_link_to_stm32_target TARGET_NAME CPU_TYPE)
    get_target_property(TARGET_INCL ${TARGET_NAME} INCLUDE_DIRECTORIES)
    target_include_directories(uiml_static PRIVATE ${TARGET_INCL})
    target_compile_options(uiml_static PUBLIC -mcpu=${CPU_TYPE})

    target_link_libraries(${TARGET_NAME} PRIVATE uiml_static)
endfunction()
