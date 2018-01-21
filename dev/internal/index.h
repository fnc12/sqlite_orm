//
//  index.h
//  CPPTest
//
//  Created by John Zakharov on 21.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef index_h
#define index_h

#include <tuple>
#include <string>

namespace sqlite_orm {
    
    namespace internal {
        
        template<class ...Cols>
        struct index_t {
            typedef std::tuple<Cols...> columns_type;
            typedef void object_type;
            
            std::string name;
            bool unique;
            columns_type columns;
            
            template<class L>
            void for_each_column_with_constraints(L) {}
        };
    }
}

#endif /* index_h */
