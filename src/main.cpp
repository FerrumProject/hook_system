#include <any>
#include <iostream>

#include "hook.hpp"

int foo(int a, int b) { return a + b; }
hook_t<int, int, int> bar([](int a, int b) {
  bar.execute_callbacks(a, b);

  return bar.original(a, b) * 2;
});

int main() {
  MH_Initialize();

  bar.add_callback<CallbackType::Always>(
      [](int, int) -> std::any {
        std::cout << "Hello world!" << std::endl;

        return std::any();
      },
      "Hello", {"Hooked", "Foo"});

  bar.add_callback<CallbackType::Single>(
      [](int, int) -> std::any {
        std::cout << "Foo" << std::endl;

        return std::any();
      },
      "Foo", {"Hooked"});

  bar.add_callback<CallbackType::Single>(
      [](int, int) -> std::any {
        std::cout << "Hooked" << std::endl;

        return std::any();
      },
      "Hooked");

  bar.hook(&foo);

  std::cout << foo(2, 3) << std::endl;
  std::cout << foo(2, 3) << std::endl;
  std::cout << foo(2, 3) << std::endl;

  MH_DisableHook(MH_ALL_HOOKS);
  MH_RemoveHook(MH_ALL_HOOKS);

  MH_Uninitialize();
  return 0;
}