add_executable(sysmon
    main.c
    monitor.c
    net_monitor.c
    #ui.c
)

target_include_directories(sysmon PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(sysmon PRIVATE Threads::Threads)

# buscar ncurses con pkg-config — funciona en Fedora, Ubuntu y Debian
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncurses)
target_include_directories(sysmon PRIVATE ${NCURSES_INCLUDE_DIRS})
target_link_libraries(sysmon PRIVATE ${NCURSES_LIBRARIES})
