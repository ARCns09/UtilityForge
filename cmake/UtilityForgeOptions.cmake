option(UTILITYFORGE_WARNINGS_AS_ERRORS "Treat UtilityForge warnings as errors" OFF)

function(utilityforge_enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            "${target}"
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
                -Wsign-conversion
        )
        if(UTILITYFORGE_WARNINGS_AS_ERRORS)
            target_compile_options("${target}" PRIVATE -Werror)
        endif()
    elseif(MSVC)
        target_compile_options("${target}" PRIVATE /W4 /permissive-)
        if(UTILITYFORGE_WARNINGS_AS_ERRORS)
            target_compile_options("${target}" PRIVATE /WX)
        endif()
    endif()
endfunction()
