# packages
configure_file(deb-functions.sh "${CMAKE_CURRENT_BINARY_DIR}/deb-functions.sh" @ONLY)

add_custom_target(deb
  "${CMAKE_CURRENT_SOURCE_DIR}/build_plumgrid-sigmund_deb"
  )
