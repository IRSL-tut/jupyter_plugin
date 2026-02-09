#include <cnoid/PyUtil>

#include "../JupyterBar.h"

using namespace cnoid;
namespace py = pybind11;

PYBIND11_MODULE(JupyterPlugin, m)
{
    m.doc() = "Choreonoid(IRSL) JupyterPlugin module";

    auto base = py::module::import("cnoid.Base");

    py::class_<JupyterBar> (m, "JupyterBar")
        .def_static("instance", &JupyterBar::instance, py::return_value_policy::reference)
        .def("addUserButton", &JupyterBar::addUserButton)
        .def("addUserToggleButton", &JupyterBar::addUserToggleButton)
        .def("getUserButton", &JupyterBar::getUserButton)
        .def("getUserButtonState", &JupyterBar::getUserButtonState);
}
