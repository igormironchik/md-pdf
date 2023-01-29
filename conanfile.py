from conan import ConanFile

class DependenciesRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"

    def requirements(self):
        self.requires("openssl/1.1.1s")
        self.requires("freetype/2.12.1")
        self.requires("libjpeg/9e")
        self.requires("libpng/1.6.39")
        self.requires("zlib/1.2.13")
        self.requires("libtiff/4.4.0")
        self.requires("libxml2/2.10.3")
        self.requires("libiconv/1.17")
        if self.settings.os == "Windows":
            self.requires("fontconfig/2.13.93")

    def configure(self):
        self.options["openssl"].shared = True
        self.options["freetype"].shared = True
        self.options["libjpeg"].shared = True
        self.options["libpng"].shared = True
        self.options["zlib"].shared = True
        self.options["libtiff"].shared = True
        self.options["libxml2"].shared = True
        self.options["libiconv"].shared = True
        self.options["fontconfig"].shared = True

    def imports(self):
        self.copy("*.dll", "bin", "bin")
        self.copy("*.dylib", "lib", "lib")
