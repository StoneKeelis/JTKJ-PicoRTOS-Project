add_library(usb_serial_debug STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
  ${CMAKE_CURRENT_LIST_DIR}/src/helper.c
)

target_include_directories(usb_serial_debug
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/config
)


#target_compile_definitions(cfg-usbcdc INTERFACE
#  TUSB_CONFIG_FILE="\"${CMAKE_CURRENT_LIST_DIR}/config/tusb_config.h\""
#)

target_link_libraries(usb_serial_debug
  PUBLIC
    pico_unique_id
    tinyusb_board
    tinyusb_device
  PRIVATE
    pico_stdlib
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

#Backwards compatibility
add_library(cfg-dual-usbcdc ALIAS usb_serial_debug)