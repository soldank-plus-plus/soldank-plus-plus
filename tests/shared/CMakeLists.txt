add_executable(NetworkEventDispatcherTest communication/NetworkEventDispatcherTest.cpp)
AddTestOptionsAndLibraries(NetworkEventDispatcherTest)
target_link_libraries(NetworkEventDispatcherTest PRIVATE shared_lib)

add_executable(NetworkMessageTest communication/NetworkMessageTest.cpp)
AddTestOptionsAndLibraries(NetworkMessageTest)
target_link_libraries(NetworkMessageTest PRIVATE shared_lib)

add_executable(AnimationDataTest core/animations/AnimationDataTest.cpp)
AddTestOptionsAndLibraries(AnimationDataTest)
target_link_libraries(AnimationDataTest PRIVATE shared_lib)

add_executable(BodyAimAnimationStateTest core/animations/BodyAimAnimationStateTest.cpp)
target_sources(BodyAimAnimationStateTest
    PRIVATE
        FILE_SET body_aim_animation_state_test_modules TYPE CXX_MODULES FILES
        core/animations/AnimationStateTestHelpers.cpp
)
AddTestOptionsAndLibraries(BodyAimAnimationStateTest)
target_link_libraries(BodyAimAnimationStateTest PRIVATE shared_lib)

add_executable(BodyGetUpAnimationStateTest core/animations/BodyGetUpAnimationStateTest.cpp)
target_sources(BodyGetUpAnimationStateTest
    PRIVATE
        FILE_SET body_get_up_animation_state_test_modules TYPE CXX_MODULES FILES
        core/animations/AnimationStateTestHelpers.cpp
)
AddTestOptionsAndLibraries(BodyGetUpAnimationStateTest)
target_link_libraries(BodyGetUpAnimationStateTest PRIVATE shared_lib)

add_executable(AnimationStatesTest
    core/animations/BodyChangeAnimationStateTest.cpp
    core/animations/BodyProneAnimationStateTest.cpp
    core/animations/BodyProneMoveAnimationStateTest.cpp
    core/animations/BodyPunchAnimationStateTest.cpp
    core/animations/BodyRollAnimationStateTest.cpp
    core/animations/BodyRollBackAnimationStateTest.cpp
    core/animations/BodyStandAnimationStateTest.cpp
    core/animations/BodyThrowAnimationStateTest.cpp
    core/animations/BodyThrowWeaponAnimationStateTest.cpp
    core/animations/LegsCrouchAnimationStateTest.cpp
    core/animations/LegsCrouchRunAnimationStateTest.cpp
    core/animations/LegsCrouchRunBackAnimationStateTest.cpp
    core/animations/LegsFallAnimationStateTest.cpp
    core/animations/LegsGetUpAnimationStateTest.cpp
    core/animations/LegsJumpAnimationStateTest.cpp
    core/animations/LegsJumpSideAnimationStateTest.cpp
    core/animations/LegsProneAnimationStateTest.cpp
    core/animations/LegsProneMoveAnimationStateTest.cpp
    core/animations/LegsRollAnimationStateTest.cpp
    core/animations/LegsRollBackAnimationStateTest.cpp
    core/animations/LegsRunAnimationStateTest.cpp
    core/animations/LegsRunBackAnimationStateTest.cpp
    core/animations/LegsStandAnimationStateTest.cpp
)
target_sources(AnimationStatesTest
    PRIVATE
        FILE_SET animation_states_test_modules TYPE CXX_MODULES FILES
        core/animations/AnimationStateTestHelpers.cpp
)
AddTestOptionsAndLibraries(AnimationStatesTest)
target_link_libraries(AnimationStatesTest PRIVATE shared_lib)

add_executable(WeaponTest core/entities/WeaponTest.cpp)
AddTestOptionsAndLibraries(WeaponTest)
target_link_libraries(WeaponTest PRIVATE shared_lib)

add_executable(MapBuilderTest core/map/MapBuilderTest.cpp)
AddTestOptionsAndLibraries(MapBuilderTest)
target_link_libraries(MapBuilderTest PRIVATE shared_lib)
target_link_libraries(MapBuilderTest PRIVATE shared_lib_testing_framework)

add_executable(MapTest core/map/MapTest.cpp)
AddTestOptionsAndLibraries(MapTest)
target_link_libraries(MapTest PRIVATE shared_lib)

add_executable(CalcTest core/math/CalcTest.cpp)
AddTestOptionsAndLibraries(CalcTest)
target_link_libraries(CalcTest PRIVATE shared_lib)

add_executable(MovementTest core/physics/MovementTest.cpp)
target_sources(MovementTest
    PUBLIC
        FILE_SET all_movement_test_modules TYPE CXX_MODULES FILES 
        core/physics/SoldierMovementSimulation.cpp
)

AddTestOptionsAndLibraries(MovementTest)
target_link_libraries(MovementTest PRIVATE shared_lib)
target_link_libraries(MovementTest PRIVATE shared_lib_testing_framework)

add_executable(ObservableTest core/utility/ObservableTest.cpp)
AddTestOptionsAndLibraries(ObservableTest)
target_link_libraries(ObservableTest PRIVATE shared_lib)

add_test(NetworkEventDispatcherTest NetworkEventDispatcherTest)
add_test(NetworkMessageTest NetworkMessageTest)
add_test(AnimationDataTest AnimationDataTest)
add_test(BodyAimAnimationStateTest BodyAimAnimationStateTest)
add_test(BodyGetUpAnimationStateTest BodyGetUpAnimationStateTest)
add_test(AnimationStatesTest AnimationStatesTest)
add_test(WeaponTest WeaponTest)
add_test(MapBuilderTest MapBuilderTest)
add_test(MapTest MapTest)
add_test(CalcTest CalcTest)
# add_test(MovementTest MovementTest)
add_test(ObservableTest ObservableTest)

file(GLOB_RECURSE ANIMATION_FILE_PATHS ${soldatbase_SOURCE_DIR}/shared/anims/*.poa)
foreach(ANIMATION_FILE_PATH ${ANIMATION_FILE_PATHS})
    cmake_path(GET ANIMATION_FILE_PATH FILENAME ANIMATION_FILE)
    add_custom_command(TARGET MovementTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ANIMATION_FILE_PATH}"
            "$<TARGET_FILE_DIR:MovementTest>/anims/${ANIMATION_FILE}") 
endforeach()

file(GLOB_RECURSE OBJECT_FILE_PATHS ${soldatbase_SOURCE_DIR}/shared/objects/*.po)
foreach(OBJECT_FILE_PATH ${OBJECT_FILE_PATHS})
    cmake_path(GET OBJECT_FILE_PATH FILENAME OBJECT_FILE)
    add_custom_command(TARGET MovementTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBJECT_FILE_PATH}"
            "$<TARGET_FILE_DIR:MovementTest>/objects/${OBJECT_FILE}") 
endforeach()

add_custom_command(TARGET MovementTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
        "$<TARGET_FILE_DIR:MovementTest>/weapons.ini") 

add_custom_command(TARGET MovementTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
        "$<TARGET_FILE_DIR:MovementTest>/weapons_realistic.ini") 
