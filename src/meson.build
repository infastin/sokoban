source_files = [
  'Assign.c',
  'Definitions.c',
  'Distance.c',
  'Game.c',
]

math_dep = cxx.find_library('m')
ncurses_dep = cxx.find_library('ncurses')
libtribble_dep = dependency('tribble')

executable('main', 'main.c',
  sources: source_files,
  dependencies: [libtribble_dep, ncurses_dep, math_dep]
)
