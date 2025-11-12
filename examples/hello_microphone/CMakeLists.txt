# Remember to uncomment in the root CMakeLists.txt the corresponding add_subdirectory if you want to include this application in your project


add_executable(test_microphone
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_link_libraries(test_microphone PRIVATE
  pico_stdlib
  TKJHAT_SDK
)

pico_enable_stdio_usb(test_microphone 1)
pico_enable_stdio_uart(test_microphone 0)

pico_add_extra_outputs(test_microphone)
