set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( kmail_potentialphishingemailjobtest_SRCS potentialphishingemailjobtest.cpp ../potentialphishingemailjob.cpp )
kde4_add_unit_test( kmail_potentialphishingemailjobtest ${kmail_potentialphishingemailjobtest_SRCS})
target_link_libraries( kmail_potentialphishingemailjobtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_KPIMUTILS_LIBS})


set( kmail_potentialphishingdetaildialogtest_SRCS potentialphishingdetaildialogtest.cpp ../potentialphishingdetaildialog.cpp )
kde4_add_unit_test( kmail_potentialphishingdetaildialogtest ${kmail_potentialphishingdetaildialogtest_SRCS})
target_link_libraries( kmail_potentialphishingdetaildialogtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} )


