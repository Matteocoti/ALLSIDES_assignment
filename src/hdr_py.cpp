#include <cstring>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "FakeCamera.hpp"
#include "HdrCombiner.hpp"

namespace py = pybind11;

constexpr int RES = 4504;
using PyScene = SyntheticScene<RES, RES>;
using PyCamFrame = CameraFrame<RES, RES>;
using PyCamera = FakeCamera<RES, RES>;
using PyHdrFrame = HdrFrame<RES, RES>;
using PyCombiner = HdrCombiner<RES, RES>;

PYBIND11_MODULE(hdr_py, m)
{
    m.doc() = "FakeCamera + HDR Combiner Python bindings";

    py::enum_<ExposureTime>(m, "ExposureTime")
        .value("Short", ExposureTime::Short)
        .value("Medium", ExposureTime::Medium)
        .value("Long", ExposureTime::Long);

    py::class_<PyScene>(m, "SyntheticScene")
        .def(py::init<>())
        .def_property_readonly("data", [](const PyScene &self) {
            auto arr = py::array_t<float>(RES * RES);
            std::memcpy(arr.mutable_data(), self.data(), RES * RES * sizeof(float));
            return arr;
        })
        .def_property_readonly("width", [](const PyScene &) { return RES; })
        .def_property_readonly("height", [](const PyScene &) { return RES; });

    py::class_<PyCamFrame>(m, "CameraFrame")
        .def(py::init<ExposureTime>())
        .def_property_readonly("data", [](const PyCamFrame &self) {
            auto arr = py::array_t<uint16_t>(RES * RES);
            std::memcpy(arr.mutable_data(), self.data(), RES * RES * sizeof(uint16_t));
            return arr;
        })
        .def_readonly("exposure", &PyCamFrame::exposure)
        .def_property_readonly("width", [](const PyCamFrame &) { return RES; })
        .def_property_readonly("height", [](const PyCamFrame &) { return RES; });

    py::class_<PyCamera>(m, "FakeCamera")
        .def(py::init<const PyScene &, float>(), py::arg("scene"), py::arg("noise_std_dev") = 5.0f)
        .def("grab", &PyCamera::grab, py::arg("exposure"));

    py::class_<PyHdrFrame>(m, "HdrFrame")
        .def_property_readonly("data", [](const PyHdrFrame &self) {
            auto arr = py::array_t<float>(RES * RES);
            std::memcpy(arr.mutable_data(), self.data(), RES * RES * sizeof(float));
            return arr;
        })
        .def_property_readonly("width", [](const PyHdrFrame &) { return RES; })
        .def_property_readonly("height", [](const PyHdrFrame &) { return RES; });

    py::class_<PyCombiner>(m, "HdrCombiner")
        .def(py::init<>())
        .def("merge", [](const PyCombiner &self, const std::vector<PyCamFrame> &frames) {
            return self.merge(frames);
        }, py::arg("frames"));
}
