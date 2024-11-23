target_sources(
    Pong PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PongAiComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PongAiComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PongPhysicsComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PongPhysicsComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockingBoardEntity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockingBoardEntity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MainGameHUDEntity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainGameHUDEntity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PongGame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PongGame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UserControlComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UserControlComponent.h
)