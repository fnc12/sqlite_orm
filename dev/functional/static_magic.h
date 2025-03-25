#pragma once

namespace sqlite_orm {

    //  got from here
    //  https://stackoverflow.com/questions/37617677/implementing-a-compile-time-static-if-logic-for-different-string-types-in-a-co
    namespace internal {

        // note: this is a class template accompanied with a variable template because older compilers (e.g. VC 2017)
        // cannot handle a static lambda variable inside a template function
        template<class R>
        struct empty_callable_t {
            template<class... Args>
            R operator()(Args&&...) const {
                return R();
            }
        };
        template<class R = void>
        constexpr empty_callable_t<R> empty_callable{};
    }

}
