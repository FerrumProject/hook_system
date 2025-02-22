# hook_system
Object-oriented library for hook

## Example
```cpp
// Original function
int foo(int a, int b) { return a + b; }

// Hook object <Type, ...args of function>
hook_t<int, int, int> bar([](int a, int b) {
  bar.execute_callbacks(a, b);

  return bar.original(a, b) * 2;
});

// add callback <Always || Single>(callback_fn, id, dependencies)
bar.add_callback<CallbackType::Always>(
  [](int, int) -> std::any {
    std::cout << "Hello world!" << std::endl;

    return std::any();
  },
  "Hello", {"Hooked", "Foo"});

// Setup hook
bar.hook(&foo);
```

### Libraries used
[MinHook](https://github.com/TsudaKageyu/minhook)

### Dependencies
+ [CMake](https://cmake.org/)
+ [MinGW (Linux)](https://www.mingw-w64.org/) (See [toolchain.cmake](./toolchain.cmake))