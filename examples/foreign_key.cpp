//
//  foreign_key.cpp
//  CPPTest
//
//  Created by John Zakharov on 29.06.17.
//  Copyright © 2017 John Zakharov. All rights reserved.
//

#include "sqlite_orm.h"

#include <string>
#include <iostream>

using std::cout;
using std::endl;

struct Artist {
    int artistId;
    std::string artistName;
};

struct Track {
    int trackId;
    std::string trackName;
    int trackArtist;    //  must map to &Artist::artistId
};

int main(int argc, char **argv) {
    cout << "path = " << argv[0] << endl;
    
    using namespace sqlite_orm;
    auto storage = make_storage("foreign_key.sqlite",
                                make_table("artist",
                                           make_column("artistid",
                                                       &Artist::artistId,
                                                       primary_key()),
                                           make_column("artistname",
                                                       &Artist::artistName)),
                                make_table("track",
                                           make_column("trackid",
                                                       &Track::trackId,
                                                       primary_key()),
                                           make_column("trackname",
                                                       &Track::trackName),
                                           make_column("trackartist",
                                                       &Track::trackArtist)
                                           ,foreign_key(&Track::trackArtist).references(&Artist::artistId)
                                           ));
    storage.sync_schema();
    
    return 0;
}
