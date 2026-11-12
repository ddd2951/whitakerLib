# INFO: The whitaker Python module:
#   cmake -S . -B build-python -DCMAKE_BUILD_TYPE=Release -DWHITAKER_BUILD_PYTHON=ON \
#     -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)
#   cmake --build build-python --target whitaker_python
find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(whitaker_python module.cpp)
target_link_libraries(whitaker_python PRIVATE whitaker)
target_compile_features(whitaker_python PRIVATE cxx_std_23)
set_target_properties(whitaker_python PROPERTIES OUTPUT_NAME whitaker)
