src = files(
    'ftxui_simulated_avr.cpp',
    'logs_renderer.cpp',
    'scroller.cpp',
)

inc = include_directories('..')

ftxui_toolbox_lib = shared_library(
    'ftxui-toolbox',
    src,
    native: true,
    include_directories: inc,
    dependencies: [ftxui_dep, simavr_drp, simavr_toolbox_dep],
)

ftxui_toolbox_dep = declare_dependency(
    include_directories: inc,
    link_with: ftxui_toolbox_lib,
)