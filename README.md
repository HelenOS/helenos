# HelenOS

HelenOS is a portable microkernel-based multiserver operating
system designed and implemented from scratch. It decomposes key
operating system functionality such as file systems, networking,
device drivers and graphical user interface into a collection of
fine-grained user space components that interact with each other
via message passing. A failure or crash of one component does not
directly harm others. HelenOS is therefore flexible, modular,
extensible, fault tolerant and easy to understand.

![screenshot](https://www.helenos.org/raw-attachment/wiki/Screenshots/gui-14.1-aio.png "Screenshot")

HelenOS aims to be compatible with the C11 and C++14 standards, but does not
aspire to be a clone of any existing operating system and trades compatibility
with legacy APIs for cleaner design. Most of HelenOS components have been made
to order specifically for HelenOS so that its essential parts can stay free of
adaptation layers, glue code, franken-components and the maintenance burden
incurred by them.

* [Website](https://helenos.org)
* [Wiki](https://helenos.org/wiki)
* [Tickets](https://www.helenos.org/report/1)
* [How to contribute](https://www.helenos.org/wiki/HowToContribute)

## Portability

HelenOS runs on eight different processor architectures and machines ranging
from embedded ARM devices and single-board computers through multicore 32-bit
and 64-bit desktop PCs to 64-bit Itanium and SPARC rack-mount servers.

## Building

### Building the toolchain

In order to build HelenOS, one must first build the cross-compiler toolchain
(the default installation location can be overridden by specifying the
`CROSS_PREFIX` environment variable) by running (example for the amd64
architecture, further list of targets can be found in the `default` directory):

```
$ cd HelenOS/tools
$ ./toolchain.sh amd64
```

The toolchain script will print a list of software packages that are required
for the toolchain to correctly build. Make sure you install all the dependencies.
Unfortunately, the script cannot install the required dependencies for you automatically
since the host environments are very diverse. In case the compilation of the toolchain
fails half way through, try to analyze the error message(s), add appropriate missing
dependencies and try again.

As an example, here are some of the packages you will need for Ubuntu 16.04:

```
$ sudo apt install build-essential wget texinfo flex bison dialog python-yaml genisoimage
```

Whereas for CentOS/Fedora, you will need:

```
# sudo dnf group install 'Development Tools'
# sudo dnf install wget texinfo PyYAML genisoimage flex bison
```

In case the toolchain script won't work no matter how hard you try, let us know.
Please supply as many relevant information (your OS and distribution, list of
installed packages with version information, the output of the toolchain script, etc.) as
possible.

### Configuring the build

Since the summer of 2019, HelenOS uses the Meson build system.
Make sure you have a recent-enough version of Meson and Ninja.
The safest bet is installing both using `pip3` tool.

```sh
$ pip3 install ninja
$ pip3 install meson
```

Meson does not support in-tree builds, so you have to create a directory
for your build. You can have as many build directories as you want, each with
its own configuration. `cd` into your build directory and run `configure.sh`
script which exists in the source root. `configure.sh` can be run with a profile
name, to use one of the predefined profiles, or without arguments for interactive
configuration.

```sh
$ git clone https://github.com/HelenOS/helenos.git
$ mkdir -p build/amd64
$ cd build/amd64
$ ../../helenos/configure.sh amd64
```

Note: If you installed the toolchain to a custom directory, make sure `CROSS_PREFIX`
environment variable is correctly set.

Once configuration is finished, use `ninja` to build HelenOS.
Invoking `ninja` without arguments builds all binaries and
debug files, but not bootable image. This is because during
development, most builds are incremental and only meant to check
that code builds properly. In this case, the time-consuming process of
creating a boot image is not useful and takes most time. This behavior
might change in the future.

In case you want to rebuild the bootable image, you must invoke
`ninja image_path`. This also emits the name of the bootable image into the
file `image_path` in build directory.

```
$ ninja
$ ninja image_path
```

Now HelenOS should automatically start building.

### Running the OS

When you get the command line back, there should be an `image.iso` file in the build
root directory. If you have QEMU, you should be able to start HelenOS by running:

```
$ ./tools/ew.py
```

For additional information about running HelenOS, see
[UsersGuide/RunningInQEMU](https://www.helenos.org/wiki/UsersGuide/RunningInQEMU) or
[UsersGuide/RunningInVirtualBox](https://www.helenos.org/wiki/UsersGuide/RunningInVirtualBox) or
see the files in tools/conf.

## Contributing

There is a whole section of our wiki devoted to
[how to contribute to HelenOS](https://www.helenos.org/wiki/HowToContribute).
But to highlight the most important points, you should subscribe to
our mailing list and if you have an idea for contributing, discuss it
with us first so that we can agree upon the design.

Especially if you are a first time contributor, blindingly shooting
a pull request may result in it being closed on the grounds that it
does not fit well within the grand scheme of things. That would not
be efficient use of time.

Communicating early and often is the key to successful acceptance of
your patch/PR.

## License

HelenOS is open source, free software. Its source code is available under
the BSD license. Some third-party components are licensed under GPL.
