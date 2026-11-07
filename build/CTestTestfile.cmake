# CMake generated Testfile for 
# Source directory: /home/dasha/Загрузки/HW3D/hw3d
# Build directory: /home/dasha/Загрузки/HW3D/hw3d/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(geometry "/home/dasha/Загрузки/HW3D/hw3d/build/check_project")
set_tests_properties(geometry PROPERTIES  _BACKTRACE_TRIPLES "/home/dasha/Загрузки/HW3D/hw3d/CMakeLists.txt;70;add_test;/home/dasha/Загрузки/HW3D/hw3d/CMakeLists.txt;0;")
add_test(render "/home/dasha/Загрузки/HW3D/hw3d/build/check_render")
set_tests_properties(render PROPERTIES  _BACKTRACE_TRIPLES "/home/dasha/Загрузки/HW3D/hw3d/CMakeLists.txt;71;add_test;/home/dasha/Загрузки/HW3D/hw3d/CMakeLists.txt;0;")
add_test(command_line "bash" "/home/dasha/Загрузки/HW3D/hw3d/tests/run_cli_tests.sh" "/home/dasha/Загрузки/HW3D/hw3d/build/hw3d")
set_tests_properties(command_line PROPERTIES  _BACKTRACE_TRIPLES "/home/dasha/Загрузки/HW3D/hw3d/CMakeLists.txt;72;add_test;/home/dasha/Загрузки/HW3D/hw3d/CMakeLists.txt;0;")
