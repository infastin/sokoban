source_files = [
  'Assign.c',
  'Definitions.c',
  'Distance.c',
]

math_dep = cxx.find_library('m')
libtribble_dep = dependency('tribble')

executable('main', 'main.c',
  sources: source_files,
  dependencies: [libtribble_dep, math_dep]
)
