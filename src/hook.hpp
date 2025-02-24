#pragma once

#include <algorithm>
#include <any>
#include <queue>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MinHook.h"

enum class CallbackState { Wait, Failed, Called };
enum class CallbackType { Single, Always };

template <typename Type> class hook_t;

template <typename Type, typename... Args> class hook_t<Type (*)(Args...)> {
public:
  using Fn = Type (*)(Args...);
  using CallbackFn = std::any (*)(Args...);

private:
  Fn m_detour;
  Fn m_original;

  struct Callback {
    CallbackState m_state;
    CallbackType m_type;

    CallbackFn m_callback;

    std::string_view m_id;
    std::vector<std::string_view> m_dependencies;

    Callback(std::string_view id, std::vector<std::string_view> dependencies,
             CallbackFn callback, CallbackType type)
        : m_id(id), m_dependencies(dependencies), m_callback(callback),
          m_type(type), m_state(CallbackState::Wait) {}

    std::any call(Args... args) {
      try {
        if (m_type == CallbackType::Single && m_state == CallbackState::Called)
          return std::any();

        auto result = this->m_callback(args...);
        this->m_state = CallbackState::Called;

        return result;
      } catch (std::any) {
        this->m_state = CallbackState::Failed;
      }

      return std::any();
    }
  };

  std::vector<Callback> m_callbacks;

public:
  hook_t(Fn detour) : m_detour(detour) {}

  ~hook_t() {
    if (this->m_original) {
      this->unhook();
    }
  }

  template <CallbackType CType>
  void add_callback(CallbackFn callback, std::string_view id = "",
                    std::vector<std::string_view> dependencies = {}) {
    this->m_callbacks.push_back(
        Callback(id, dependencies, std::move(callback), CType));
  }

  std::vector<Callback> &callbacks() { return this->m_callbacks; }

  MH_STATUS
  hook(auto target) {
    if (this->m_original) {
      return MH_ERROR_ALREADY_CREATED;
    }

    auto status = MH_CreateHook(reinterpret_cast<LPVOID>(target),
                                reinterpret_cast<LPVOID>(this->m_detour),
                                reinterpret_cast<LPVOID *>(&this->m_original));

    if (status == MH_OK) {
      status = MH_EnableHook(reinterpret_cast<LPVOID>(target));

      if (status != MH_OK) {
        MH_RemoveHook(reinterpret_cast<LPVOID>(target));
        this->m_original = nullptr;
      }
    }

    return status;
  }

  MH_STATUS unhook() {
    if (!this->m_original) {
      return MH_ERROR_NOT_CREATED;
    }

    auto status = MH_DisableHook(reinterpret_cast<LPVOID>(this->m_original));

    if (status == MH_OK) {
      status = MH_RemoveHook(reinterpret_cast<LPVOID>(this->m_original));
      this->m_original = nullptr;
    }

    return status;
  }

  auto original(Args... args) {
    return this->m_original(std::forward<Args>(args)...);
  }

  void execute_callbacks(Args... args) {
    std::unordered_map<std::string_view, Callback *> id_to_callback;
    for (auto &cb : m_callbacks) {
      if (!cb.m_id.empty()) {
        id_to_callback[cb.m_id] = &cb;
      }
    }

    std::vector<Callback *> candidates;
    std::unordered_set<Callback *> candidates_set;
    for (auto &cb : m_callbacks) {
      if ((cb.m_type == CallbackType::Single &&
           cb.m_state == CallbackState::Wait) ||
          cb.m_type == CallbackType::Always) {
        candidates.push_back(&cb);
        candidates_set.insert(&cb);
      }
    }

    if (candidates.empty()) {
      return;
    }

    std::unordered_map<Callback *, int> dep_count_map;
    std::unordered_map<std::string_view, std::vector<Callback *>> dependents;

    for (auto cb : candidates) {
      int dep_count = 0;
      for (auto dep_id : cb->m_dependencies) {
        auto it = id_to_callback.find(dep_id);
        if (it == id_to_callback.end()) {
          dep_count++;
          continue;
        }
        Callback *dep_cb = it->second;
        if (candidates_set.count(dep_cb)) {
          dep_count++;
          dependents[dep_id].push_back(cb);
        } else {
          if (dep_cb->m_state != CallbackState::Called) {
            dep_count++;
          }
        }
      }
      dep_count_map[cb] = dep_count;
    }

    std::queue<Callback *> queue;
    for (auto cb : candidates) {
      if (dep_count_map[cb] == 0) {
        queue.push(cb);
      }
    }

    while (!queue.empty()) {
      Callback *cb = queue.front();
      queue.pop();

      cb->call(args...);

      auto it = dependents.find(cb->m_id);
      if (it != dependents.end()) {
        for (auto dependent : it->second) {
          dep_count_map[dependent]--;
          if (dep_count_map[dependent] == 0) {
            queue.push(dependent);
          }
        }
      }
    }
  }
};