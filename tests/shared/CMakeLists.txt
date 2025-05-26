add_executable(NetworkEventDispatcherTest communication/NetworkEventDispatcherTest.cpp)
AddTestOptionsAndLibraries(NetworkEventDispatcherTest)
target_link_libraries(NetworkEventDispatcherTest PRIVATE shared_lib)

add_executable(NetworkMessageTest communication/NetworkMessageTest.cpp)
AddTestOptionsAndLibraries(NetworkMessageTest)
target_link_libraries(NetworkMessageTest PRIVATE shared_lib)

add_executable(AnimationDataTest core/animations/AnimationDataTest.cpp)
AddTestOptionsAndLibraries(AnimationDataTest)
target_link_libraries(AnimationDataTest PRIVATE shared_lib)

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

add_executable(MovementTest core/physics/MovementTest.cpp core/physics/SoldierMovementSimulation.cpp)
AddTestOptionsAndLibraries(MovementTest)
target_link_libraries(MovementTest PRIVATE shared_lib)
target_link_libraries(MovementTest PRIVATE shared_lib_testing_framework)

add_executable(ObservableTest core/utility/ObservableTest.cpp)
AddTestOptionsAndLibraries(ObservableTest)
target_link_libraries(ObservableTest PRIVATE shared_lib)

add_test(NetworkEventDispatcherTest NetworkEventDispatcherTest)
add_test(NetworkMessageTest NetworkMessageTest)
add_test(AnimationDataTest WeaponTest)
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
