from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    build_requires = "cmake/3.22.0"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("freetype/2.12.1@#0bbb81805455cee1339cfd08a4682f1b")
        self.requires("libpng/1.6.37@#abacd98afb859828aa70354fa7d55285")
        self.requires("sol2/3.3.0")
        self.requires("sqlitecpp/3.1.1")
        self.requires("boost/1.79.0")
        self.requires("fmt/7.0.3")
        self.requires("stb/cci.20210910")
        self.requires("taocpp-pegtl/3.2.6")
        self.requires("ms-gsl/4.0.0")
        self.requires("type_safe/0.2.2")
        self.requires("luajit/2.0.5")
        self.requires("cocoyaxi/2.0.3")
        self.requires("sfml/2.5.1")
        self.requires("di/1.2.0")

        # Testing only dependencies below
        self.requires("catch2/3.0.1")
