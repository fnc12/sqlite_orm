#pragma once

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  This is a cute class which allows storing something or nothing
         *  depending on template argument. Useful for optional class members
         */
        template<class T>
        struct optional_container {
            using type = T;
            
            type field;
            
            template<class L>
            void apply(const L &l) const {
                l(this->field);
            }
        };
        
        template<>
        struct optional_container<void>{
            using type = void;
            
            template<class L>
            void apply(const L &) const {
                //..
            }
        };
    }
}
