project('simavr-toolbox', 'c', 'cpp')

ftxui_dep = dependency('ftxui', native: true, required: false)

simavr_drp = dependency('simavr', native: true)

subdir('src/simavr-toolbox')

if ftxui_dep.found()
  subdir('src/ftxui-toolbox')
endif