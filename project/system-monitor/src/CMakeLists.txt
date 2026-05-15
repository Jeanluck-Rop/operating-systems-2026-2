add_executable(sysmon
    main.c
    monitor.c
)

target_include_directories(sysmon PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(sysmon PRIVATE Threads::Threads)
