from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps


class PowerCalcDeps(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("yaml-cpp/0.8.0")
        self.requires("gtest/1.14.0")
        self.requires("zlib/1.3.1")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False   # не создавать CMakeUserPresets.json в корне
        tc.generate()
        CMakeDeps(self).generate()