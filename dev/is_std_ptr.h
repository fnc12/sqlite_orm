#pragma once

namespace sqlite_orm {

    /**
     *  Specialization for optional type (std::shared_ptr / std::unique_ptr).
     */
    template<typename T>
    struct is_std_ptr : std::false_type {};

    template<typename T>
    struct is_std_ptr<std::shared_ptr<T>> : std::true_type {
        using container_type = typename std::shared_ptr<T>;
        using element_type = T;

        static std::shared_ptr<T> make(const T &v) {
            return std::make_shared<T>(v);
        }
        static bool isEmpty(const std::shared_ptr<T> &v) {
            return static_cast<bool>(v);
        }
    };

    template<typename T>
    struct is_std_ptr<std::unique_ptr<T>> : std::true_type {
        using container_type = typename std::unique_ptr<T>;
        using element_type = T;

        static std::unique_ptr<T> make(const T &v) {
            return std::make_unique<T>(v);
        }
        static bool isEmpty(const std::unique_ptr<T> &v) {
            return static_cast<bool>(v);
        }
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<typename T>
    struct is_std_ptr<std::optional<T>> : std::true_type {
        using container_type = typename std::optional<T>;
        using element_type = T;

        static std::optional<T> make(const T &v) {
            return std::make_optional<T>(v);
        }
        static bool isEmpty(const std::optional<T> &v) {
            return v.has_value();
        }
    };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
}
