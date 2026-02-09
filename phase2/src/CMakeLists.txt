set(SOURCES
    main.cpp
    action_camera.cpp
    button.cpp
    ssd1306.cpp
)
set(HEADERS
    action_camera.hpp
    button.hpp
    ssd1306.hpp
    ssd1306_font.hpp
)

add_executable(action_cam ${SOURCES})
target_link_libraries(action_cam PRIVATE
    ${GPIOD_LIBRARIES}
    Threads::Threads
)
target_include_directories(action_cam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GPIOD_INCLUDE_DIRS}
)
target_compile_options(action_cam PRIVATE
    ${GPIOD_CFLAGS_OTHER}
)
set_target_properties(action_cam PROPERTIES FOLDER "Application")
