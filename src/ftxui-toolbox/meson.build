src = files(
  'ftxui_simulated_avr.cpp',
  'i2c_listener_renderer.cpp',
  'logs_renderer.cpp',
  'scroller.cpp',
  'uart_linebuffer_logger.cpp',
)

inc = include_directories('..')

ftxui_toolbox_lib = shared_library(
  'ftxui-toolbox',
  src,
  native: true,
  include_directories: inc,
  cpp_args: ['-std=c++20'],
  dependencies: [ftxui_dep, simavr_drp, simavr_toolbox_dep],
)

ftxui_toolbox_dep = declare_dependency(
  include_directories: inc,
  link_with: ftxui_toolbox_lib,
)