# HelenOS

HelenOS is a portable microkernel-based multiserver operating
system designed and implemented from scratch. It decomposes key
operating system functionality such as file systems, networking,
device drivers and graphical user interface into a collection of
fine-grained user space components that interact with each other
via message passing. A failure or crash of one component does not
directly harm others. HelenOS is therefore flexible, modular,
extensible, fault tolerant and easy to understand.

HelenOS does not aim to be a clone of any existing operating system
and trades compatibility with legacy APIs for cleaner design.
Most of HelenOS components have been made to order specifically for
HelenOS so that its essential parts can stay free of adaptation layers,
glue code, franken-components and the maintenance burden incurred by them.

* [Website](http://helenos.org)
* [Wiki](http://helenos.org/wiki)
* [Tickets](http://www.helenos.org/report/1)
* [How to contribute](http://www.helenos.org/wiki/HowToContribute)

## Portability

HelenOS runs on seven different processor architectures and machines ranging
from embedded ARM devices and single-board computers through multicore 32-bit
and 64-bit desktop PCs to 64-bit Itanium and SPARC rack-mount servers.

## Building

### Building the toolchain

In order to build HelenOS, one must first build the cross-compiler toolchain
(either as a root or by specifying the `CROSS_PREFIX` environment variable)
by running (example for the amd64 architecture, further list of targets can be
found in the `default` directory):

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

As an example, here are some of the packages you will need for Ubuntu 12.10 (may be out of date):

```
$ sudo apt-get install build-essential libgmp-dev libmpfr-dev ppl-dev libmpc-dev zlib1g-dev texinfo libtinfo-dev xutils-dev
```

Whereas for CentOS/Fedora, you will need:

```
# sudo dnf group install 'Development Tools'
# sudo dnf install wget texinfo libmpc-devel mpfr-devel gmp-devel PyYAML genisoimage
```
In case the toolchain script won't work no matter how hard you try, let us know.
Please supply as many relevant information (your OS and distribution, list of
installed packages with version information, the output of the toolchain script, etc.) as
possible.

### Configuring the build

Go back to the source root of HelenOS and start the build process:

```
$ cd ..
$ make PROFILE=amd64
```

Now HelenOS should automatically start building.

Note: If you installed the toolchain to a custom directory, make sure `CROSS_PREFIX`
environment variable is correctly set.

### Running the OS

When you get the command line back, there should be an `image.iso` file in the source
root directory. If you have QEMU, you should be able to start HelenOS by running:

```
$ ./tools/ew.py
```

For additional information about running HelenOS, see
[UsersGuide/RunningInQEMU](http://www.helenos.org/wiki/UsersGuide/RunningInQEMU) or
[UsersGuide/RunningInVirtualBox](http://www.helenos.org/wiki/UsersGuide/RunningInVirtualBox) or
see the files in contrib/conf.

## License

HelenOS is open source, free software. Its source code is available under
the BSD license. Some third-party components are licensed under GPL.
