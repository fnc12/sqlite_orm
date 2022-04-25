#pragma once

#include "cxx_polyfill.h"

template<class F>
class final_action {
  public:
    explicit final_action(F f) noexcept : f_{std::move(f)} {}

    final_action(final_action&& other) noexcept :
        f_{std::move(other.f_)}, invoke_{std::exchange(other.invoke_, false)} {}

    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
    final_action& operator=(final_action&&) = delete;

    ~final_action() {
        if(invoke_) {
            f_();
        }
    }

  private:
    F f_;
    bool invoke_ = true;
};

template<class F>
auto finally(F&& f) noexcept {
    return final_action<sqlite_orm::polyfill::remove_cvref_t<F>>(std::forward<F>(f));
}
