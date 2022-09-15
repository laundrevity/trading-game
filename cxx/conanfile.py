from conans import ConanFile, CMake

class FakexConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "boost/1.78.0",
        "gtest/1.11.0",
        "protobuf/3.21.4"
    ]

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
