![Banner](https://github.com/saleyn/utxx/blob/master/assets/utxx-banner.png?raw=true)

`utxx` is an open-source collection of C++17 utility components for high-performance,
low-latency systems. It is dual-licensed under LGPLv2 (non-commercial) and a commercial
open-source license (contact the author for details).

---

## Table of Contents

- [Table of Contents](#table-of-contents)
- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
  - [CMake options](#cmake-options)
  - [`.cmake-args` customization](#cmake-args-customization)
  - [Building without Boost](#building-without-boost)
- [Component Reference](#component-reference)
  - [Logging](#logging)
  - [Reflectable Enums](#reflectable-enums)
  - [Configuration / Variant Tree](#configuration--variant-tree)
  - [Concurrent Containers](#concurrent-containers)
  - [Memory Allocators](#memory-allocators)
  - [I/O Reactor](#io-reactor)
  - [Async I/O Wrappers (Boost.Asio)](#async-io-wrappers-boostasio)
  - [Time \& Timestamps](#time--timestamps)
  - [String \& Conversion](#string--conversion)
  - [Math \& Algorithms](#math--algorithms)
  - [Hashing \& Cryptography](#hashing--cryptography)
  - [Persistent Storage](#persistent-storage)
  - [File \& Path Utilities](#file--path-utilities)
  - [Networking](#networking)
  - [Delegates \& Signals](#delegates--signals)
  - [Synchronization Primitives](#synchronization-primitives)
  - [Template Metaprogramming](#template-metaprogramming)
  - [Miscellaneous Utilities](#miscellaneous-utilities)
- [Boost Dependency](#boost-dependency)
- [Contributing](#contributing)
  - [Commit message style](#commit-message-style)
  - [Code style](#code-style)
- [Author \& License](#author--license)

---

## Features

- **Ultra-low latency async logger** — ~300 ns average critical-path latency, pluggable back-ends (console, file, async file, syslog, scribe)
- **Reflectable enums** — name ↔ value ↔ string at compile time, no runtime tables
- **Configuration tree** — typed property tree with SCON, XML, JSON, and INI parsers; schema validation
- **Lock-free concurrent containers** — MPSC/SPSC queues, stack, FIFO, clustered map, priority queue
- **I/O reactor** — epoll (Linux) / kqueue (macOS) event loop; timers, signals, eventfd, async file reads via libaio
- **Custom allocators** — fixed-page, cached, pool, concurrent-page allocators
- **Fast numeric conversion** — integers and floats to/from strings without locale overhead
- **Time utilities** — `time_val` arithmetic, high-resolution timestamps, rate throttling
- **Persistent shared-memory** — `persist_array` and `persist_blob` backed by `boost::interprocess`
- **PCAP reader/writer**, **LEB128**, **Base64**, **xxHash**, **MD5/SHA256/HMAC**
- **Template metaprogramming** helpers, type traits, compile-time math
- Header-only where possible; split Boost/no-Boost tiers via `UTXX_HAVE_BOOST`

---

## Requirements

| Requirement | Notes |
|-------------|-------|
| C++17 compiler | GCC ≥ 7, Clang ≥ 5 |
| CMake ≥ 3.5 | |
| Boost ≥ 1.55 | Optional; required for enums, async I/O, shared-memory, config parsers |
| libaio | Optional; required for async file reads on Linux |
| Python 3 (or 2) + lxml | Required only for config schema generation |

---

## Building

```bash
# Clone
git clone git@github.com:saleyn/utxx.git
cd utxx

# Bootstrap and build (GCC, debug by default)
make bootstrap [toolchain=gcc|clang] [build=Debug|Release] \
               [generator=make|ninja] [prefix=/usr/local] [verbose=true]
make [verbose=true]
make install        # default prefix: /usr/local
```

To build both debug and release libraries (`libutxx_d.so` + `libutxx.so`):

```bash
make bootstrap toolchain=gcc build=debug
make src/libutxx_d.so src/libutxx_d.a   # debug
make rebootstrap build=release
make                                     # release
make install
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `WITH_BOOST` | `ON` | Enable Boost-dependent components |
| `WITH_THRIFT` | `OFF` | Build the Scribe logger back-end |
| `WITH_ENUM_SERIALIZATION` | `OFF` | Add serialization support to reflectable enums |
| `VERBOSE` | `OFF` | Verbose CMake/make output |

### `.cmake-args` customization

Create `.cmake-args.${HOSTNAME}` (or `.cmake-args` for all hosts) to override build/install
locations and dependency paths:

```
DIR:BUILD=/tmp/@PROJECT@/build
DIR:INSTALL=/opt/pkg/@PROJECT@/@VERSION@
ENV:BOOST_ROOT=/opt/pkg/boost/current
ENV:BOOST_LIBRARYDIR=/opt/pkg/boost/current/gcc/lib
PKG_ROOT_DIR=/opt/pkg
```

Macros available: `@PROJECT@`, `@VERSION@`, `@BUILD@`, `${ENV_VAR}`.

### Building without Boost

```bash
cmake -DWITH_BOOST=OFF -B build-noboost && cmake --build build-noboost
```

All core utilities, the variant tree (except format parsers), and ~65 of ~90 tests
compile and run without Boost. See [BOOST_DEPENDENCY_MAP.md](BOOST_DEPENDENCY_MAP.md) for the
full breakdown.

---

## Component Reference

### Logging

```cpp
#include <utxx/logger/logger.hpp>
```

A lightweight, pluggable logging framework with synchronous and asynchronous back-ends.
Log calls are `printf`-style for speed; a category string is optional.

**Back-ends:**

| Back-end | Header | Description |
|----------|--------|-------------|
| Console | `logger_impl_console.hpp` | Writes to stdout/stderr |
| File | `logger_impl_file.hpp` | Synchronous file writer |
| Async File | `async_file_logger.hpp` | Lock-free async writer, ~300 ns critical path |
| Syslog | `logger_impl_syslog.hpp` | System log |
| Scribe | `logger_impl_scribe.hpp` | Thrift/Scribe sink (requires `WITH_THRIFT`) |

**Log levels:** `TRACE4 TRACE3 TRACE2 TRACE1 TRACE DEBUG INFO NOTICE WARNING ERROR FATAL ALERT`

**Macros:**

```cpp
UTXX_LOG_INFO ("Connected to %s:%d", host, port);
UTXX_LOG_DEBUG("Value: %d", val);
UTXX_LOG_ERROR("Failed: %s", strerror(errno));

// With category:
UTXX_CLOG_WARNING(my_category, "Retry %d/%d", attempt, max);

// Generic (level as runtime or compile-time value):
UTXX_LOG(utxx::LEVEL_INFO, "message %s", str);
```

**Async logger usage:**

```cpp
#include <utxx/multi_file_async_logger.hpp>

utxx::multi_file_async_logger logger;
auto stream = logger.open_stream("app.log");
logger.start();

logger.write(stream, utxx::LEVEL_INFO, "Hello %s\n", "world");
logger.stop();
```

---

### Reflectable Enums

```cpp
#include <utxx/enum.hpp>       // UTXX_ENUM
#include <utxx/enumv.hpp>      // UTXX_ENUMV  (with associated values)
#include <utxx/enumu.hpp>      // UTXX_ENUMU  (unscoped, global names)
#include <utxx/enum_flags.hpp> // UTXX_ENUM_FLAGS (bitmask flags)
```

Requires `boost::preprocessor` (`WITH_BOOST=ON`).

**`UTXX_ENUM` — basic reflectable enum:**

```cpp
UTXX_ENUM(Color, char,          // storage type
    Red,
    (Green, "lime"),             // optional string value label
    Blue
);

Color c = Color::from_name("Red");    // lookup by name
Color d = Color::from_string("lime"); // lookup by value label
std::cout << c.name()   << '\n';      // "Red"
std::cout << c.value()  << '\n';      // "Red"  (no label → defaults to name)
std::cout << d.name()   << '\n';      // "Green"
std::cout << d.value()  << '\n';      // "lime"
std::cout << int(c)     << '\n';      // 0
```

Options for the second argument:

```
Type                             — storage type, first item = 0
(Type, DefVal)                   — undefined item = DefVal
(Type, UndefName, DefVal)        — custom undefined item name
(Type, UndefName, DefVal, First) — custom start value
```

**`UTXX_ENUMV` — enum with distinct code, name, and string value per item:**

```cpp
UTXX_ENUMV(Status, int,
    (OK,      0, "success")
    (TIMEOUT, 1, "timed out")
    (ERROR,   2, "error")
);
```

**`UTXX_ENUM_FLAGS` — bitmask enum:**

```cpp
UTXX_ENUM_FLAGS(Perm, uint8_t, Read, Write, Execute);
Perm p = Perm::Read | Perm::Write;
bool can_read = p.has(Perm::Read);   // true
```

---

### Configuration / Variant Tree

```cpp
#include <utxx/variant_tree.hpp>
#include <utxx/variant_tree_parser.hpp>   // file/stream I/O
#include <utxx/config_validator.hpp>      // schema validation
```

A typed property tree where each node holds a `utxx::variant` (int/bool/double/string).
The core tree is boost-free; the format parsers (XML, JSON, INI, INFO) require Boost.

**SCON format** (utxx's own config format — no Boost needed):

```
# comment
server {
    host = "localhost"
    port = 8080
    debug = true
}
```

**Reading a config file:**

```cpp
utxx::variant_tree cfg;
utxx::variant_tree_parser::read_scon("app.conf", cfg);  // SCON (always available)
utxx::variant_tree_parser::read_xml ("app.xml",  cfg);  // XML  (needs Boost)
utxx::variant_tree_parser::read_json("app.json", cfg);  // JSON (needs Boost)

std::string host = cfg.get<std::string>("server.host");
int         port = cfg.get<int>        ("server.port");
```

**Schema validation (`config_validator`):**

Validates a `variant_tree` against an XML schema that describes expected keys,
types, required/optional status, and default values.

---

### Concurrent Containers

All containers are lock-free or wait-free unless noted.

| Header | Container | Description |
|--------|-----------|-------------|
| `concurrent_mpsc_queue.hpp` | `concurrent_mpsc_queue<T>` | Multi-producer single-consumer queue |
| `concurrent_spsc_queue.hpp` | `concurrent_spsc_queue<T>` | Single-producer single-consumer queue |
| `container/concurrent_stack.hpp` | `concurrent_stack<T>` | Lock-free Treiber stack |
| `container/base_fifo.hpp` | `concurrent_fifo<T>` | Lock-free FIFO |
| `container/concurrent_priority_queue.hpp` | `concurrent_priority_queue<T>` | Lock-free priority queue |
| `container/concurrent_array.hpp` | `concurrent_array<T,N>` | Fixed-size wait-free array |
| `container/clustered_map.hpp` | `clustered_map<K,V>` | Cache-friendly sorted map with cluster buckets |
| `ring_buffer.hpp` | `ring_buffer<T>` | Wait-free circular buffer (1-reader N-writer) |
| `mt_queue.hpp` | `mt_queue<T>` | Mutex-based thread-safe queue |
| `atomic_hash_map.hpp` | `atomic_hash_map<K,V>` | Lock-free hash map (Folly-based, optional) |

```cpp
#include <utxx/concurrent_mpsc_queue.hpp>

utxx::concurrent_mpsc_queue<int> q(1024);
q.enqueue(42);

int val;
while (q.dequeue(val))
    process(val);
```

---

### Memory Allocators

| Header | Class | Description |
|--------|-------|-------------|
| `allocator.hpp` | — | `std::allocator`-compatible wrapper utilities |
| `alloc_cached.hpp` | `alloc_cached<T>` | Thread-local caching allocator |
| `alloc_fixed_page.hpp` | `alloc_fixed_page<N>` | Fixed-page allocator (non-concurrent) |
| `concurrent_alloc_fixed_page.hpp` | `concurrent_alloc_fixed_page<N>` | Fixed-page allocator with atomic page allocation |
| `alloc_fixed_pool.hpp` | `alloc_fixed_pool<T>` | Pool allocator backed by `boost::interprocess::offset_ptr` |

---

### I/O Reactor

```cpp
#include <reactor/reactor.hxx>
```

Standalone, dependency-free (no Boost) event reactor using epoll on Linux and kqueue on
macOS/BSD.  See [REACTOR.md](REACTOR.md) for the full API reference.

**Supported event sources:**

| Source | Add method | Handler signature |
|--------|-----------|-------------------|
| Buffered I/O | `Add(name, fd, on_read, on_write, on_err)` | `int(FdInfo&, dynamic_io_buffer&)` |
| Raw I/O | `Add(name, fd, on_io, on_err, …, events)` | `void(FdInfo&, IOType, uint32_t)` |
| Async file | `AddFile(name, path, on_read, on_err)` | `int(FdInfo&, dynamic_io_buffer&)` |
| Eventfd | `AddEvent(name, on_event, on_err)` | `void(FdInfo&, long)` |
| Timer | `AddTimer(name, initial_ms, interval_ms, on_timer, on_err)` | `void(FdInfo&, long)` |
| Signal | `AddSignal(name, mask, on_signal, on_err)` | `void(FdInfo&, int signo, int code)` |
| UDS server | `AddUDSListener(name, path, on_accept, on_err)` | `bool(FdInfo&, const char*, int fd)` |

**Quick example:**

```cpp
#include <reactor/reactor.hxx>

utxx::Reactor r("demo");

r.AddTimer("tick", 0, 500, [](utxx::FdInfo&, long) {
    printf("tick\n");
}, nullptr);

while (true) r.Wait(100);
```

---

### Async I/O Wrappers (Boost.Asio)

These headers require `WITH_BOOST=ON`.

| Header | Description |
|--------|-------------|
| `basic_buffer_queue.hpp` | `basic_buffer_queue<Socket>` — async write queue over any Asio socket |
| `buffered_queue.hpp` | `buffered_queue<Socket>` — async read/write buffer pair |
| `shared_buffer_queue.hpp` | `shared_const_buffer` + enqueue API for shared-ownership async writes |
| `io/basic_udp_receiver.hpp` | `basic_udp_receiver<Handler>` — async UDP datagram receiver |
| `boost/repeating_timer.hpp` | `repeating_timer` — Asio deadline timer that auto-restarts |
| `boost/async_wait_timeout.hpp` | Async wait with timeout helper |
| `boost/iovec_range.hpp` | `iovec_range` — Asio buffer sequence over `iovec` arrays |

---

### Time & Timestamps

```cpp
#include <utxx/time_val.hpp>   // time_val
#include <utxx/timestamp.hpp>  // high-res stamp formatting
#include <utxx/time.hpp>       // time zone / calendar helpers
```

**`time_val`** — `struct timeval` wrapper with arithmetic operators:

```cpp
utxx::time_val now  = utxx::now_utc();
utxx::time_val then = now + utxx::time_val(1, 500000); // +1.5 s
double secs  = now.seconds();
long   usecs = now.microseconds();
```

`boost::posix_time` conversion methods are available when `UTXX_HAVE_BOOST_DATE_TIME` is defined.

**`timestamp`** — nanosecond-precision UTC/local stamps:

```cpp
#include <utxx/timestamp.hpp>

char buf[32];
utxx::timestamp::write_time(buf, sizeof(buf),
    utxx::TIME_WITH_USEC, utxx::now_utc());
// buf → "2024-01-15 12:34:56.123456"
```

**`rate_throttler`:**

```cpp
#include <utxx/rate_throttler.hpp>

utxx::basic_rate_throttler<16> throttle;
throttle.init(100, 1000); // 100 events per 1000 ms
if (throttle.add())
    do_event();           // only when under the rate limit
```

---

### String & Conversion

```cpp
#include <utxx/string.hpp>     // string utilities
#include <utxx/convert.hpp>    // numeric ↔ string
#include <utxx/fast_itoa.hpp>  // fastest integer → ASCII
#include <utxx/print.hpp>      // fmt-style formatting
```

**`convert.hpp`** — locale-free, zero-allocation conversions:

```cpp
char buf[32];
char* end = utxx::itoa(buf, 12345);           // integer → string
char* end = utxx::ftoa_left(buf, 3.14, 4);   // float  → string, 4 decimal places

int n; double d;
const char* p = "123";
utxx::fast_atoi<int>(p, p+3, n);             // string → integer
utxx::atof(p, p+6, d);                       // string → double
```

**`string.hpp`** — split, join, trim, case conversion, wildcard match, URL encode/decode,
`to_string()` for enums, pretty-print of durations, etc.

**`print.hpp`** — lightweight type-safe formatting into fixed buffers:

```cpp
char buf[64];
utxx::print(buf, "Value: ", 42, " pi=", 3.14);
```

---

### Math & Algorithms

```cpp
#include <utxx/math.hpp>
#include <utxx/bits.hpp>
#include <utxx/bitmap.hpp>
#include <utxx/leb128.hpp>
#include <utxx/running_stat.hpp>
#include <utxx/polynomial.hpp>
#include <utxx/perf_histogram.hpp>
```

**`math.hpp`:**

```cpp
utxx::math::gcd(12, 8);        // 4
utxx::math::lcm(4, 6);         // 12
utxx::math::is_prime(97);      // true
utxx::math::next_prime(100);   // 101
utxx::math::upper_pow2(100);   // 128  (next power of 2)
utxx::math::log2(64);          // 6
```

**`bits.hpp` / `bitmap.hpp`:**

```cpp
utxx::bitmap<64> bm;
bm.set(5); bm.set(10);
for (int bit : bm) printf("%d\n", bit);  // 5, 10
```

**`running_stat`** — online mean, variance, min, max:

```cpp
utxx::basic_running_stat<double> stat;
stat.add(1.0); stat.add(2.0); stat.add(3.0);
printf("mean=%.1f stddev=%.3f\n", stat.mean(), stat.stddev());
```

**`perf_histogram`** — lock-free latency histogram with configurable buckets.

**`polynomial`** — polynomial curve fitting and evaluation.

---

### Hashing & Cryptography

| Header | Description |
|--------|-------------|
| `xxhash.hpp` | xxHash — extremely fast non-cryptographic hash |
| `hmac.hpp` | HMAC-SHA1/SHA256 |
| `sha256.hpp` | SHA-256 (standalone, no Boost) |
| `md5.hpp` | MD5 and SHA1 via `boost::uuid` (requires `UTXX_HAVE_BOOST`) |
| `base64.hpp` | Base64 encode/decode (requires Boost archive iterators) |

```cpp
#include <utxx/xxhash.hpp>
uint64_t h = utxx::xxhash64("hello world", 11, /*seed=*/0);

#include <utxx/hmac.hpp>
std::string sig = utxx::hmac_sha256("secret", "message");
```

---

### Persistent Storage

```cpp
#include <utxx/persist_array.hpp>    // requires boost::interprocess
#include <utxx/persist_blob.hpp>     // requires boost::interprocess
#include <utxx/generation_buffer.hpp>
```

**`persist_array<T, N>`** — memory-mapped array shared across processes:

```cpp
utxx::persist_array<int, 1024> arr;
arr.open("data.bin", utxx::READ_WRITE);
arr[0] = 42;
arr.flush();
```

**`persist_blob<T>`** — typed memory-mapped single record:

```cpp
utxx::persist_blob<MyStruct> blob;
blob.open("record.bin", utxx::READ_WRITE);
blob->field = value;
```

**`generation_buffer`** — double-buffered data with a generation counter for wait-free
single-writer multiple-reader access.  Supports both in-process and shared-memory modes.

---

### File & Path Utilities

```cpp
#include <utxx/path.hpp>        // path manipulation
#include <utxx/file_reader.hpp> // typed binary file reader
#include <utxx/file_writer.hpp> // typed binary file writer
#include <utxx/gzstream.hpp>    // gzip stream (zlib)
#include <utxx/pcap.hpp>        // PCAP file format
#include <utxx/pidfile.hpp>     // PID file management
```

**`path.hpp`:**

```cpp
std::string home = utxx::path::home();
std::string full = utxx::path::expand("~/data/%Y%m%d.log"); // strftime expansion
bool exists      = utxx::path::file_exists("/tmp/app.lock");
std::string dir  = utxx::path::dirname("/tmp/app.log");     // "/tmp"
```

**`file_reader` / `file_writer`** — iterate over binary records with header + payload:

```cpp
utxx::file_reader<MyHeader> reader("data.bin");
while (reader.read()) {
    const MyHeader& hdr = reader.header();
    const char*     dat = reader.data();
}
```

**`pcap`** — write network captures in Wireshark-compatible PCAP format.

**`pidfile`** — creates `/var/run/<name>.pid` on construction; removes it on destruction.

---

### Networking

```cpp
#include <utxx/url.hpp>     // URL / connection-string parsing
#include <utxx/iovec.hpp>   // iovec helpers
#include <utxx/iovector.hpp>// iovec_vector
```

**`url.hpp`** — parse connection strings:

```cpp
utxx::addr_info info;
utxx::parse_addr("tcp://127.0.0.1:8080", info);
// info.proto = "tcp", info.addr = "127.0.0.1", info.port = 8080
```

Supported schemes: `tcp://`, `udp://`, `uds://`, `file://`, `self://`.

---

### Delegates & Signals

```cpp
#include <utxx/delegate.hpp>      // fast_delegate (FastDelegate by Don Clugston)
#include <utxx/function.hpp>      // small-buffer-optimized std::function
#include <utxx/delegate/event.hpp>// signal/slot without Boost.Signals2
```

**`function<R(Args...)>`** — drop-in for `std::function` with a configurable inline
buffer that avoids heap allocation for small callables:

```cpp
utxx::function<int(int,int)> f = [](int a, int b) { return a+b; };
int result = f(3, 4);  // 7
```

**`delegate`** — zero-overhead member/free-function pointer wrapper, no virtual dispatch:

```cpp
utxx::delegate<void(int)> d;
d.bind<&MyClass::handler>(&obj);
d(42);
```

---

### Synchronization Primitives

| Header | Description |
|--------|-------------|
| `futex.hpp` | Linux futex wrapper for low-latency blocking |
| `lock.hpp` | Scoped lock helpers over `pthread_mutex_t` |
| `robust_mutex.hpp` | Process-robust mutex (survives owner crash) |
| `synch.hpp` | Barrier, semaphore, event abstractions |
| `event.hpp` | `eventfd`-based single event (Linux) |
| `thread_local.hpp` | `thr_local<T>` — 4× faster than `boost::thread_specific_ptr` |
| `thread_cached_int.hpp` | `thread_cached_int<T>` — thread-local sharded counter |
| `signal_block.hpp` | RAII signal mask management |
| `singleton.hpp` | Thread-safe singleton template |

---

### Template Metaprogramming

```cpp
#include <utxx/meta.hpp>           // type lists, integral_sequence, etc.
#include <utxx/meta_tuple.hpp>     // tuple iteration / index_of
#include <utxx/type_traits.hpp>    // is_container, has_const_iterator, etc.
#include <utxx/compiler_hints.hpp> // likely/unlikely, UTXX_ASSUME, __attribute__
#include <utxx/typeinfo.hpp>       // type_to_string<T>()
```

**`compiler_hints.hpp`:**

```cpp
if (UTXX_LIKELY(x > 0))      process(x);
if (UTXX_UNLIKELY(x == -1))  handle_error();
UTXX_ASSUME(ptr != nullptr);  // elided in release
```

---

### Miscellaneous Utilities

| Header | Description |
|--------|-------------|
| `error.hpp` | `utxx::runtime_error` with source location; `UTXX_THROW_RUNTIME_ERROR(...)` |
| `verbosity.hpp` | Runtime verbosity level (global, per-category) |
| `version.hpp` | `UTXX_VERSION` string and integer constants |
| `noncopyable.hpp` | `utxx::noncopyable` base class |
| `scope_exit.hpp` | `utxx::scope_exit(lambda)` — RAII cleanup |
| `get_option.hpp` | `get_option(argc, argv, "--flag", default)` — command-line parsing |
| `short_vector.hpp` | `short_vector<T,N>` — small-buffer-optimized `std::vector` |
| `decimal.hpp` | Fixed-point decimal with configurable precision |
| `pmap.hpp` | Presence-map bit-field encoding (FIX/FAST protocol helper) |
| `stream_io.hpp` | `read_line`, `write_all` with EINTR retry |
| `os.hpp` | `cpu_count()`, `page_size()`, memory stats |
| `cpu.hpp` | `set_cpu_affinity()`, `get_cpu_affinity()` |
| `name.hpp` | `name<N>` — fixed-length char array with string semantics |
| `nchar.hpp` | Null-padded fixed-length character field |
| `collections.hpp` | `unordered_map`, `map` type aliases with consistent API |
| `hashmap.hpp` | `basic_hash_map<K,V,Hash>` — `std::unordered_map` wrapper |
| `registrar.hpp` | IoC registrar — named factory/instance registry |
| `unordered_map_with_ttl.hpp` | Hash map with TTL eviction on insert |
| `test_helper.hpp` | Standalone test harness (no Boost required) |

---

## Boost Dependency

utxx has two tiers: a boost-free core and optional Boost-dependent components.
The split is controlled by `UTXX_HAVE_BOOST` (`-DWITH_BOOST=ON/OFF`).

**Requires Boost:**
- Enum macros (`enum.hpp`, `enumu.hpp`, `enumv.hpp`, `enum_flags.hpp`) — `boost::preprocessor`
- Persistent storage (`persist_array`, `persist_blob`) — `boost::interprocess`
- Async I/O wrappers (`basic_buffer_queue`, `buffered_queue`, `io/basic_udp_receiver`) — `boost::asio`
- Config format parsers — `boost::property_tree` (XML/JSON/INI/INFO)
- Base64 (`base64.hpp`) — `boost::archive`, `boost::algorithm`
- MD5/SHA1 (`md5.hpp`) — `boost::uuid`

**Always available (no Boost):**
- All core utilities, reactor, variant tree (core + SCON parser), time, string, math, containers, allocators

See [BOOST_DEPENDENCY_MAP.md](BOOST_DEPENDENCY_MAP.md) for the complete header-by-header breakdown.

---

## Contributing

### Commit message style

```
One line summary (< 50 characters)

Longer description (wrap at 72 characters). Explain WHY,
not what. Use imperative present tense (fix, add, change).
```

### Code style

- 4-space indentation, no tabs, 80-column wrap
- K&R brace style
- `class my_class`, `int this_my_fun()` — snake_case throughout
- Function arguments: `a_` prefix (e.g. `int a_count`)
- Member variables: `m_` prefix (e.g. `int m_count`)
- Static variables: `s_` prefix (e.g. `static int s_count`)
- First line of every file: `// vim:ts=4:sw=4:et`

---

## Author & License

**Author:** Serge Aleynikov `<saleyn at gmail dot com>`
(see [AUTHORS.md](AUTHORS.md) for contributors)

**License:**
- Non-commercial: GNU Lesser General Public License 2.1
- Commercial: contact the author for details

**Commit notifications:** `github-utxx at googlegroups dot com`
