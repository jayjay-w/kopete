 include_directories(
    ${KOPETE_INCLUDES}
)

########### next target ###############
set(kopete_history_PART_SRCS chathistoryhandler.cpp databaseconstants.cpp)
kde4_add_plugin(kopete_history ${kopete_history_PART_SRCS} )
target_link_libraries(kopete_history ${KDE4_KHTML_LIBS} kopete ${QT_QTSQL_LIBRARY})

install(TARGETS kopete_history  DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############
install( FILES kopete_history.desktop DESTINATION ${SERVICES_INSTALL_DIR})
