macro(declare_dependencies)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)

    set(project_build_testing ${BUILD_TESTING})
    set(BUILD_TESTING OFF)
    set(STDEXEC_BUILD_EXAMPLES OFF)
    FetchContent_Declare(
        stdexec
        GIT_REPOSITORY https://github.com/nvidia/stdexec.git
        GIT_TAG        c4b905342c8335dca5f49617997b0997908a8111
        FIND_PACKAGE_ARGS
    )
    FetchContent_MakeAvailable(stdexec)

    set(BUILD_TESTING ${project_build_testing})
    if(BUILD_TESTING)
        set(CATCH_INSTALL_DOCS OFF)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v3.7.1
            FIND_PACKAGE_ARGS
        )
        FetchContent_MakeAvailable(Catch2 )
    endif()


endmacro()
