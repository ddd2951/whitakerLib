// INFO: The Python module: the two helpers the generated code uses, and the generated code.
#include <whitaker.hpp>

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace {

[[nodiscard]] py::object text(const char* const value) {
  return value != nullptr ? py::object(py::str(value)) : py::none();
}

// NOTE: A struct reaches Python as a SimpleNamespace of copies: `form.stemSpelling`, and it prints its fields.
[[nodiscard]] py::object record(const py::dict& fields) {
  return py::module_::import("types").attr("SimpleNamespace")(**fields);
}

#include "whitaker_generated.inc"

} // namespace

PYBIND11_MODULE(whitaker, m) { bindGenerated(m); }
