#!/bin/bash
ln -sf build/compile_commands.json .
ctags -R --exclude={build,compile_commands.json,meson.build,makeproject.sh,CMakeLists.txt,doxygen,docs,doxygen.conf} .
#doxygen doxygen.conf
meson compile -C build
