#pragma once
#include <type_traits>
#include <memory>

namespace sqlite_orm {

    /**
     *  Specialization for optional type (std::shared_ptr / std::unique_ptr).
     */
    template<typename T>
    struct is_std_ptr : std::false_type {};

    template<typename T>
    struct is_std_ptr<std::shared_ptr<T>> : std::true_type {
        using element_type = typename std::shared_ptr<T>::element_type;

        static std::shared_ptr<T> make(std::remove_cv_t<T>&& v) {
            return std::make_shared<T>(std::move(v));
        }
    };

    template<typename T>
    struct is_std_ptr<std::unique_ptr<T>> : std::true_type {
        using element_type = typename std::unique_ptr<T>::element_type;

        static auto make(std::remove_cv_t<T>&& v) {
            return std::make_unique<T>(std::move(v));
        }
    };
}
