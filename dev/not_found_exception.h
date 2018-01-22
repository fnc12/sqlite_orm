//
//  not_found_exception.h
//  CPPTest
//
//  Created by John Zakharov on 21.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef not_found_exception_h
#define not_found_exception_h

#include <stdexcept>

namespace sqlite_orm {
    
    /**
     *  Exeption thrown if nothing was found in database with specified id.
     */
    struct not_found_exception : public std::exception {
        
        const char* what() const throw() override {
            return "Not found";
        };
    };
}

#endif /* not_found_exception_h */
