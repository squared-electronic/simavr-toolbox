project('simavr-toolbox', 'c', 'cpp')

src = files(
  'src/simavr-toolbox/ds3231_virt.cpp',
  'src/simavr-toolbox/hd44780.cpp',
  'src/simavr-toolbox/sim_47l04.cpp',
  'src/simavr-toolbox/sim_bouncy_switch.cpp',
  'src/simavr-toolbox/sim_gu7000.cpp',
  'src/simavr-toolbox/sim_i2c_base.cpp',
  'src/simavr-toolbox/sim_tca8418.cpp',
  'src/simavr-toolbox/sim_tlc59116.cpp',
  'src/simavr-toolbox/sim_tlp9202.cpp',
  'src/simavr-toolbox/timer.cpp',
)

inc = include_directories('src')

simavr_drp = dependency('simavr', native: true)

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