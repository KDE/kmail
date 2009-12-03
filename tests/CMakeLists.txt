set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

MACRO(KMAIL_ADD_UNITTEST _source )
  set(_test ${_source} ${ARGN})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME kmail-${_name} ${_test})
  target_link_libraries(
    ${_name}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_AKONADI_CONTACT_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${SOPRANO_LIBRARIES}
    ${NEPOMUK_LIBRARIES}
    ${NEPOMUK_QUERY_LIBRARIES}
    messagecore
    messageviewer
    niefast_apps
    kmailprivate
  )
ENDMACRO(KMAIL_ADD_UNITTEST)


########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt4_add_dbus_interfaces(dbustest_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml)
kde4_add_executable(dbustest TEST ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest ${KDE4_KIO_LIBS})

########### recipientseditor test ###############

set(recipienteditortest_SRCS recipientseditortest.cpp)


kde4_add_executable(recipienteditortest TEST ${recipienteditortest_SRCS})

target_link_libraries(recipienteditortest ${KDE4_KIO_LIBS} kmailprivate kdepim)


###### TODO port storagelayer tests to QTestLib

#kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp

set(utiltest_SRCS utiltests.cpp ../util.cpp)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "../metatype.h")
  qt4_add_dbus_interface(utiltest_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro(add_resource_iface)

add_resource_iface( imap/imapresource.kcfg
                    org.kde.Akonadi.Imap.Settings ImapSettings )


kmail_add_unittest( ${utiltest_SRCS} )

kmail_add_unittest( searchpatterntest.cpp )
