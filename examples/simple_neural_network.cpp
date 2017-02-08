//
//  simple_neural_network.cpp
//  CPPTest
//
//  Created by John Zakharov on 04.02.17.
//  Copyright © 2017 John Zakharov. All rights reserved.
//

#include "sqlite_orm.h"

#include <iostream>
#include <random>

using std::cout;
using std::cin;
using std::endl;

struct Turn {
    int sticksCount;
    int shouldPickOneStick;
    int shouldPickTwoSticks;
};

using namespace sqlite_orm;

int main(int argc, char **argv) {
    
    static auto storage = make_storage("simple_neural_network.sqlite",
                                       make_table("turns_info",
                                                  make_column("sticks_count",
                                                              &Turn::sticksCount),
                                                  make_column("one",
                                                              &Turn::shouldPickOneStick),
                                                  make_column("two",
                                                              &Turn::shouldPickTwoSticks)));
    storage.sync_schema();
    
    int sticksLeft = 11;
    
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0, 1);
    
    for (int i=0; i<16; ++i)
        std::cout << dist(mt) << "\n";
    
    cout << "make a turn: input 1 or 2:" << endl;
    
    while(true){
        int number;
        cin >> number;
        switch(number){
            case 1:
                sticksLeft -= 1;
                break;
            case 2:
                sticksLeft -= 2;
                break;
            default:
                
                break;
        }
    }
    
    return 0;
}
