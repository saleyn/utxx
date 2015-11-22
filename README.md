# utxx - an open-source collection of C++ miscelaneous components #

This dual-licensed library provides a set of classes that
complement C++ and boost with functionality needed by many
applications.

The library is free to use under LGPLv2 license for
non-commercial projects, and has a commercial open source licence
for commercial use (contact the author for commercial
licensing details).

The components include:

* Custom allocators
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
* Logging framework
* Ultra-low latency async logger
* Math functions
* Metaprogramming functions
* Persistent blob and array
* Futex signaling
* PCAP format reader
* Throttling components
* PID file manager
* Variant / variant tree components
* Exceptions with error source location
* Reflectable enums
* Configuration framework with option validation (SCON configuration format support)

## Downloading ##
``$ git clone git@github.com:saleyn/utxx.git``

## Building ##
Make sure that you have autoconf-archive package installed:
http://www.gnu.org/software/autoconf-archive

Run:
```
$ ./bootstrap
$ ./configure --with-boost="/path/to/boost" \
	[--prefix="/target/install/path"]
$ make
$ make install      # Default install path is ./install
```

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
* Wrap at 80 characters
* Use 4-space indentation
* Expand tabs with spaces in indentations (for vi use settings: `ts=4:sw=4:et`)
* Use the following braces style:
```
if (...) {
   ...
} else {
   ...
}
```
## Author ##
* Serge Aleynikov `<saleyn at gmail dot com>`

## Contributors ##
* Dmitriy Kargapolov `<dmitriy.kargapolov at gmail dot com>`
* Leonid Timochouk   `<l.timochouk at gmail dot com>`

## LICENSE ##
* Non-commercial: GNU Lesser General Public License 2.1
* Commercial:     (contact the author for details)
