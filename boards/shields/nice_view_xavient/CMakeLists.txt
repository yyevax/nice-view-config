if(CONFIG_ZMK_DISPLAY AND CONFIG_NICE_VIEW_WIDGET_STATUS)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)

  zephyr_library_sources(custom_status_screen.c)
  zephyr_library_sources(widgets/bolt.c)
  zephyr_library_sources(widgets/util.c)

  # MASTER / CENTRAL
  if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

    zephyr_library_sources(widgets/status.c)

    if(CONFIG_SHIELD_XAVIEN_LEFT_MASTER)
      zephyr_library_sources(widgets/left.c)

    elseif(CONFIG_SHIELD_XAVIEN_RIGHT_MASTER)
      zephyr_library_sources(widgets/right.c)
    endif()

  # LEFT PERIPHERAL
  elseif(CONFIG_SHIELD_XAVIEN_LEFT)

    zephyr_library_sources(widgets/peripheral_status.c)
    zephyr_library_sources(widgets/left.c)

  # RIGHT PERIPHERAL
  elseif(CONFIG_SHIELD_XAVIEN_RIGHT)

    zephyr_library_sources(widgets/peripheral_status.c)
    zephyr_library_sources(widgets/right.c)

  endif()
endif()