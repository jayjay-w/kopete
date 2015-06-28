include_directories(
    ${KOPETE_INCLUDES}
)

########### next target ###############

set(kopete_history3_PART_SRCS history3plugin.cpp databasemanager.cpp chathistoryhandler.cpp )
kde4_add_plugin(kopete_history3 ${kopete_history3_PART_SRCS} )

target_link_libraries(kopete_history3 ${KDE4_KHTML_LIBS} kopete ${QT_QTSQL_LIBRARY})

install(TARGETS kopete_history3  DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############
install( FILES kopete_history3.desktop DESTINATION ${SERVICES_INSTALL_DIR})
