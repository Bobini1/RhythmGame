from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        #self.requires("lua/5.4.4")
        self.requires("sol2/3.3.0")
        self.requires("redis-plus-plus/1.3.3")

        # Testing only dependencies below
        self.requires("catch2/3.0.1")
