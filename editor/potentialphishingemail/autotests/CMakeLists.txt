set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( kmail_potentialphishingemailjobtest_SRCS potentialphishingemailjobtest.cpp ../potentialphishingemailjob.cpp )
add_executable( kmail_potentialphishingemailjobtest ${kmail_potentialphishingemailjobtest_SRCS})
add_test(kmail_potentialphishingemailjobtest kmail_potentialphishingemailjobtest)
ecm_mark_as_test(kmail_potentialphishingemailjobtest)
target_link_libraries( kmail_potentialphishingemailjobtest Qt5::Test  KF5::Codecs)


set( kmail_potentialphishingdetaildialogtest_SRCS potentialphishingdetaildialogtest.cpp ../potentialphishingdetaildialog.cpp )
kde4_add_unit_test( kmail_potentialphishingdetaildialogtest ${kmail_potentialphishingdetaildialogtest_SRCS})
target_link_libraries( kmail_potentialphishingdetaildialogtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} )


