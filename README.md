# About #
This is ほたるび (Hotarubi), a hobby OS developed from scratch just for the sake of it.   
The development started in August 2014 and is ongoing.   

Depending on how things work out there may one day be more sources like 
userspace or even ports of existing software here.   
For now it's just the sources required to build the kernel itself.

*Unless otherwise stated all files are licensed under the GNU GPL* (see COPYING for details).   
Most files contain proper licensing headers at their start however a few file may not.   
These files are mostly in-development code which I consider unstable I that I'm currently   
working on. *All of these files are covered by the GNU GPL too.*

## Building ##

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

[1]: https://github.com/jimweirich/rake#rake--ruby-make 
[2]: http://gcc.gnu.org
[3]: http://clang.llvm.org
[4]: https://developer.apple.com/xcode/downloads/
