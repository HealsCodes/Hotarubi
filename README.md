# Wellcome! #
This is ほたるび (Hotarubi), a hobby OS developed from scratch just for the sake of it.   
The development started in August 2014 and is ongoing.   

Depending on how things work out there may one day be more sources like 
userspace or even ports of existing software here.   
For now it's just the sources required to build the kernel itself.

*Unless otherwise stated all files are licensed under the GNU GPL version 3 or,*   
 *at your oppinion any later version (see COPYING for details).*   

Refer to the license headers in individual files for different / additional licensing terms.   
Some files, for example parts of the build system, may not contain explicit licensing headers.
In this case the files should be considered to be licensed as stated in COPYING.
(These files are mostly in-development code which I consider unstable or that I'm currently   
working on.)

## Building [![Build Status](https://travis-ci.org/Shirk/Hotarubi.svg?branch=kyushu_dev)](https://travis-ci.org/Shirk/Hotarubi) ##

ほたるび uses [Rake][1] as the build system of choice.   
In addition to that a working C/C++ compiler is required to build the needed tools.

All recent versions of [GCC][2] 4.7 or newer should work.   
For builds on OS X systems the [clang][3] compiler that is installed along with [XCode][4]
or as part of the *Commandline Developer Tools* will do just fine.

### Toolchain ###
Building the kernel requires a [GCC][2] based toolchain targeting `x86_64-elf`.   
This source tree includes some patches to the stock [GCC][2] to be able to use libgcc
and a few other builtin features for free.

To download, patch and build the toolchain run:   
`rake toolchain:build`   

After building the toolchain temporary files can be removed with:   
`rake toolchain:clean`   

This will delete the archives, extracted sources and build trees.

### Kernel ###

After having the toolchain in-place the kernel can be build with a simple `rake` call.   

Use `rake clean` to remove any intermediate build results or `rake clobber` to remove all binaries and
intermediate files.   

### Testing ###

Some parts of ほたるび can be tested offline without having to build and run the complete kernel.
To provide a robust test suite the source tree contains a fused copy of the [Google Test Framework (gtest)][5].   

*gtest is released under the New BSD License - see scripts/test/gtest/LICENSE for details.*   

Local tests can be build and run by executing:   
`rake test:offline`

Additionally there is a special target to clean only the test executables:   
`rake test:clean`   
(this is also part of the regular `rake clean` task.)

[1]: https://github.com/jimweirich/rake#rake--ruby-make 
[2]: http://gcc.gnu.org
[3]: http://clang.llvm.org
[4]: https://developer.apple.com/xcode/downloads/
[5]: https://code.google.com/p/googletest/
