set(soldank_core_modules
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
    core/map/MapDocument.cpp
    core/map/PMSEnums.cpp
    core/map/PMSStructs.cpp
    core/map/PMSConstants.cpp
    core/map/RuntimeMap.cpp

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

    core/simulation/SimulationCommands.cpp
    core/simulation/SimulationCommandQueue.cpp
    core/simulation/SimulationEvents.cpp
    core/simulation/SimulationEventSink.cpp
    core/simulation/PlayerInputApplication.cpp
    core/simulation/WorldTick.cpp

    core/types/BulletType.cpp
    core/types/ItemType.cpp
    core/types/TeamType.cpp
    core/types/WeaponGroupType.cpp
    core/types/WeaponType.cpp

    core/utility/Getline.cpp
    core/utility/Observable.cpp
    core/utility/SerialNumber.cpp
    core/utility/VisitHelper.cpp
)

set(soldank_protocol_modules
    communication/DeliveryMode.cpp
    communication/NetworkEvent.cpp
    communication/NetworkEventDispatcher.cpp
    communication/NetworkMessage.cpp
    communication/NetworkPackets.cpp
    communication/PingTimer.cpp
    communication/ProtocolConversions.cpp
)

function(configure_shared_module_library target_name)
    target_compile_definitions(${target_name} PUBLIC ${CLIENT_COMPILE_DEFINITIONS} -DNOMINMAX)

    if(MSVC)
      target_compile_options(${target_name} PRIVATE /W4)
    else()
      if (BUILD_COMPILE_WITH_COVERAGE)
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -coverage
                -fprofile-arcs
                -ftest-coverage
        )
        target_link_options(${target_name} PRIVATE -coverage -fprofile-arcs)
      else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
      endif()
    endif()

    target_include_directories(${target_name} PUBLIC ${CMAKE_SOURCE_DIR}/source/shared)
    target_link_libraries(${target_name} PUBLIC Threads::Threads)

    soldank_target_precompile_headers(${target_name})
endfunction()

add_library(soldank_core)
target_sources(soldank_core
    PUBLIC
        FILE_SET all_soldank_core_modules TYPE CXX_MODULES FILES
        ${soldank_core_modules}
    PRIVATE
        core/map/MapIO.cpp
        core/map/MapEditing.cpp
        core/map/MapGeometry.cpp
        core/map/MapSectors.cpp
)

configure_shared_module_library(soldank_core)
target_include_directories(soldank_core PUBLIC ${SIMPLEINI_INCLUDE_DIRS})
target_link_libraries(soldank_core PUBLIC Glm)
target_link_libraries(soldank_core PRIVATE spdlog::spdlog)

add_library(soldank_protocol)
target_sources(soldank_protocol
    PUBLIC
        FILE_SET all_soldank_protocol_modules TYPE CXX_MODULES FILES
        ${soldank_protocol_modules}
)

configure_shared_module_library(soldank_protocol)
target_link_libraries(soldank_protocol PUBLIC soldank_core)

add_library(shared_lib INTERFACE)
target_link_libraries(shared_lib INTERFACE soldank_core soldank_protocol)
