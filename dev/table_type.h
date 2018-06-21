#pragma once

#include "select_constraints.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  Trait class used to define table mapped type by setter/getter/member
         */
        template<class T>
        struct table_type;
        
        template<class O, class F>
        struct table_type<F O::*> {
            using type = O;
        };
        
        template<class O, class F>
        struct table_type<const F& (O::*)() const> {
            using type = O;
        };
        
        template<class O, class F>
        struct table_type<void (O::*)(F)> {
            using type = O;
        };
        
        template<class T, class F>
        struct table_type<column_pointer<T, F>> {
            using type = T;
        };
    }
}
