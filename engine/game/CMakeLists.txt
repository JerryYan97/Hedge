if(${USE_GAME_TEMPLATE})
    target_sources(
        HedgeEngine PRIVATE
        GameTemplate.cpp
        GameTemplate.h
    )
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ui)