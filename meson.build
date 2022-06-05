project('sokoban', 'c',
  version: '0.0.1',
  default_options: [
    'buildtype=debug',
    'warning_level=1',
    'c_std=gnu2x',
  ],
  license: 'MIT'
)

cxx = meson.get_compiler('c')

cflags = []

if cxx.get_id() == 'gcc' or cxx.get_id() == 'clang'
  cflags = [
    '-pipe',
    '-funsigned-char',
  ]
endif

add_project_arguments(cxx.get_supported_arguments(cflags), language: 'c')

subdir('src')
