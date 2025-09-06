src = files(
    'ds3231_virt.cpp',
    'hd44780.cpp',
    'sim_47l04.cpp',
    'sim_base.cpp',
    'sim_bouncy_switch.cpp',
    'sim_gu7000.cpp',
    'sim_i2c_base.cpp',
    'sim_tca8418.cpp',
    'sim_tlc59116.cpp',
    'sim_tlp9202.cpp',
    'timer.cpp',
)

inc = include_directories('.')

simavr_toolbox_lib = shared_library(
    'simavr-toolbox',
    src,
    include_directories: inc,
    dependencies: [simavr_drp],
)

simavr_toolbox_dep = declare_dependency(
    include_directories: inc,
    link_with: simavr_toolbox_lib,
)