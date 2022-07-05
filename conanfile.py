from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    build_requires = "cmake/3.23.0"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("sol2/3.3.0")
        self.requires("redis-plus-plus/1.3.3")
        self.requires("folly/2022.01.31.00")
        self.requires("boost/1.79.0")
        self.requires("di/1.2.0")
        self.requires("fmt/7.0.3")
        self.requires("stb/cci.20210910")


        # Testing only dependencies below
        self.requires("catch2/3.0.1")
