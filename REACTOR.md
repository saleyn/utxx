# Reactor — I/O Event Reactor Reference

A standalone, cross-platform epoll/kqueue event reactor in `namespace utxx`.
No Boost, no external dependencies beyond the C++17 standard library and Linux/macOS kernel headers.

**Platforms:** Linux (epoll + eventfd + timerfd + signalfd + libaio) · macOS / BSD (kqueue)

**Build:**
```sh
make reactor          # compile all objects and test binaries
make reactor-tests    # compile + run, prints "Total: N passed, N failed"
make reactor-clean    # wipe .build/reactor/
```

**Headers to include:**
```cpp
#include <reactor/reactor.hxx>          // Reactor + FdInfo (full inline impl)
#include <reactor/reactor_platform.hpp> // platform types and free functions only
```

---

## Table of Contents

1. [Quick-start examples](#quick-start-examples)
2. [Reactor class](#reactor-class)
3. [FdInfo class](#fdinfo-class)
4. [Handler types](#handler-types)
5. [Enumerations](#enumerations)
6. [AIOReader class](#aioreader-class)
7. [Platform abstraction](#platform-abstraction)
8. [Utility functions](#utility-functions)
9. [Error handling](#error-handling)
10. [Logging](#logging)

---

## Quick-start examples

### TCP / Unix-domain socket server

```cpp
#include <reactor/reactor.hxx>

utxx::Reactor r("my-server");

// Add a UDS listener; accept() fires for each new connection
r.AddUDSListener(
    "uds-server", "/tmp/my.sock",
    [&](utxx::FdInfo& fi, const char* cli_path, int cli_fd) -> bool {
        // Return true to keep cli_fd (reactor takes ownership),
        // false to close it immediately.
        r.Add("client", cli_fd,
            [](utxx::FdInfo& fi, utxx::dynamic_io_buffer& buf) -> int {
                // process buf.rd_ptr()[0..buf.size()-1]
                buf.crunch();          // consume data
                return 0;
            },
            nullptr,                   // write handler (optional)
            nullptr);                  // error handler (optional)
        return true;
    },
    nullptr);                          // error handler

while (true)
    r.Wait(100);                       // 100 ms timeout
```

### Event (wakeup from another thread)

```cpp
utxx::Reactor r("demo");

auto& fi = r.AddEvent(
    "wakeup",
    [](utxx::FdInfo& fi, long value) {
        printf("got event, count=%ld\n", value);
    },
    nullptr);

// From another thread, write to the eventfd to wake the reactor:
uint64_t one = 1;
utxx::reactor_eventfd_write(utxx::reactor_eventfd_write_fd(fi.FD()), one);

r.Wait(1000);
```

### Periodic timer

```cpp
utxx::Reactor r("timer-demo");

r.AddTimer(
    "tick",
    500,   // initial delay ms
    250,   // repeat interval ms
    [](utxx::FdInfo& fi, long expirations) {
        printf("tick! missed=%ld\n", expirations - 1);
    },
    nullptr);

for (int i = 0; i < 10; ++i)
    r.Wait(300);
```

### Raw I/O (edge-triggered, custom events)

```cpp
int sock = /* connected socket */;
utxx::Reactor r("raw");

r.Add("raw-sock", sock,
    [](utxx::FdInfo& fi, utxx::IOType tp, uint32_t events) {
        if (utxx::Reactor::IsReadable(events)) { /* read */ }
        if (utxx::Reactor::IsWritable(events)) { /* write */ }
        if (utxx::Reactor::IsError(events))    { /* close */ }
    },
    nullptr,                            // error handler
    nullptr,                            // opaque
    REACTOR_EV_IN | REACTOR_EV_OUT | REACTOR_EV_ET);

r.Wait(500);
```

### Signal handling

```cpp
utxx::Reactor r("signals");

sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGTERM);
sigaddset(&mask, SIGHUP);

r.AddSignal("sigs", mask,
    [](utxx::FdInfo& fi, int signo, int code) {
        printf("signal %d received (code=%d)\n", signo, code);
    },
    nullptr);

r.Wait(-1);  // block indefinitely
```

### Async file read (Linux only)

```cpp
utxx::Reactor r("file-reader");

r.AddFile(
    "data", "/path/to/data.bin",
    [](utxx::FdInfo& fi, utxx::dynamic_io_buffer& buf) -> int {
        // entire file (or chunk) is in buf
        printf("read %zu bytes\n", buf.size());
        return 0;
    },
    [](utxx::FdInfo& fi, utxx::IOType tp, const std::string& err,
       utxx::src_info&&) {
        if (tp == utxx::IOType::EndOfFile) printf("done.\n");
        else                               fprintf(stderr, "error: %s\n", err.c_str());
    });

while (r.Wait(100) >= 0);
```

---

## Reactor class

```cpp
class Reactor;
```

Central event loop. Owns a `reactor_handle_t` (epoll fd / kqueue fd) and a vector of `FdInfo` slots indexed by fd number.

### Constructor

```cpp
Reactor(std::string const& a_ident,
        int               a_debug    = 0,
        reactor_handle_t  a_epoll_fd = -1,
        int               a_max_fds  = 128);
```

| Parameter | Description |
|-----------|-------------|
| `a_ident` | Human-readable name used in log messages |
| `a_debug` | Debug verbosity level (0 = off) |
| `a_epoll_fd` | Provide an existing multiplexer handle, or -1 to create one |
| `a_max_fds` | Initial fd-slot vector capacity (grows automatically) |

### Lifecycle / configuration

| Method | Description |
|--------|-------------|
| `const std::string& Ident() const` | Returns reactor name |
| `void Ident(const std::string&)` | Sets reactor name |
| `int Debug() const` | Returns debug level |
| `void Debug(int level)` | Sets debug level |
| `void SetIdle(const IdleHandler&)` | Callback invoked on every `Wait()` timeout |
| `void SetLogger(const Logger&)` | Override default stderr logger |
| `reactor_handle_t EPollFD() const` | Returns the underlying multiplexer handle |

### Event sources

#### Add — buffered I/O

```cpp
FdInfo& Add(
    std::string const&    a_name,
    int                   a_fd,
    RWIOHandler const&    a_on_read,
    RWIOHandler const&    a_on_write,
    ErrHandler  const&    a_on_error,
    void*                 a_instance        = nullptr,
    void*                 a_opaque          = nullptr,
    int                   a_rd_bufsz        = 0,
    int                   a_wr_bufsz        = 0,
    dynamic_io_buffer*    a_wr_buf          = nullptr,
    ReadSizeEstim         a_read_at_least   = nullptr,
    TriggerT              a_trigger         = EDGE_TRIGGERED);
```

Registers `a_fd` for buffered I/O.  Incoming data is accumulated in a `dynamic_io_buffer`; `a_on_read` is called with the filled buffer.  `a_on_write` may be `nullptr`.

#### Add — raw I/O

```cpp
FdInfo& Add(
    const std::string&  a_name,
    int                 a_fd,
    RawIOHandler const& a_on_io,
    ErrHandler   const& a_on_error,
    void*               a_opaque  = nullptr,
    uint32_t            a_events  = REACTOR_EV_IN|REACTOR_EV_RDHUP|
                                    REACTOR_EV_OUT|REACTOR_EV_ET,
    size_t              a_rd_bufsz = 0);
```

Delivers raw `(FdInfo&, IOType, uint32_t events)` callbacks without buffering.

#### AddFile — async file reader (Linux)

```cpp
FdInfo& AddFile(
    std::string const&  a_name,
    std::string const&  a_filename,
    FileHandler  const& a_on_read,
    ErrHandler   const& a_on_error,
    void*               a_instance      = nullptr,
    void*               a_opaque        = nullptr,
    size_t              a_rd_bufsz      = 5 * 1024 * 1024,
    ReadSizeEstim       a_read_at_least = nullptr,
    TriggerT            a_trigger       = LEVEL_TRIGGERED);
```

Uses libaio + eventfd for zero-copy async reads.  On macOS throws `ENOSYS`.

#### AddEvent — eventfd wakeup

```cpp
FdInfo& AddEvent(
    std::string const&   a_name,
    EventHandler  const& a_on_event,
    ErrHandler    const& a_on_error,
    void*                a_opaque = nullptr);
```

Creates an eventfd (Linux) or pipe (macOS).  Write a `uint64_t` to `reactor_eventfd_write_fd(fi.FD())` to wake the reactor.

#### AddTimer — periodic/one-shot timer

```cpp
FdInfo& AddTimer(
    std::string const& a_name,
    uint32_t           a_initial_msec,
    uint32_t           a_interval_msec,
    EventHandler       a_on_timer,
    ErrHandler         a_on_error,
    void*              a_opaque = nullptr);
```

`a_interval_msec = 0` means one-shot.  `a_on_timer(fi, expirations)` receives the number of missed firings.

#### AddSignal — signal delivery via reactor

```cpp
FdInfo& AddSignal(
    std::string const& a_name,
    const sigset_t&    a_mask,
    SigHandler  const& a_on_signal,
    ErrHandler  const& a_on_error,
    void*              a_opaque       = nullptr,
    int                a_sigq_capacity = 8);
```

Uses signalfd (Linux) or EVFILT_SIGNAL (macOS).  The signals in `a_mask` are blocked from default delivery first.

#### AddUDSListener — Unix Domain Socket server

```cpp
FdInfo& AddUDSListener(
    std::string const&   a_name,
    std::string const&   a_file_path,
    AcceptHandler const& a_on_accept,
    ErrHandler    const& a_on_error,
    void*                a_opaque      = nullptr,
    int                  a_permissions = 0660);
```

Binds, listens, and fires `a_on_accept(fi, client_path, client_fd)` for each new connection.  Return `true` to keep `client_fd`, `false` to close it.

### Event loop

```cpp
void Wait(int a_timeout_msec = 0);
```

Calls `epoll_wait` / `kevent`, dispatches all ready events, then calls the idle handler (if set) when the timeout expires.  Pass `-1` to block indefinitely.

### FD management

```cpp
void               Remove(int& a_fd, bool a_clear_fdinfo = true);
dynamic_io_buffer* RdBuff(int a_fd);
dynamic_io_buffer* WrBuff(int a_fd);
dynamic_io_buffer* SubscribeWrite(int a_fd, bool a_on);
void               SetBuffer(int a_fd, bool a_is_read, dynamic_io_buffer* a_buf);
void               ResizeBuffer(int a_fd, bool a_is_read, size_t a_size);
FdInfo&            Get(int a_fd, src_info&&);
FdInfo const&      Get(int a_fd, src_info&&) const;
```

`SubscribeWrite(fd, true)` enables `REACTOR_EV_OUT` on an already-registered fd so the write handler fires when the socket is writable.

### Static helpers

```cpp
static bool IsError   (uint32_t events);
static bool IsReadable(uint32_t events);
static bool IsWritable(uint32_t events);
```

---

## FdInfo class

Represents one registered file descriptor.  Created and owned by `Reactor`; returned by reference from `Add*` methods.

### Key accessors

| Method | Description |
|--------|-------------|
| `int FD() const` | The file descriptor number |
| `HType Type() const` | Handler type (IO, Timer, Signal, …) |
| `const std::string& Name() const` | Registered name |
| `void* Opaque() const` | User pointer passed to `Add*` |
| `void* Instance() const` | Instance pointer passed to `Add*` |
| `dynamic_io_buffer* RdBuff()` | Read buffer (may be null) |
| `dynamic_io_buffer* WrBuff()` | Write buffer (may be null) |
| `TriggerT Trigger() const` | EDGE_TRIGGERED or LEVEL_TRIGGERED |
| `Reactor& Owner()` | The owning reactor |
| `AIOReader* FileReader()` | Non-null for File handlers |
| `time_val TsWire() const` | Timestamp of last received packet (UDP only) |

### Network metadata (UDP with pkt-info)

```cpp
void      EnableDgramPktInfo(bool a_enable = true);
void      EnablePktTimeStamps(bool a_enable);
in_addr_t SockSrcAddr() const;
in_port_t SockSrcPort() const;
in_addr_t SockDstAddr() const;
in_port_t SockDstPort() const;
in_addr_t SockIfAddr() const;
```

### Lifecycle

```cpp
void Clear();   // deregister fd, release buffers
void Reset();   // clear internal state (fd must already be -1)
```

---

## Handler types

All handler types are `utxx::function<Sig>` (a lightweight `std::function` equivalent).

| Type alias | Signature | Used for |
|------------|-----------|----------|
| `RWIOHandler` | `int(FdInfo&, dynamic_io_buffer&)` | buffered read/write |
| `IOHandler` | `int(FdInfo&, dynamic_io_buffer&)` | alias for File/Pipe |
| `RawIOHandler` | `void(FdInfo&, IOType, uint32_t events)` | raw edge events |
| `EventHandler` | `void(FdInfo&, long value)` | eventfd / timer |
| `SigHandler` | `void(FdInfo&, int signo, int si_code)` | signals |
| `AcceptHandler` | `bool(FdInfo&, const char* path, int cli_fd)` | new UDS connections |
| `ErrHandler` | `void(FdInfo&, IOType, const string&, src_info&&)` | errors |
| `IdleHandler` | `void()` | reactor idle (timeout expired) |
| `Logger` | `void(log_level, src_info&&, const string&)` | log sink |
| `ReadSizeEstim` | `size_t(const char* buf, size_t size)` | minimum-read hint |

---

## Enumerations

### HType — handler type

```
UNDEFINED   IO   RawIO   File   Pipe   Event   Timer   Signal   Accept   Error
```

### IOType — I/O operation type (passed to ErrHandler)

```
Init  Read  Write  Connect  Disconnect  Accept  EndOfFile
Decoding  System  UserCode  UserData  Auth  Fatal  AppLogic
```

### FdTypeT — file descriptor type

```
UNDEFINED  Stream  Datagram  SeqPacket  File  Pipe  Event  Timer  Signal
```

### TriggerT

```
LEVEL_TRIGGERED   EDGE_TRIGGERED
```

---

## AIOReader class

Linux async file reader (throws `ENOSYS` on macOS).

```cpp
AIOReader();
AIOReader(int a_efd, const char* a_filename);   // calls Init()
~AIOReader();                                    // calls Clear()
```

### Methods

```cpp
void                         Init(int a_efd, const char* a_filename);
void                         Clear();
int                          AsyncRead(char* a_buf, size_t a_size);
int                          CheckEvents();
std::pair<long, const char*> ReadEvents(int a_events);
```

| Accessor | Description |
|----------|-------------|
| `long Offset()` | Bytes consumed from the start of the file |
| `long Position()` | Byte offset of the last `AsyncRead` call |
| `long Size()` | Total file size in bytes |
| `long Remaining()` | `Size() - Offset()` |
| `int EventFD()` | The eventfd used for completion notifications |
| `const string& Filename()` | Path passed to `Init` |

### Typical usage

```cpp
int efd = utxx::reactor_eventfd_create();
utxx::AIOReader reader(efd, "/data/file.bin");

std::vector<char> buf(reader.Size());
reader.AsyncRead(buf.data(), buf.size());

// later, when efd becomes readable:
int n = reader.CheckEvents();          // drains the eventfd counter
auto [bytes, err] = reader.ReadEvents(n);
if (bytes < 0) perror(err);
```

---

## Platform abstraction

`#include <reactor/reactor_platform.hpp>`

Provides a uniform API over epoll (Linux) and kqueue (macOS/BSD).

### Types

```cpp
// Linux
using reactor_event_t  = struct epoll_event;
using reactor_handle_t = int;

// macOS/BSD
struct reactor_event_t { int fd; uint32_t mask; };
using reactor_handle_t = int;
```

### Event flag constants (namespace utxx)

| Constant | Linux value | Description |
|----------|-------------|-------------|
| `REACTOR_EV_IN` | `EPOLLIN` | fd is readable |
| `REACTOR_EV_OUT` | `EPOLLOUT` | fd is writable |
| `REACTOR_EV_ERR` | `EPOLLERR` | error condition |
| `REACTOR_EV_HUP` | `EPOLLHUP` | hang-up |
| `REACTOR_EV_RDHUP` | `EPOLLRDHUP` | peer closed write end |
| `REACTOR_EV_ET` | `EPOLLET` | edge-triggered mode |
| `REACTOR_EV_ONESHOT` | `EPOLLONESHOT` | one-shot delivery |

### Multiplexer operations

```cpp
reactor_handle_t reactor_create();
void             reactor_destroy(reactor_handle_t h);
void             reactor_add (reactor_handle_t h, int fd, uint32_t ev, uintptr_t udata = 0);
void             reactor_mod (reactor_handle_t h, int fd, uint32_t ev, uintptr_t udata = 0);
void             reactor_del (reactor_handle_t h, int fd);
int              reactor_wait(reactor_handle_t h, reactor_event_t* buf, int maxev, int timeout_ms);
```

### Event inspection

```cpp
int      reactor_ev_fd  (const reactor_event_t& ev);  // extract fd
uint32_t reactor_ev_mask(const reactor_event_t& ev);  // extract event flags
bool     reactor_is_error   (uint32_t mask);
bool     reactor_is_readable(uint32_t mask);
bool     reactor_is_writable(uint32_t mask);
```

### Eventfd / wakeup

```cpp
int  reactor_eventfd_create();              // create; returns the readable fd
int  reactor_eventfd_write_fd(int efd);     // Linux: same fd; macOS: write end of pipe
int  reactor_eventfd_read (int efd, uint64_t& val);
int  reactor_eventfd_write(int write_fd, uint64_t val);
void reactor_eventfd_close(int efd);        // closes all underlying fds
```

### Timerfd

```cpp
int  reactor_timerfd_create();
int  reactor_timerfd_arm(reactor_handle_t kq, int timer_id,
                         uint64_t initial_ms, uint64_t interval_ms);
int  reactor_timerfd_read(int timer_id, uint64_t& expirations);
void reactor_timerfd_close(reactor_handle_t kq, int timer_id);
```

### Signalfd

```cpp
int  reactor_signalfd_create(reactor_handle_t kq, const sigset_t& mask, int capacity);
int  reactor_signalfd_read  (int fd, int& signo, int& code);
void reactor_signalfd_close (reactor_handle_t kq, int fd, const sigset_t& mask);
```

---

## Utility functions

`#include <reactor/reactor_misc.hpp>`

### Network helpers

```cpp
// Describe epoll event flags as a string, e.g. "IN|OUT|ET"
std::string EPollEvents(int a_events);

// Resolve interface name + IP for the interface routing to a_ip
void     GetIfAddr(const char* a_ip, std::string& ifname, std::string& ifip);

// Get interface IP from interface name
in_addr  GetIfAddr(const std::string& ifname);

// Get interface name from its IP address
std::string GetIfName(in_addr ifaddr);

// Return true if the UDS socket file is bound and listening
bool     IsUDSAlive(const std::string& uds_filename);
```

### FD helpers

```cpp
// Set or clear the O_NONBLOCK flag; returns true on success
bool Blocking(int fd, bool block);

// Returns true if fd is in blocking mode
bool IsBlocking(int fd);

// Returns the pending socket error (SO_ERROR), or 0
int  SocketError(int fd);
```

### I/O helpers

```cpp
// write(2) with EINTR retry; returns bytes written or -1
int Send(int fd, const char* buf, size_t sz, int flags = MSG_NOSIGNAL);

// sendto(2) with EINTR retry
int SendTo(int sock, const char* buf, size_t sz, int flags,
           const sockaddr* dest, socklen_t destlen);

// Drain a non-blocking fd into buf, calling action after each read.
// Stops on EAGAIN or when action returns < 0.
template <typename Action, typename DebugAction>
bool ReadUntilEAgain(int fd, dynamic_io_buffer& buf,
                     Action const& action,
                     DebugAction const& debug_action,
                     const std::string& name = "",
                     int max_reads = INT_MAX);
```

---

## Error handling

Errors are reported through the `ErrHandler` callback:

```cpp
using ErrHandler = utxx::function<void(
    FdInfo&           fi,
    IOType            type,    // what operation failed
    const string&     message,
    src_info&&        where    // source location macro
)>;
```

If no error handler is provided, the reactor logs to stderr and removes the fd.

### Exception types (thrown by Add* on setup failure)

```cpp
struct io_error      : reactor_error { int m_errno; … };
struct runtime_error : reactor_error { … };
struct badarg_error  : reactor_error { … };
```

All derive from `std::runtime_error`.  `src_info` stores file/line/function context.

### Throw macros

```cpp
REACTOR_THROWX_IO_ERROR(errno, "message parts", …);
REACTOR_THROW(utxx::runtime_error, "message parts", …);
```

---

## Logging

The reactor logs internally with a configurable sink.

### Default logger

Writes `[LEVEL] src:line fun message` to `stderr`.

### Custom logger

```cpp
r.SetLogger([](utxx::log_level level,
               utxx::src_info&& src,
               const std::string& msg) {
    printf("[%s] %s\n", utxx::log_level_to_string(level), msg.c_str());
});
```

### Log levels

```
NONE  FATAL  ERROR  WARNING  INFO  DEBUG  TRACE  TRACE1  TRACE2  TRACE3  TRACE4  TRACE5
```

### UTXX_RLOG macro

Used internally (and available to handlers):

```cpp
UTXX_RLOG(reactor_or_fdinfo_ptr, TRACE5, "field=", value, " other=", other);
```

Expands to a log call only when the reactor's debug level is high enough.
