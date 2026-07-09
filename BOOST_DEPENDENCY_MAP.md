# utxx Boost Dependency Map

## Summary

utxx is split into two tiers: components that work with standard C++ only, and components that require Boost. The split is controlled by `UTXX_HAVE_BOOST` (defined when CMake finds Boost or `WITH_BOOST=ON`).

---

## Components with NO Boost dependency (always available)

### Core utilities
| Header | Description |
|--------|-------------|
| `algorithm.hpp` | Generic algorithms |
| `allocator.hpp`, `alloc_cached.hpp`, `alloc_fixed_page.hpp` | Memory allocators |
| `atomic.hpp`, `atomic_hash_array.hpp`, `atomic_hash_map.hpp` | Atomic ops and hash containers |
| `bitmap.hpp`, `bits.hpp` | Bit manipulation |
| `buffer.hpp` | Buffer utilities |
| `compiler_hints.hpp` | `likely`/`unlikely`, `__attribute__` wrappers |
| `concurrent_alloc_fixed_page.hpp` | Lock-free fixed-page allocator |
| `concurrent_mpsc_queue.hpp`, `concurrent_spsc_queue.hpp` | Lock-free queues |
| `convert.hpp` | Fast numeric ↔ string conversions |
| `cpu.hpp` | CPU affinity helpers |
| `decimal.hpp` | Fixed-point decimal |
| `delegate.hpp` | Fast delegate / callable wrapper |
| `endian.hpp` | Byte-order utilities |
| `error.hpp` | Exception and error macros |
| `event.hpp` | Event signaling (eventfd) |
| `fast_itoa.hpp` | Fast integer-to-ASCII |
| `file_reader.hpp`, `file_writer.hpp` | Binary file I/O |
| `function.hpp` | Small-buffer-optimized `std::function` |
| `futex.hpp` | Linux futex wrapper |
| `get_option.hpp` | Command-line option parser |
| `gzstream.hpp` | gzip stream (zlib only) |
| `high_res_timer.hpp` | High-resolution timer |
| `hmac.hpp` | HMAC-SHA computation |
| `iovec.hpp`, `iovector.hpp` | Scatter/gather I/O |
| `leb128.hpp` | LEB128 variable-length encoding |
| `lock.hpp` | Mutex wrappers (pthread) |
| `logger.hpp` | Async logger framework |
| `math.hpp` | Math utilities (GCD, primes, etc.) |
| `meta.hpp` | Template metaprogramming helpers |
| `noncopyable.hpp` | Non-copyable base class |
| `path.hpp` | File path utilities |
| `perf_histogram.hpp` | Performance histogram |
| `pmap.hpp` | Presence map encoding/decoding |
| `print.hpp` | Formatted printing |
| `ptree.hpp` | Boost-free `basic_ptree` drop-in used by `variant_tree` |
| `rate_throttler.hpp` | Rate limiter |
| `reactor.hpp` | I/O reactor (epoll-based) |
| `robust_mutex.hpp` | Process-robust mutex |
| `running_stat.hpp` | Running mean/variance |
| `signal_block.hpp` | Signal masking |
| `stack_container.hpp` | Stack-allocated containers |
| `string.hpp` | String utilities |
| `synch.hpp` | Synchronization primitives |
| `test_helper.hpp` | **Standalone test harness** (no Boost required) |
| `time_val.hpp` | Time value arithmetic (see note below) |
| `timestamp.hpp` | High-resolution timestamps |
| `type_traits.hpp` | Type trait utilities |
| `variant.hpp` | `std::variant`-based value type (boost-free) |
| `variant_translator.hpp` | Translator specializations for `variant_tree` |
| `variant_tree.hpp` | Variant-typed property tree (core; boost-free) |
| `variant_tree_error.hpp` | Error types for `variant_tree` |
| `variant_tree_fwd.hpp` | Forward declarations for `variant_tree` |
| `variant_tree_path.hpp` | Path type for `variant_tree` |
| `verbosity.hpp` | Runtime verbosity levels |
| `version.hpp` | Library version |

> **`time_val.hpp`** — the `boost::posix_time` conversion helpers are compiled only when
> `UTXX_HAVE_BOOST_DATE_TIME` is defined; the rest of the header is boost-free.

> **`hashmap.hpp`** — provides a `boost::unordered_map` fallback only on pre-C++11 compilers
> (`#ifndef __GXX_EXPERIMENTAL_CXX0X__`); on C++11 and later the header is boost-free.

> **`collections.hpp`** — no boost dependency (the `Boost Software License` header comment
> is not a code dependency).

### Sub-directories (no Boost)
| Path | Description |
|------|-------------|
| `io/reactor*.hpp` | Reactor I/O handlers |
| `container/` | `concurrent_fifo`, `concurrent_stack`, `clustered_map`, etc. |
| `detail/handler_traits.hpp` | Handler type traits |
| `detail/variant_tree_utils.hpp` | Internal helpers for `variant_tree` (no boost includes) |
| `detail/variant_tree_scon_parser.hpp` | SCON format parser (no boost includes) |
| `reactor/` | All reactor headers (boost-free since `cpp26` branch) |

---

## Components requiring Boost (`#ifdef UTXX_HAVE_BOOST`)

### Enum stringification — requires `boost::preprocessor`
| Header | Boost dependency |
|--------|-----------------|
| `enum.hpp` | `boost::preprocessor` (BOOST_PP_SEQ_*, BOOST_PP_VARIADIC_*) |
| `enumu.hpp` | `boost::preprocessor` |
| `enumv.hpp` | `boost::preprocessor` |
| `enum_flags.hpp` | `boost::preprocessor` |
| `detail/enum_helper.hpp` | `boost::preprocessor` |

These are compile-time macro expansions — there is no stdlib replacement for `BOOST_PP_SEQ_FOR_EACH`.

### Shared-memory / persistence — requires `boost::interprocess`
| Header | Boost dependency |
|--------|-----------------|
| `alloc_fixed_pool.hpp` | `boost::interprocess::offset_ptr` |
| `persist_array.hpp` | `boost::interprocess` (entire header) |
| `persist_blob.hpp` | `boost::interprocess` (entire header) |
| `generation_buffer.hpp` | `boost::interprocess` (shmem mode only, `UTXX_GENERATION_BUFFER_SHMEM`) |

### Async I/O — requires `boost::asio`
| Header | Boost dependency |
|--------|-----------------|
| `basic_buffer_queue.hpp` | `boost::asio` (`boost/asio/write.hpp`, `boost/system/error_code.hpp`) |
| `buffered_queue.hpp` | `boost::asio` (`boost/asio/buffer.hpp`, `boost/asio/write.hpp`, etc.) |
| `shared_buffer_queue.hpp` | `boost::asio` (entire header guarded by `UTXX_HAVE_BOOST`) |
| `io/basic_udp_receiver.hpp` | `boost::asio` (entire header) |
| `boost/async_wait_timeout.hpp` | `boost::asio` |
| `boost/iovec_range.hpp` | `boost::asio` |
| `boost/repeating_timer.hpp` | `boost::asio`, `boost::date_time`, `boost::thread`, `boost::shared_ptr` |

### Base64 — requires `boost::archive` + `boost::algorithm`
| Header | Boost dependency |
|--------|-----------------|
| `base64.hpp` | `boost/archive/iterators/binary_from_base64.hpp`, `boost/archive/iterators/base64_from_binary.hpp`, `boost/archive/iterators/transform_width.hpp`, `boost/algorithm/string.hpp` (unconditional) |

### MD5 / SHA — requires `boost::uuid`
| Header | Boost dependency |
|--------|-----------------|
| `md5.hpp` | `boost/uuid/detail/md5.hpp`, `boost/uuid/detail/sha1.hpp` (guarded by `UTXX_HAVE_BOOST`) |

### Configuration tree — requires `boost::property_tree` (parsers only)
| Header | Boost dependency |
|--------|-----------------|
| `variant_tree_parser.hpp` | `boost::property_tree` parsers (INI/XML/JSON) when `UTXX_HAVE_BOOST` defined |
| `config_validator.hpp` | `variant_tree_parser` (transitive; core tree is boost-free) |
| `dynamic_config.hpp` | `variant_tree_parser` (transitive) |
| `config_tree.hpp` | `variant_tree_parser` (transitive) |
| `detail/variant_tree_parser_impl.hpp` | `boost/property_tree/{info,xml,json,ini}_parser.hpp` (all guarded by `UTXX_HAVE_BOOST`) |

> Note: `variant_tree.hpp`, `variant_tree_fwd.hpp`, `variant_tree_error.hpp`,
> `variant_tree_path.hpp`, `variant_translator.hpp`, `detail/variant_tree_utils.hpp`,
> and `detail/variant_tree_scon_parser.hpp` no longer include any boost headers
> (the internal `ptree.hpp` replaces `boost::property_tree::basic_ptree`).
> Only the *parser* layer (`variant_tree_parser.hpp` and
> `detail/variant_tree_parser_impl.hpp`) still pulls in boost for the
> INFO/XML/JSON/INI format backends.

### Scribe logger — requires `boost::thread` + `boost::shared_ptr` (via Thrift)
| Header | Boost dependency |
|--------|-----------------|
| `logger/logger_impl_scribe.hpp` | `boost::mutex`, `boost::shared_ptr` (guarded by `UTXX_HAVE_THRIFT_H`) |

---

## Test files: Boost-dependent vs. standalone

### Always compiled (no Boost required)
`test_atomic`, `test_atomic_hash_map`, `test_bits`, `test_bitmap`, `test_clustered_map`\*\*, `test_collections`, `test_compiler_hints`, `test_decimal`, `test_delegate`, `test_endian`, `test_fast_itoa`, `test_function`, `test_futex`, `test_get_option`, `test_hashmap`, `test_high_res_timer`, `test_hmac`, `test_math`, `test_meta`, `test_mt_queue`, `test_pmap`, `test_print`, `test_rate_throttler`, `test_registrar`, `test_running_stat`, `test_signal`, `test_static_polymorphism`, `test_stream64`, `test_string`, `test_synch`, `test_thread_cached_int`, `test_thread_local`, `test_time`, `test_verbosity`, `test_utxx` (entry point)

\*\* `test_clustered_map` includes `<boost/random.hpp>` only as a C++03 fallback (`#if __cplusplus < 201103L`); on C++11 it uses `<random>`.

### Conditionally uses Boost (compiles both with and without)
| Test file | Guard | Boost used for |
|-----------|-------|---------------|
| `test_convert.cpp` | `UTXX_HAVE_BOOST_TIMER_TIMER_HPP` | `boost/timer/timer.hpp`, `boost/format.hpp` (benchmarking only) |
| `test_logger_async_file.cpp`\* | `UTXX_HAVE_BOOST` | `boost::barrier`, `boost::thread` |
| `test_robust_mutex.cpp` | `HAVE_BOOST_TIMER_TIMER_HPP` | `boost/timer/timer.hpp` (timing only) |
| `test_time_val.cpp`\* | `UTXX_HAVE_BOOST_DATE_TIME` | `boost::date_time` conversions |
| `test_timestamp.cpp`\* | `UTXX_HAVE_BOOST` | `boost::posix_time`, `boost::thread::barrier` |
| `test_logger.cpp` | `UTXX_HAVE_BOOST` | Logger Boost-dependent back-ends |

\* Core test logic compiles and runs without Boost; Boost adds extra test cases.

### Requires Boost (`#ifdef UTXX_HAVE_BOOST`)
| Test file | Reason |
|-----------|--------|
| `test_enum.cpp` | Tests `UTXX_ENUM` which requires `boost::preprocessor` |
| `test_variant.cpp` | Uses `boost::mpl`, `boost::property_tree`, `boost::foreach` |
| `test_variant_tree_scon_parser.cpp` | Tests SCON parser (variant_tree; boost for INI/XML/JSON parsers) |
| `test_config_validator.cpp` | Tests config validator; uses `boost::algorithm::string` |
| `test_dynamic_config.cpp` | Tests dynamic config built on `variant_tree` |
| `test_persist_array.cpp` | Tests `persist_array` which requires `boost::interprocess` |
| `test_persist_blob.cpp` | Tests `persist_blob` which requires `boost::interprocess` |
| `test_basic_udp_receiver.cpp` | Tests UDP receiver built on `boost::asio` |
| `test_concurrent_fifo.cpp` | Uses `boost::shared_ptr`, `boost::thread` |
| `test_concurrent_stack.cpp` | Uses `boost::thread::barrier` |
| `test_concurrent_update.cpp` | Uses `boost::barrier`, `boost::thread` |
| `test_file_reader.cpp` | Uses `boost::assign` |
| `test_logger_scribe.cpp` | Uses `boost::property_tree` |
| `test_logger_syslog.cpp` | Uses `boost::property_tree` |
| `test_multi_file_async_logger.cpp` | Uses `boost::barrier`, `boost::thread`, `boost::pool` |
| `test_call_speed.cpp` | Uses `boost::variant`, `boost::posix_time` |
| `test_shared_queue.cpp` | Exercises `shared_buffer_queue` which requires `boost::asio` |

---

## Build configurations

```bash
# Full build with Boost (default)
cmake -DWITH_BOOST=ON  -B build && cmake --build build

# Minimal build without Boost
cmake -DWITH_BOOST=OFF -B build-noboost && cmake --build build-noboost
# → Boost-dependent headers/tests are compiled away via #ifdef UTXX_HAVE_BOOST
```

### What works without Boost
- All core utilities (string, math, time, atomic, containers, logger framework)
- `variant_tree` core (tree, path, error, SCON parser) — no boost required
- The `test_helper.hpp` standalone test harness
- ~65 of ~90 test cases

### What requires Boost
- Enum stringification macros (`UTXX_ENUM`, `UTXX_ENUMV`, `UTXX_ENUM_FLAGS`)
- Shared-memory persistence (`persist_array`, `persist_blob`)
- Async I/O wrappers (`basic_buffer_queue`, `buffered_queue`, `shared_buffer_queue`, `basic_udp_receiver`)
- Base64 encode/decode (`base64.hpp`)
- MD5/SHA utilities (`md5.hpp`)
- `variant_tree` format parsers — INFO, XML, JSON, INI back-ends (`variant_tree_parser.hpp`)
- ~20 test files that exercise the above
