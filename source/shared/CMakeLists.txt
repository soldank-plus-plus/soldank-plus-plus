set(shared_lib_modules
    communication/DeliveryMode.cpp
    communication/NetworkEvent.cpp
    communication/NetworkEventDispatcher.cpp
    communication/NetworkMessage.cpp
    communication/NetworkPackets.cpp
    communication/PingTimer.cpp

    core/IWorld.cpp
    core/World.cpp
    core/WorldEvents.cpp
    core/CoreEventHandler.cpp
    core/loop/FixedTimestepRunner.cpp
    core/loop/NativeFixedTimestepLoop.cpp
    core/loop/PlatformFixedTimestepLoop.cpp

    core/animations/AnimationData.cpp
    core/animations/AnimationState.cpp
    core/animations/Animations.cpp
    core/animations/AnimationsStates.cpp
    core/animations/states/BodyAimAnimationState.cpp
    core/animations/states/BodyChangeAnimationState.cpp
    core/animations/states/BodyGetUpAnimationState.cpp
    core/animations/states/BodyProneAnimationState.cpp
    core/animations/states/BodyProneMoveAnimationState.cpp
    core/animations/states/BodyPunchAnimationState.cpp
    core/animations/states/BodyRollAnimationState.cpp
    core/animations/states/BodyRollBackAnimationState.cpp
    core/animations/states/BodyStandAnimationState.cpp
    core/animations/states/BodyThrowAnimationState.cpp
    core/animations/states/BodyThrowWeaponAnimationState.cpp
    core/animations/states/CommonAnimationStateTransitions.cpp
    core/animations/states/LegsCrouchAnimationState.cpp
    core/animations/states/LegsCrouchRunAnimationState.cpp
    core/animations/states/LegsCrouchRunBackAnimationState.cpp
    core/animations/states/LegsFallAnimationState.cpp
    core/animations/states/LegsGetUpAnimationState.cpp
    core/animations/states/LegsJumpAnimationState.cpp
    core/animations/states/LegsJumpSideAnimationState.cpp
    core/animations/states/LegsProneAnimationState.cpp
    core/animations/states/LegsProneMoveAnimationState.cpp
    core/animations/states/LegsRollAnimationState.cpp
    core/animations/states/LegsRollBackAnimationState.cpp
    core/animations/states/LegsRunAnimationState.cpp
    core/animations/states/LegsRunBackAnimationState.cpp
    core/animations/states/LegsStandAnimationState.cpp

    core/config/Config.cpp

    core/data/FileReader.cpp
    core/data/IFileReader.cpp
    core/data/FileWriter.cpp
    core/data/IFileWriter.cpp

    core/entities/Bullet.cpp
    core/entities/Item.cpp
    core/entities/Soldier.cpp
    core/entities/Weapon.cpp
    core/entities/WeaponParameters.cpp
    core/entities/WeaponParametersFactory.cpp

    core/map/Map.cpp
    core/map/PMSEnums.cpp
    core/map/PMSStructs.cpp
    core/map/PMSConstants.cpp

    core/math/Glm.cpp
    core/math/Calc.cpp

    core/physics/BulletPhysics.cpp
    core/physics/Constants.cpp
    core/physics/ItemPhysics.cpp
    core/physics/SoldierPhysics.cpp
    core/physics/SoldierSkeletonPhysics.cpp
    core/physics/Particles.cpp
    core/physics/PhysicsEvents.cpp

    core/state/Control.cpp
    core/state/State.cpp
    core/state/StateManager.cpp

    core/types/BulletType.cpp
    core/types/ItemType.cpp
    core/types/TeamType.cpp
    core/types/WeaponGroupType.cpp
    core/types/WeaponType.cpp

    core/utility/Getline.cpp
    core/utility/Observable.cpp
    core/utility/VisitHelper.cpp
)

add_library(shared_lib)
target_sources(shared_lib
    PUBLIC
        FILE_SET all_shared_lib_modules TYPE CXX_MODULES FILES 
        ${shared_lib_modules}
)

target_compile_definitions(shared_lib PUBLIC ${CLIENT_COMPILE_DEFINITIONS} -DNOMINMAX)

if(MSVC)
  target_compile_options(shared_lib PRIVATE /W4)
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
target_link_libraries(shared_lib PUBLIC Threads::Threads)

soldank_target_precompile_headers(shared_lib)
