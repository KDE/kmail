include_directories(
  ${CMAKE_SOURCE_DIR}/kmail/mailmerge
)
set(mailmergewidget_gui_SRCS mailmergewidgettest_gui.cpp ../widgets/mailmergewidget.cpp ../widgets/attachmentlistwidget.cpp ../widgets/csvwidget.cpp ../widgets/addressbookwidget.cpp ../widgets/selectattachmentdialog.cpp)
add_executable(mailmergewidget_gui ${mailmergewidget_gui_SRCS})
target_link_libraries(mailmergewidget_gui
  KF5::KIOCore
  KF5::KIOWidgets
  pimcommon
)

