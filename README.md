# AD_TUN

`ad_tun` is a compact, self-contained TUN interface manager for Linux written in C. It provides a clean API to create, configure, bring up/down, read from, and write to a TUN device — ideal for VPNs, network tunnels, or any application that needs direct access to raw IP packets without dealing with repetitive low-level boilerplate.

---

## Features

* **Full TUN Lifecycle Management** – Initialize, create, configure, bring up/down, restart, and clean up.
* **INI-Based Configuration Loader** – Uses `inih` to load interface name, MTU, IPv4/IPv6, and persist flags.
* **Structured Logging (zlog)** – All operations use the `ad_tun` logging category.
* **Thread-Safe State Management** – Internal global state protected via mutex.
* **Simple Packet I/O APIs** – Blocking read/write wrappers for raw IP packets.
* **Convenience Helpers** – Access configuration, MTU, IPs, file descriptor, and interface state.
* **Unit-Test Ready** – GTest integration supported.

---

## Project Structure

```
workspace/
├── ad_tun/
│   ├── configs/           # Sample INI configs for manual + test usage
│   ├── include/           # Public headers (ad_tun.h, ...)
│   ├── src/               # Implementation (.c files)
│   ├── manual_test/       # Manual standalone test program
│   ├── test_configs/      # INI configs used for gtest
│   ├── tests/             # GTest unit tests
│   └── README.md
└── prebuilt/
    ├── googletest/
    │   └── build/         # Build gtest here
    ├── inih/
    │   ├── include/       # ini.h
    │   └── src/           # ini.c
    └── zlog/
        ├── config/        # ad_zlog_config.conf
        ├── include/       # zlog.h
        └── lib/           # libzlog.so and links
```

---

## Dependencies

* **zlog** – Logging
* **inih** – INI parser
* **gtest** (optional) – Unit tests
* **Linux with TUN/TAP support**

---

## Building the Project

```
mkdir build
cd build
cmake ..
make
```

### Running Unit Tests

Requires superuser, since tests create TUN interfaces:

```
sudo ctest
```

---

## Manual Testing

From the `manual_test/` directory:

1. Compile the standalone test:

```
gcc -c ad_tun_test.c -o ad_tun_test.o
```

2. Link with the module:

```
gcc ad_tun_test.o -L../build/lib -lad_tun -Wl,-rpath=../build/lib -o ad_tun_test
```

3. Run:

```
sudo ./ad_tun_test
```

---

## Development Notes

### Modular Architecture

The module is implemented in three internal components:

1. **Config Loader** – Parses INI files using `inih` and fills `ad_tun_config_t`.
2. **Device Manager** – Creates TUN device, assigns IPv4/IPv6, configures MTU, and applies persist flag.
3. **State Manager** – Maintains lifecycle state, protects global state via a mutex, handles cleanup and restart.

---

### State Tracking

The library uses controlled **internal global state**, protected by a lock:

* `g_state` – Current interface state (`UNINITIALIZED → RUNNING`)
* `g_cfg` – Active configuration (copied internally)
* `g_tun_fd` – File descriptor of the TUN device
* Initialization flags, mutexes, and helper fields

Clients interact only through safe getters.

---

### Clear Error Semantics

All public APIs return:

* `AD_TUN_OK` on success
* A specific `ad_tun_error_t` value on failure (`AD_TUN_ERR_SYS`, `AD_TUN_ERR_CONFIG`, etc.)

System failures include zlog messages with `errno` details.

---

### Thread Safety

* All state transitions take a mutex
* Read/write operations snapshot internal FD safely
* Config copies returned to the caller are independent and must be freed by the caller

Parallel I/O on the same FD is not guaranteed safe (Linux kernel rule).

---

### Recommended Usage Pattern

```c
ad_tun_config_t cfg;
ad_tun_load_config("tun.ini", &cfg);

ad_tun_init(&cfg);
ad_tun_start();

// packet I/O
ad_tun_read(buf, sizeof(buf));
ad_tun_write(buf, len);

ad_tun_stop();
ad_tun_cleanup();

ad_tun_free_config(&cfg);
```

---

## API Reference

A summary of the public API defined in `ad_tun.h`:

### **Configuration APIs**

* `ad_tun_load_config(path, cfg)`
* `ad_tun_free_config(cfg)`

### **Lifecycle APIs**

* `ad_tun_init(cfg)`
* `ad_tun_start()`
* `ad_tun_stop()`
* `ad_tun_restart()`
* `ad_tun_cleanup()`

### **I/O APIs**

* `ad_tun_read(buf, len)`
* `ad_tun_write(buf, len)`

### **Information APIs**

* `ad_tun_get_fd()`
* `ad_tun_get_config_copy()`
* `ad_tun_get_name()`
* `ad_tun_get_mtu()`
* `ad_tun_get_ipv4()`
* `ad_tun_get_ipv6()`
* `ad_tun_get_state()`

---

## License

MIT License. Use it, modify it, ship it, or tape it to your wall as décor.

---

## Contributing

Pull requests are welcome.
Bug reports are welcome.
Unexpected praise is extremely welcome.
