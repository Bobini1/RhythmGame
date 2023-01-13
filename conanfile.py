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
        self.requires("stb/cci.20210910")
        self.requires("ms-gsl/4.0.0")
        self.requires("type_safe/0.2.2")
        self.requires("luajit/2.0.5")
        self.requires("sfml/2.5.1")
        self.requires("di/1.2.0")
        self.requires("spdlog/1.11.0")
        self.requires("foonathan-lexy/2022.12.00")
        self.requires("gst-plugins-good/1.19.1")
        self.requires("zlib/1.2.13")
        self.requires("libalsa/1.2.7.2")
        self.requires("libffi/3.4.3")
        self.requires("freetype/2.12.1")
        self.requires("libpng/1.6.39")
        self.requires("expat/2.5.0")
        self.requires("glib/2.75.0")

        # Testing only dependencies below
        self.requires("catch2/3.0.1")

    def configure(self):
        # https://www.ffmpeg.org/legal.html
        self.options["gstreamer"].shared = True
        self.options["glib"].shared = True
        self.options["gst-plugins-base"].shared = True
        self.options["gst-plugins-good"].shared = True
        self.options["gst-libav"].shared = True
        self.options["ffmpeg"].shared = True
        self.options["ffmpeg"].with_libx264 = False
        self.options["ffmpeg"].with_libx265 = False
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].with_libfdk_aac = False

        self.options["ffmpeg"].with_zlib = False
