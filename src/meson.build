source_files = [
  'Assign.c',
  'Definitions.c',
  'Distance.c',
  'Queue.c',
]

math_dep = cxx.find_library('m')
libtribble_dep = dependency('tribble')

executable('sokoban', 'sokoban.c',
  sources: source_files,
  dependencies: [libtribble_dep, math_dep]
)
