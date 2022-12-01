
[![Build](https://github.com/igormironchik/md-pdf/workflows/build/badge.svg)](https://github.com/igormironchik/md-pdf/actions)[![codecov](https://codecov.io/gh/igormironchik/md-pdf/branch/master/graph/badge.svg?token=LA0S72FLL2)](https://codecov.io/gh/igormironchik/md-pdf)[![License: GPL](https://img.shields.io/badge/license-GPL-blue)](https://opensource.org/licenses/GPL-3.0)

# About

Converter of Markdown to PDF.

Why another Markdown to PDF converter?
Because I want to feed root Markdown file to the application and get
complete PDF with all subpages that linked in root file. I tried to find
such ready tool, but my search was unsuccessful.

# Testing

If you will find a bug of rendering, please, try to localize the issue in
short Markdown and reproduce with fonts in `tests/fonts` folder (`Droid Serif` for text
and `Courier New` for code). And post the issue here...

# Getting from Repository

After clone update submodules with the next command:

```
git submodule update --init --recursive
```

# Building

To build this application first of all install dependencies of PoDoFo

* freetype
* libjpeg
* libpng
* zlib

On UNIX you also need

* fontconfig

Also ImageMagick6 is required.

With these dependencies installed just open CMakeLists.txt in QtCreator and run build.

You can use Conan to install dependencies, only ImageMagick should be installed by hands, as in
Conan ImageMagick available version is 7 only, and not all platforms are supported. And
Fontconfig should be installed in system, as using Fontconfig from Conan leads to ugly UI. In case
using Conan as dependencies manager define `MDPDF_USE_CONAN` to CMake. Good luck!

And one more thing - don't use `jom`.

## Building with QtCreator and Conan

 * Enable `Conan` plugin in `QtCreator`
 * Open `CMakeLists.txt` with `QtCreator`
 * Go on `Project` page and on build page move up `Conan` build step to the very top position
 * There disable `CMake` step
 * Rub build with `QtCreator`, this will install dependencies with `Conan`
 * Enable `CMake` step on the build page
 * Re-run `CMake`
 * And voila, run build and everything should go without any problem.
 
In this stack you can have problems only with `ImageMagick`, that I don't install with
`Conan`, as there is no build possible on Windows platform with `Conan`. So be sure you installed
`ImageMagick 6` and installation is available through the `PATH` environment, this should
be enough to detect `ImageMagick`. Good luck.

# Example

Good example of work of this application is
[PDF version of my book.](https://github.com/igormironchik/qt-widgets-on-real-example/blob/master/pdf/The%20book%20about%20real%20examples%20of%20Qt%20Widgets%20usage.pdf)

# Screenshot

![](mdpdf.png)

# Known issues
 
 * I do not render HTML tags in Markdown.

 * Some LaTeX Math expressions can be wrongly rendered. I use very good
 library `JKQtPlotter` to render LaTeX Math, and not everything is
 implemented there. But most common math things are done.

 * I don't support Chinese, as they don't use spaces. I don't know theirs rules, to adapt
 algorithms.
   
 * I don't support right-to-left languages. I don't know theirs rules too.
 
 If you are one from these groups of people - you are welcome to make PRs to adapt
 this application to your language.
