if(NOT LibSoup_FOUND)
  add_library(libsoup INTERFACE IMPORTED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBSOUP REQUIRED libsoup-gnome-2.4)
  set_property(TARGET libsoup PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${LIBSOUP_INCLUDE_DIRS})
  set_property(TARGET libsoup PROPERTY
    INTERFACE_COMPILE_OPTIONS ${LIBSOUP_CFLAGS})
  set_property(TARGET libsoup PROPERTY
    INTERFACE_LINK_OPTIONS ${LIBSOUP_LDFLAGS})
  set_property(TARGET libsoup PROPERTY
    INTERFACE_LINK_LIBRARIES ${LIBSOUP_LINK_LIBRARIES})
  set(LibSoup_FOUND ON)
endif()
