set(shared_lib_sources
    core/state/State.cpp
    core/state/StateManager.cpp
    core/entities/Weapon.cpp
    core/entities/WeaponParametersFactory.cpp
    core/entities/Bullet.cpp
    core/entities/Soldier.cpp
    core/data/FileReader.cpp
    core/data/FileWriter.cpp
    core/map/Map.cpp
    core/types/ItemType.cpp
    core/physics/SoldierSkeletonPhysics.cpp
    core/physics/SoldierPhysics.cpp
    core/physics/ItemPhysics.cpp
    core/physics/Particles.cpp
    core/physics/BulletPhysics.cpp
    core/utility/Getline.cpp
    core/math/Calc.cpp
    core/animations/states/LegsRunBackAnimationState.cpp
    core/animations/states/BodyThrowWeaponAnimationState.cpp
    core/animations/states/BodyAimAnimationState.cpp
    core/animations/states/CommonAnimationStateTransitions.cpp
    core/animations/states/LegsStandAnimationState.cpp
    core/animations/states/LegsRunAnimationState.cpp
    core/animations/states/BodyPunchAnimationState.cpp
    core/animations/states/BodyRollAnimationState.cpp
    core/animations/states/LegsCrouchRunBackAnimationState.cpp
    core/animations/states/LegsJumpSideAnimationState.cpp
    core/animations/states/BodyProneMoveAnimationState.cpp
    core/animations/states/LegsProneMoveAnimationState.cpp
    core/animations/states/LegsCrouchAnimationState.cpp
    core/animations/states/LegsCrouchRunAnimationState.cpp
    core/animations/states/BodyProneAnimationState.cpp
    core/animations/states/LegsFallAnimationState.cpp
    core/animations/states/LegsGetUpAnimationState.cpp
    core/animations/states/BodyRollBackAnimationState.cpp
    core/animations/states/BodyGetUpAnimationState.cpp
    core/animations/states/LegsProneAnimationState.cpp
    core/animations/states/LegsRollAnimationState.cpp
    core/animations/states/BodyStandAnimationState.cpp
    core/animations/states/LegsRollBackAnimationState.cpp
    core/animations/states/BodyChangeAnimationState.cpp
    core/animations/states/BodyThrowAnimationState.cpp
    core/animations/states/LegsJumpAnimationState.cpp
    core/animations/AnimationData.cpp
    core/animations/AnimationState.cpp
)

set(shared_lib_modules
    communication/NetworkEvent.cpp
    communication/NetworkEventDispatcher.cpp
    communication/NetworkMessage.cpp
    communication/NetworkPackets.cpp
    communication/PingTimer.cpp

    core/IWorld.cpp
    core/World.cpp
    core/CoreEventHandler.cpp
)

add_library(shared_lib)
target_sources(shared_lib
    PUBLIC
    ${shared_lib_sources}
)
target_sources(shared_lib
    PUBLIC
        FILE_SET all_shared_lib_modules TYPE CXX_MODULES FILES 
        ${shared_lib_modules}
)

target_compile_definitions(shared_lib PUBLIC ${CLIENT_COMPILE_DEFINITIONS} -DNOMINMAX)

if(MSVC)
  set(BUILD_COMPILE_WITH_COVERAGE ON)
  target_compile_options(shared_lib PRIVATE /Wall)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(shared_lib PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(shared_lib PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(shared_lib PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

target_include_directories(shared_lib PUBLIC ${SIMPLEINI_INCLUDE_DIRS})
target_include_directories(shared_lib PUBLIC ${CMAKE_SOURCE_DIR}/source/shared)

target_link_libraries(shared_lib PUBLIC glm::glm)
target_link_libraries(shared_lib PRIVATE spdlog::spdlog)
