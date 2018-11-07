set(common_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/../unifiedmailboxagent_debug.cpp
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

add_definitions(-DUNIT_TESTS)
kconfig_add_kcfg_files(unifiedmailbox_agent_autotest_SRCS
    ../settings.kcfgc
)

add_akonadi_isolated_test(SOURCE unifiedmailboxmanagertest.cpp
    ADDITIONAL_SOURCES ${common_SRCS} ../unifiedmailboxmanager.cpp ../unifiedmailbox.cpp ${unifiedmailbox_agent_autotest_SRCS}
    LINK_LIBRARIES KF5::I18n KF5::AkonadiMime KF5::ConfigGui
)
