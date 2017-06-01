/**
 *  Example is imlemented from here http://www.sqlitetutorial.net/sqlite-left-join/
 *  In this example you got to download db file 'chinook.db' first from here http://www.sqlitetutorial.net/sqlite-sample-database/
 *  an place the file near executable.
 */

#include "sqlite_orm.h"

#include <string>
#include <iostream>
#include <memory>

using std::cout;
using std::endl;

struct Artist {
    std::shared_ptr<int> artistId;
    std::shared_ptr<std::string> name;
};

struct Album {
    std::shared_ptr<int> albumId;
    std::shared_ptr<std::string> title;
    std::shared_ptr<int> artistId;
};

struct Track {
    int trackId;
    std::string name;
    std::shared_ptr<int> albumId;
    int mediaTypeId;
    std::shared_ptr<int> genreId;
    std::shared_ptr<std::string> composer;
    long milliseconds;
    std::shared_ptr<long> bytes;
    double unitPrice;
};

inline auto initStorage(const std::string &path){
    using namespace sqlite_orm;
    return make_storage(path,
                        make_table("artists",
                                   make_column("ArtistId",
                                               &Artist::artistId,
                                               primary_key()),
                                   make_column("Name",
                                               &Artist::name)),
                        make_table("albums",
                                   make_column("AlbumId",
                                               &Album::albumId,
                                               primary_key()),
                                   make_column("Title",
                                               &Album::title),
                                   make_column("ArtistId",
                                               &Album::artistId)),
                        make_table("tracks",
                                   make_column("TrackId",
                                               &Track::trackId,
                                               primary_key()),
                                   make_column("Name",
                                               &Track::name),
                                   make_column("AlbumId",
                                               &Track::albumId),
                                   make_column("MediaTypeId",
                                               &Track::mediaTypeId),
                                   make_column("GenreId",
                                               &Track::genreId),
                                   make_column("Composer",
                                               &Track::composer),
                                   make_column("Milliseconds",
                                               &Track::milliseconds),
                                   make_column("Bytes",
                                               &Track::bytes),
                                   make_column("UnitPrice",
                                               &Track::unitPrice)));
}

int main(int argc, char **argv) {
    
    auto storage = initStorage("chinook.db");

    using namespace sqlite_orm;
    
    //  SELECT
    //      artists.ArtistId,
    //      albumId
    //  FROM
    //      artists
    //  LEFT JOIN albums ON albums.artistid = artists.artistid
    //  ORDER BY
    //      albumid;
    auto rows = storage.select(columns(&Artist::artistId, &Album::albumId),
                               left_join<Album>(on(is_equal(&Album::artistId, &Artist::artistId))),
                               order_by(&Album::albumId));
    cout << "rows count = " << rows.size() << endl;
    for(auto &row : rows) {
        auto &artistId = std::get<0>(row);
        if(artistId){
            cout << *artistId;
        }else{
            cout << "null";
        }
        cout << '\t';
        auto &albumId = std::get<1>(row);
        if(albumId){
            cout << *albumId;
        }else{
            cout << "null";
        }
        cout << endl;
    }
    
    cout << endl;
    
    
    //  SELECT
    //      artists.ArtistId,
    //      albumId
    //  FROM
    //      artists
    //  LEFT JOIN albums ON albums.artistid = artists.artistid
    //  WHERE
    //      albumid IS NULL;
    rows = storage.select(columns(&Artist::artistId, &Album::albumId),
                          left_join<Album>(on(is_equal(&Album::artistId, &Artist::artistId))),
                          where(is_null(&Album::albumId)));
    cout << "rows count = " << rows.size() << endl;
    for(auto &row : rows) {
        auto &artistId = std::get<0>(row);
        if(artistId){
            cout << *artistId;
        }else{
            cout << "null";
        }
        cout << '\t';
        auto &albumId = std::get<1>(row);
        if(albumId){
            cout << *albumId;
        }else{
            cout << "null";
        }
        cout << endl;
    }
    
    cout << endl;
    
    //  SELECT
    //      trackid,
    //      name,
    //      title
    //  FROM
    //      tracks
    //  INNER JOIN albums ON albums.albumid = tracks.albumid;
    auto innerJoinRows0 = storage.select(columns(&Track::trackId, &Track::name, &Album::title),
                                         inner_join<Album>(on(is_equal(&Track::albumId, &Album::albumId))));
    cout << "innerJoinRows0 count = " << innerJoinRows0.size() << endl;
    for(auto &row : innerJoinRows0) {
        cout << std::get<0>(row) << '\t' << std::get<1>(row) << '\t';
        if(std::get<2>(row)){
            cout << *std::get<2>(row);
        }else{
            cout << "null";
        }
        cout << endl;
    }
    cout << endl;
    
    //  SELECT
    //      trackid,
    //      name,
    //      tracks.AlbumId,
    //      albums.AlbumId,
    //      title
    //  FROM
    //      tracks
    //  INNER JOIN albums ON albums.albumid = tracks.albumid;
    auto innerJoinRows1 = storage.select(columns(&Track::trackId, &Track::name, &Track::albumId, &Album::albumId, &Album::title),
                                         inner_join<Album>(on(is_equal(&Album::albumId, &Track::trackId))));
    cout << "innerJoinRows1 count = " << innerJoinRows1.size() << endl;
    for(auto &row : innerJoinRows1) {
        cout << std::get<0>(row) << '\t' << std::get<1>(row) << '\t';
        if(std::get<2>(row)){
            cout << *std::get<2>(row);
        }else{
            cout << "null";
        }
        cout << '\t';
        if(std::get<3>(row)){
            cout << *std::get<3>(row);
        }else{
            cout << "null";
        }
        cout << '\t';
        if(std::get<4>(row)){
            cout << *std::get<4>(row);
        }else{
            cout << "null";
        }
        cout << '\t';
        cout << endl;
    }
    cout << endl;
    
    //  SELECT
    //      trackid,
    //      tracks.name AS Track,
    //      albums.title AS Album,
    //      artists.name AS Artist
    //  FROM
    //      tracks
    //  INNER JOIN albums ON albums.albumid = tracks.albumid
    //  INNER JOIN artists ON artists.artistid = albums.artistid;
    auto innerJoinRows2 = storage.select(columns(&Track::trackId, &Track::name, &Album::title, &Artist::name),
                                         inner_join<Album>(on(eq(&Album::albumId, &Track::albumId))),
                                         inner_join<Artist>(on(eq(&Artist::artistId, &Album::artistId))));
    cout << "innerJoinRows2 count = " << innerJoinRows2.size() << endl;
    for(auto &row : innerJoinRows2) {
        cout << std::get<0>(row) << '\t' << std::get<1>(row) << '\t';
        if(std::get<2>(row)){
            cout << *std::get<2>(row);
        }else{
            cout << "null";
        }
        cout << '\t';
        if(std::get<3>(row)){
            cout << *std::get<3>(row);
        }else{
            cout << "null";
        }
        cout << '\t';
    }
    cout << endl;
    
    return 0;
}
