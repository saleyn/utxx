# utxx - an open-source collection of C++ utility components #

This dual-licensed library provides a set of classes that
complement C++ and boost with functionality needed by many
applications.

The library is free to use under LGPLv2 license for
non-commercial projects, and has a commercial open source licence
for commercial use (contact the author for commercial
licensing details).

The components include:

* Custom allocators (including atomic pool allocator, stack allocator)
* Atomic functions
* Bitmap mask with fast iteration
* BOOST wait_timeout, repeating timer
* Buffer for I/O handling
* Concurrent array/fifo/priority_queue/stack/hashmap
* Fast delegates
* Type conversion routines
* Endian-handling
* Exception error classes with dynamic number of arguments
* Hashmap abstraction
* UDP receiver
* Logging framework with pluggable back-ends using Inversion of Control design pattern 
* Ultra-low latency async logger
* Math functions
* Metaprogramming functions
* Persistent blob and array
* Futex signaling
* PCAP format reader/writer
* Throttling components
* PID file manager
* Variant / variant tree components
* Exceptions with error source location
* Reflectable enums
* Configuration framework with option validation (SCON configuration format support)

## Downloading ##
``$ git clone git@github.com:saleyn/utxx.git``

## Building ##
To customize location of BOOST or installation prefix, create a file called
`.cmake-args.${HOSTNAME}`. Alternatively if you are doing multi-host build with
identical configuration, create a file call `.cmake-args`. E.g.:

There are three sets of variables present in this file:

1. Build and install locations.

   * `DIR:BUILD=...`   - Build directory
   * `DIR:INSTALL=...` - Install directory

   They may contain macros:
   
      * `@PROJECT@`   - name of current project (from CMakeList.txt)
      * `@VERSION@`   - project version number  (from CMakeList.txt)
      * `@BUILD@`     - build type (from command line)
      * `${...}`      - environment variable

2. `ENV:VAR=...`     - Environment var set before running cmake

3. `VAR=...`         - Variable passed to cmake with -D prefix

Example:
```
$ cat > .cmake-args.${HOSTNAME}
DIR:BUILD=/tmp/@PROJECT@/build
DIR:INSTALL=/opt/pkt/@PROJECT@/@VERSION@
ENV:BOOST_ROOT=/opt/pkg/boost/current
ENV:BOOST_LIBRARYDIR=/opt/pkg/boost/current/gcc/lib
PKG_ROOT_DIR=/opt/pkg
```
Run:
```
$ make bootstrap [toolchain=gcc|clang]  [build=Debug|Release] \
                 [generator=make|ninja] [prefix=/usr/local] [verbose=true]
$ make [verbose=true]
$ make install      # Default install path is /usr/local
```
After running `make bootstrap` two local links are created `build` and `inst`
pointing to build and installation directories.

If you need to do a full cleanup of the current build and rerun bootstrap with
previously chosen options, do:
```
$ make distclean
$ make rebootstrap [toolchain=gcc|clang]  [build=Debug|Release]
```
Note that the `rebootstrap` command remembers previous bootstrap options, but
if you give it arguments they will override the old ones.

## Commit Notifications ##
The following news group was set up for commit notifications:
`github-utxx at googlegroups dot com`

## Contributing ##

### Writing Commit Messages ###
Structure your commit message like this:

<pre>
One line summary (less than 50 characters)

Longer description (wrap at 72 characters)
</pre>

#### Summary ####
* Less than 50 characters
* What was changed
* Imperative present tense (fix, add, change)
  * `Fix bug 123`
  * `Add 'foobar' command`
  * `Change default timeout to 123`
* No period

#### Longer Description ####
* Wrap at 72 characters
* Why, explain intention and implementation approach
* Present tense (fix, add, change)

#### Commit Atomicity ####
* Break up logical changes
* Make whitespace changes separately

#### Code Formatting ####
* Wrap at 80 characters (not counting '\n')
* Use 4-space indentation
* Expand tabs with spaces in indentations (for vi use settings: `ts=4:sw=4:et`)
* Place `"// vim:ts=4:sw=4:et"` at the beginning of each file
* Use K&R braces style:
```
if (...) {
   ...
} else {
   ...
}
```
## Author ##
* Serge Aleynikov `<saleyn at gmail dot com>`
* (see AUTHORS.md for a list of contributors)

## LICENSE ##
* Non-commercial: GNU Lesser General Public License 2.1
* Commercial:     (contact the author for details)
