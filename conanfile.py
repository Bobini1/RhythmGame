from conan.tools.cmake import CMakeToolchain

from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    # enable the runenv_info from gst-plugins-good

    def requirements(self):
        self.requires("sol2/3.3.0")
        self.requires("sqlitecpp/3.1.1")
        self.requires("boost/1.79.0")
        self.requires("ms-gsl/4.0.0")
        self.requires("luajit/2.1.0-beta3")
        self.requires("sfml/2.5.1")
        self.requires("spdlog/1.11.0")
        self.requires("foonathan-lexy/2022.12.00")
        self.requires("ffmpeg/5.0")

        # version overrides
        self.requires("zlib/1.2.13")
        if self.settings.os == "Linux":
            self.requires("libalsa/1.2.7.2")
            self.requires("libffi/3.4.3")
            self.requires("flac/1.4.2")
            self.requires("libsndfile/1.2.0")

        # Testing only dependencies below
        self.requires("catch2/3.3.2")

    def configure(self):
        # https://www.ffmpeg.org/legal.html
        self.options["ffmpeg"].shared = False
        self.options["ffmpeg"].with_libx264 = False
        self.options["ffmpeg"].with_libx265 = False
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].with_libfdk_aac = False
        self.options["ffmpeg"].with_openjpeg = False
