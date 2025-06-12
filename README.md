## uthreads: A User-Level Thread Library

uthreads is a lightweight, user-level threading library implemented in C. It enables developers to create, manage, and schedule multiple threads within a single process, without relying on kernel-level threading. This library is provided as a static library and exposes a simple, well-documented API via the `uthreads.h` header file.

---

**Features**

- Create and manage user-level threads efficiently
- Cooperative scheduling for lightweight context switching
- Simple, intuitive API for thread operations
- Encapsulated internal logic and data structures for safety and clarity

---

**Getting Started**

1. **Include the Library**

   Add `uthreads.h` to your project and link against the compiled static library (`libuthreads.a`).

   ```c
   #include "uthreads.h"
   ```

2. **Basic Usage Example**

   ```c
   #include "uthreads.h"

   void thread_func(void* arg) {
       // Thread logic here
   }

   int main() {
       uthreads_init();
       uthreads_create(thread_func, NULL);
       uthreads_run();
       return 0;
   }
   ```

---

**API Overview**

| Function                | Description                                   |
|-------------------------|-----------------------------------------------|
| `uthreads_init()`       | Initialize the threading environment          |
| `uthreads_create()`     | Create a new user-level thread                |
| `uthreads_yield()`      | Yield execution to another thread             |
| `uthreads_exit()`       | Terminate the current thread                  |
| `uthreads_run()`        | Start the scheduler and run threads           |
| `uthreads_join()`       | Wait for a thread to finish                   |

*See `uthreads.h` for full API documentation and usage details.*

---

**Implementation Notes**

- All internal data structures and helper functions are private to the library and not exposed to users.
- The implementation focuses on clarity, efficiency, and maintainability.
- Thread switching and management are handled cooperatively; threads must yield control explicitly.

---

**Building**

Compile the library using your preferred C compiler:

```sh
gcc -c uthreads.c -o uthreads.o
ar rcs libuthreads.a uthreads.o
```

Link `libuthreads.a` with your application.

---

**Contributing**

Contributions and suggestions are welcome! Please ensure code is clear, well-documented, and efficient.

---

**License**

This project is licensed under the MIT License.

---

**Contact**

For questions or feedback, please open an issue or contact the maintainer.

---

Enjoy lightweight, user-level threading with uthreads!
