include_directories(
${KOPETE_INCLUDES}
)

set (kopete_history3_PART_SRCS
    History3Logger.cpp
    databasehelper.cpp
)

kde4_add_kcfg_files(kopete_history3_PART_SRCS history3config.kcfgc )
kde4_add_plugin(kopete_history3 ${kopete_history3_PART_SRCS})
target_link_libraries(kopete_history3  ${KDE4_KHTML_LIBS} kopete ${QT_QTSQL_LIBRARY})
install(TARGETS kopete_history3  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kcm_kopete_history3_PART_SRCS databasehelper.cpp)
kde4_add_kcfg_files(kcm_kopete_history3_PART_SRCS history3config.kcfgc )
kde4_add_plugin(kcm_kopete_history3 ${kcm_kopete_history3_PART_SRCS})

target_link_libraries(kcm_kopete_history3  ${KDE4_KUTILS_LIBS} ${KDE4_KHTML_LIBS} kopete ${QT_QTSQL_LIBRARY})

install(TARGETS kcm_kopete_history3  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############
install( FILES history3config.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kopete_history3.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES kopete_history3_config.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kconfiguredialog)


