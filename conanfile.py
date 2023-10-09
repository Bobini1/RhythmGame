from conan.tools.cmake import CMakeToolchain

from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    # enable the runenv_info from gst-plugins-good

    def requirements(self):
        self.requires("sqlitecpp/3.3.0")
        self.requires("boost/1.82.0")
        self.requires("spdlog/1.11.0")
        self.requires("foonathan-lexy/2022.12.00")
        self.requires("ffmpeg/6.0")
        self.requires("openal-soft/1.22.2")
        self.requires("qt/6.5.2")
        self.requires("sqlite3/3.42.0", override=True)
        self.requires("brotli/1.1.0", override=True)
        self.requires("cryptopp/8.7.0")
        self.requires("magic_enum/0.9.3")
        self.requires("zstd/1.5.5")
        self.requires("libsndfile/1.2.2")
        self.requires("opus/1.4", override=True)
        self.requires("libalsa/1.2.10", override=True)
        self.requires("zlib/1.3", override=True)

        # Testing only dependencies below
        self.requires("catch2/3.4.0")

    def configure(self):
        # https://www.ffmpeg.org/legal.html
        self.options["ffmpeg"].shared = False
        self.options["ffmpeg"].with_libx264 = False
        self.options["ffmpeg"].with_libx265 = False
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].with_libfdk_aac = False
        self.options["ffmpeg"].with_openjpeg = False
        self.options["ffmpeg"].with_asm = False
        self.options["qt"].shared = True
        self.options["qt"].qtshadertools = True
        self.options["qt"].qtdeclarative = True
