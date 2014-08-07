include_directories(
  ${CMAKE_SOURCE_DIR}/kmail/mailmerge
)
set(mailmergewidget_gui_SRCS mailmergewidgettest_gui.cpp ../mailmergewidget.cpp ../attachmentlistwidget.cpp)
add_executable(mailmergewidget_gui ${mailmergewidget_gui_SRCS})
target_link_libraries(mailmergewidget_gui
  KF5::KIOCore
  pimcommon
)

