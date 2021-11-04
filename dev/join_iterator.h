#pragma once

#include "conditions.h"

namespace sqlite_orm {

    namespace internal {

        template<class... Args>
        struct join_iterator;

        template<>
        struct join_iterator<> {

            template<class L>
            void operator()(const L&) const {
                //..
            }
        };

        template<class H, class... Tail>
        struct join_iterator<H, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;

            template<class L>
            void operator()(const L& l) const {
                this->super::operator()(l);
            }
        };

        template<class T, class... Tail>
        struct join_iterator<cross_join_t<T>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = cross_join_t<T>;

            template<class L>
            void operator()(const L& l) const {
                l(*this);
                this->super::operator()(l);
            }
        };

        template<class T, class... Tail>
        struct join_iterator<natural_join_t<T>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = natural_join_t<T>;

            template<class L>
            void operator()(const L& l) const {
                l(*this);
                this->super::operator()(l);
            }
        };

        template<class T, class O, class... Tail>
        struct join_iterator<left_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = left_join_t<T, O>;

            template<class L>
            void operator()(const L& l) const {
                l(*this);
                this->super::operator()(l);
            }
        };

        template<class T, class O, class... Tail>
        struct join_iterator<join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = join_t<T, O>;

            template<class L>
            void operator()(const L& l) const {
                l(*this);
                this->super::operator()(l);
            }
        };

        template<class T, class O, class... Tail>
        struct join_iterator<left_outer_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = left_outer_join_t<T, O>;

            template<class L>
            void operator()(const L& l) const {
                l(*this);
                this->super::operator()(l);
            }
        };

        template<class T, class O, class... Tail>
        struct join_iterator<inner_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            using super = join_iterator<Tail...>;
            using join_type = inner_join_t<T, O>;

            template<class L>
            void operator()(const L& l) const {
                l(*this);
                this->super::operator()(l);
            }
        };
    }
}
