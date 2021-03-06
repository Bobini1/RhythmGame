from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    build_requires = "cmake/3.23.0"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("openssl/1.1.1o")
        self.requires("sol2/3.3.0")
        self.requires("sqlitecpp/3.1.1")
        self.requires("boost/1.79.0")
        self.requires("fmt/7.0.3")
        self.requires("stb/cci.20210910")
        self.requires("taocpp-pegtl/3.2.6")
        self.requires("ms-gsl/4.0.0")
        self.requires("type_safe/0.2.2")
        self.requires("luajit/2.0.5")
        # self.requires("folly/2022.01.31.00")

        # Testing only dependencies below
        self.requires("catch2/3.0.1")
