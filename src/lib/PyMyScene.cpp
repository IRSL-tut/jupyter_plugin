#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/iostream.h> // add_ostream_redirect
#include <pybind11/numpy.h>
//
#include <cnoid/PyUtil>
#include <cnoid/PyEigenTypes>

#include "myscene.h"

using namespace cnoid;
namespace py = pybind11;

PYBIND11_MODULE(MyScene, m)
{
    py::module::import("cnoid.Util");
#if 0
    m.def("instance", [](void) {
        return MySceneWidget::instance();
    });
#endif
    m.def("renderer", [](void) {
        MySceneWidget *m = MySceneWidget::instance();
        SceneRenderer *ret = nullptr;
        if (!!m) {
            ret = m->renderer;
        }
        return ret;
    });
}
