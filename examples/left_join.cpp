/**
 *  Example is imlemented from here http://www.sqlitetutorial.net/sqlite-left-join/
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
                                               &Album::artistId)));
}

int main(int argc, char **argv) {
    
    auto storage = initStorage("left_join.sqlite");
    storage.sync_schema();
    
//    storage.remove_all<Artist>();
    
    storage.replace(Artist{ std::make_shared<int>(1), std::make_shared<std::string>("AC/DC") });
    storage.replace(Artist{ std::make_shared<int>(2), std::make_shared<std::string>("Accept") });
    storage.replace(Artist{ std::make_shared<int>(3), std::make_shared<std::string>("Aerosmith") });
    storage.replace(Artist{ std::make_shared<int>(4), std::make_shared<std::string>("Alanis Morissette") });
    storage.replace(Artist{ std::make_shared<int>(5), std::make_shared<std::string>("Alice In Chains") });
    storage.replace(Artist{ std::make_shared<int>(6), std::make_shared<std::string>("Antônio Carlos Jobim") });
    storage.replace(Artist{ std::make_shared<int>(7), std::make_shared<std::string>("Apocalyptica") });
    storage.replace(Artist{ std::make_shared<int>(8), std::make_shared<std::string>("Audioslave") });
    storage.replace(Artist{ std::make_shared<int>(9), std::make_shared<std::string>("BackBeat") });
    storage.replace(Artist{ std::make_shared<int>(10), std::make_shared<std::string>("Billy Cobham") });
    storage.replace(Artist{ std::make_shared<int>(11), std::make_shared<std::string>("Black Label Society") });
    storage.replace(Artist{ std::make_shared<int>(12), std::make_shared<std::string>("Black Sabbath") });
    storage.replace(Artist{ std::make_shared<int>(13), std::make_shared<std::string>("Body Count") });
    storage.replace(Artist{ std::make_shared<int>(14), std::make_shared<std::string>("Bruce Dickinson") });
    storage.replace(Artist{ std::make_shared<int>(15), std::make_shared<std::string>("Buddy Guy") });
    storage.replace(Artist{ std::make_shared<int>(16), std::make_shared<std::string>("Caetano Veloso") });
    storage.replace(Artist{ std::make_shared<int>(17), std::make_shared<std::string>("Chico Buarque") });
    storage.replace(Artist{ std::make_shared<int>(18), std::make_shared<std::string>("Chico Science & Nação Zumbi") });
    storage.replace(Artist{ std::make_shared<int>(19), std::make_shared<std::string>("Cidade Negra") });
    storage.replace(Artist{ std::make_shared<int>(20), std::make_shared<std::string>("Cláudio Zoli") });
    storage.replace(Artist{ std::make_shared<int>(21), std::make_shared<std::string>("Various Artists") });
    storage.replace(Artist{ std::make_shared<int>(22), std::make_shared<std::string>("Led Zeppelin") });
    storage.replace(Artist{ std::make_shared<int>(23), std::make_shared<std::string>("Frank Zappa & Captain Beefheart") });
    storage.replace(Artist{ std::make_shared<int>(24), std::make_shared<std::string>("Marcos Valle") });
    storage.replace(Artist{ std::make_shared<int>(25), std::make_shared<std::string>("Milton Nascimento & Bebeto") });
    storage.replace(Artist{ std::make_shared<int>(26), std::make_shared<std::string>("Azymuth") });
    storage.replace(Artist{ std::make_shared<int>(27), std::make_shared<std::string>("Gilberto Gil") });
    storage.replace(Artist{ std::make_shared<int>(28), std::make_shared<std::string>("João Gilberto") });
    storage.replace(Artist{ std::make_shared<int>(29), std::make_shared<std::string>("Bebel Gilberto") });
    storage.replace(Artist{ std::make_shared<int>(30), std::make_shared<std::string>("Jorge Vercilo") });
    storage.replace(Artist{ std::make_shared<int>(31), std::make_shared<std::string>("Baby Consuelo") });
    storage.replace(Artist{ std::make_shared<int>(32), std::make_shared<std::string>("Ney Matogrosso") });
    storage.replace(Artist{ std::make_shared<int>(33), std::make_shared<std::string>("Luiz Melodia") });
    storage.replace(Artist{ std::make_shared<int>(34), std::make_shared<std::string>("Nando Reis") });
    storage.replace(Artist{ std::make_shared<int>(35), std::make_shared<std::string>("Pedro Luís & A Parede") });
    storage.replace(Artist{ std::make_shared<int>(36), std::make_shared<std::string>("O Rappa") });
    storage.replace(Artist{ std::make_shared<int>(37), std::make_shared<std::string>("Ed Motta") });
    storage.replace(Artist{ std::make_shared<int>(38), std::make_shared<std::string>("Banda Black Rio") });
    storage.replace(Artist{ std::make_shared<int>(39), std::make_shared<std::string>("Fernanda Porto") });
    storage.replace(Artist{ std::make_shared<int>(40), std::make_shared<std::string>("Os Cariocas") });
    storage.replace(Artist{ std::make_shared<int>(41), std::make_shared<std::string>("Elis Regina") });
    storage.replace(Artist{ std::make_shared<int>(42), std::make_shared<std::string>("Milton Nascimento") });
    storage.replace(Artist{ std::make_shared<int>(43), std::make_shared<std::string>("A Cor Do Som") });
    storage.replace(Artist{ std::make_shared<int>(44), std::make_shared<std::string>("Kid Abelha") });
    storage.replace(Artist{ std::make_shared<int>(45), std::make_shared<std::string>("Sandra De Sá") });
    storage.replace(Artist{ std::make_shared<int>(46), std::make_shared<std::string>("Jorge Ben") });
    storage.replace(Artist{ std::make_shared<int>(47), std::make_shared<std::string>("Hermeto Pascoal") });
    storage.replace(Artist{ std::make_shared<int>(48), std::make_shared<std::string>("Barão Vermelho") });
    storage.replace(Artist{ std::make_shared<int>(49), std::make_shared<std::string>("Edson, DJ Marky & DJ Patife Featuring Fernanda Porto") });
    storage.replace(Artist{ std::make_shared<int>(50), std::make_shared<std::string>("Metallica") });
    storage.replace(Artist{ std::make_shared<int>(51), std::make_shared<std::string>("Queen") });
    storage.replace(Artist{ std::make_shared<int>(52), std::make_shared<std::string>("Kiss") });
    storage.replace(Artist{ std::make_shared<int>(53), std::make_shared<std::string>("Spyro Gyra") });
    storage.replace(Artist{ std::make_shared<int>(54), std::make_shared<std::string>("Green Day") });
    storage.replace(Artist{ std::make_shared<int>(55), std::make_shared<std::string>("David Coverdale") });
    storage.replace(Artist{ std::make_shared<int>(56), std::make_shared<std::string>("Gonzaguinha") });
    storage.replace(Artist{ std::make_shared<int>(57), std::make_shared<std::string>("Os Mutantes") });
    storage.replace(Artist{ std::make_shared<int>(58), std::make_shared<std::string>("Deep Purple") });
    storage.replace(Artist{ std::make_shared<int>(59), std::make_shared<std::string>("Santana") });
    storage.replace(Artist{ std::make_shared<int>(60), std::make_shared<std::string>("Santana Feat. Dave Matthews") });
    storage.replace(Artist{ std::make_shared<int>(61), std::make_shared<std::string>("Santana Feat. Everlast") });
    storage.replace(Artist{ std::make_shared<int>(62), std::make_shared<std::string>("Santana Feat. Rob Thomas") });
    storage.replace(Artist{ std::make_shared<int>(63), std::make_shared<std::string>("Santana Feat. Lauryn Hill & Cee-Lo") });
    storage.replace(Artist{ std::make_shared<int>(64), std::make_shared<std::string>("Santana Feat. The Project G&B") });
    storage.replace(Artist{ std::make_shared<int>(65), std::make_shared<std::string>("Santana Feat. Maná") });
    storage.replace(Artist{ std::make_shared<int>(66), std::make_shared<std::string>("Santana Feat. Eagle-Eye Cherry") });
    storage.replace(Artist{ std::make_shared<int>(67), std::make_shared<std::string>("Santana Feat. Eric Clapton") });
    storage.replace(Artist{ std::make_shared<int>(68), std::make_shared<std::string>("Miles Davis") });
    storage.replace(Artist{ std::make_shared<int>(69), std::make_shared<std::string>("Gene Krupa") });
    storage.replace(Artist{ std::make_shared<int>(70), std::make_shared<std::string>("Toquinho & Vinícius") });
    storage.replace(Artist{ std::make_shared<int>(71), std::make_shared<std::string>("Vinícius De Moraes & Baden Powell") });
    storage.replace(Artist{ std::make_shared<int>(72), std::make_shared<std::string>("Vinícius De Moraes") });
    storage.replace(Artist{ std::make_shared<int>(73), std::make_shared<std::string>("Vinícius E Qurteto Em Cy") });
    storage.replace(Artist{ std::make_shared<int>(74), std::make_shared<std::string>("Vinícius E Odette Lara") });
    storage.replace(Artist{ std::make_shared<int>(75), std::make_shared<std::string>("Vinicius, Toquinho & Quarteto Em Cy") });
    storage.replace(Artist{ std::make_shared<int>(76), std::make_shared<std::string>("Creedence Clearwater Revival") });
    storage.replace(Artist{ std::make_shared<int>(77), std::make_shared<std::string>("Cássia Eller") });
    storage.replace(Artist{ std::make_shared<int>(78), std::make_shared<std::string>("Def Leppard") });
    storage.replace(Artist{ std::make_shared<int>(79), std::make_shared<std::string>("Dennis Chambers") });
    storage.replace(Artist{ std::make_shared<int>(80), std::make_shared<std::string>("Djavan") });
    storage.replace(Artist{ std::make_shared<int>(81), std::make_shared<std::string>("Eric Clapton") });
    storage.replace(Artist{ std::make_shared<int>(82), std::make_shared<std::string>("Faith No More") });
    storage.replace(Artist{ std::make_shared<int>(83), std::make_shared<std::string>("Falamansa") });
    storage.replace(Artist{ std::make_shared<int>(84), std::make_shared<std::string>("Foo Fighters") });
    storage.replace(Artist{ std::make_shared<int>(85), std::make_shared<std::string>("Frank Sinatra") });
    storage.replace(Artist{ std::make_shared<int>(86), std::make_shared<std::string>("Funk Como Le Gusta") });
    storage.replace(Artist{ std::make_shared<int>(87), std::make_shared<std::string>("Godsmack") });
    storage.replace(Artist{ std::make_shared<int>(88), std::make_shared<std::string>("Guns N' Roses") });
    storage.replace(Artist{ std::make_shared<int>(89), std::make_shared<std::string>("Incognito") });
    storage.replace(Artist{ std::make_shared<int>(90), std::make_shared<std::string>("Iron Maiden") });
    storage.replace(Artist{ std::make_shared<int>(91), std::make_shared<std::string>("James Brown") });
    storage.replace(Artist{ std::make_shared<int>(92), std::make_shared<std::string>("Jamiroquai") });
    storage.replace(Artist{ std::make_shared<int>(93), std::make_shared<std::string>("JET") });
    storage.replace(Artist{ std::make_shared<int>(94), std::make_shared<std::string>("Jimi Hendrix") });
    storage.replace(Artist{ std::make_shared<int>(95), std::make_shared<std::string>("Joe Satriani") });
    storage.replace(Artist{ std::make_shared<int>(96), std::make_shared<std::string>("Jota Quest") });
    storage.replace(Artist{ std::make_shared<int>(97), std::make_shared<std::string>("João Suplicy") });
    storage.replace(Artist{ std::make_shared<int>(98), std::make_shared<std::string>("Judas Priest") });
    storage.replace(Artist{ std::make_shared<int>(99), std::make_shared<std::string>("Legião Urbana") });
    storage.replace(Artist{ std::make_shared<int>(100), std::make_shared<std::string>("Lenny Kravitz") });
    storage.replace(Artist{ std::make_shared<int>(101), std::make_shared<std::string>("Lulu Santos") });
    storage.replace(Artist{ std::make_shared<int>(102), std::make_shared<std::string>("Marillion") });
    storage.replace(Artist{ std::make_shared<int>(103), std::make_shared<std::string>("Marisa Monte") });
    storage.replace(Artist{ std::make_shared<int>(104), std::make_shared<std::string>("Marvin Gaye") });
    storage.replace(Artist{ std::make_shared<int>(105), std::make_shared<std::string>("Men At Work") });
    storage.replace(Artist{ std::make_shared<int>(106), std::make_shared<std::string>("Motörhead") });
    storage.replace(Artist{ std::make_shared<int>(107), std::make_shared<std::string>("Motörhead & Girlschool") });
    storage.replace(Artist{ std::make_shared<int>(108), std::make_shared<std::string>("Mônica Marianno") });
    storage.replace(Artist{ std::make_shared<int>(109), std::make_shared<std::string>("Mötley Crüe") });
    storage.replace(Artist{ std::make_shared<int>(110), std::make_shared<std::string>("Nirvana") });
    storage.replace(Artist{ std::make_shared<int>(111), std::make_shared<std::string>("O Terço") });
    storage.replace(Artist{ std::make_shared<int>(112), std::make_shared<std::string>("Olodum") });
    storage.replace(Artist{ std::make_shared<int>(113), std::make_shared<std::string>("Os Paralamas Do Sucesso") });
    storage.replace(Artist{ std::make_shared<int>(114), std::make_shared<std::string>("Ozzy Osbourne") });
    storage.replace(Artist{ std::make_shared<int>(115), std::make_shared<std::string>("Page & Plant") });
    storage.replace(Artist{ std::make_shared<int>(116), std::make_shared<std::string>("Passengers") });
    storage.replace(Artist{ std::make_shared<int>(117), std::make_shared<std::string>("Paul D'Ianno") });
    storage.replace(Artist{ std::make_shared<int>(118), std::make_shared<std::string>("Pearl Jam") });
    storage.replace(Artist{ std::make_shared<int>(119), std::make_shared<std::string>("Peter Tosh") });
    storage.replace(Artist{ std::make_shared<int>(120), std::make_shared<std::string>("Pink Floyd") });
    storage.replace(Artist{ std::make_shared<int>(121), std::make_shared<std::string>("Planet Hemp") });
    storage.replace(Artist{ std::make_shared<int>(122), std::make_shared<std::string>("R.E.M. Feat. Kate Pearson") });
    storage.replace(Artist{ std::make_shared<int>(123), std::make_shared<std::string>("R.E.M. Feat. KRS-One") });
    storage.replace(Artist{ std::make_shared<int>(124), std::make_shared<std::string>("R.E.M.") });
    storage.replace(Artist{ std::make_shared<int>(125), std::make_shared<std::string>("Raimundos") });
    storage.replace(Artist{ std::make_shared<int>(126), std::make_shared<std::string>("Raul Seixas") });
    storage.replace(Artist{ std::make_shared<int>(127), std::make_shared<std::string>("Red Hot Chili Peppers") });
    storage.replace(Artist{ std::make_shared<int>(128), std::make_shared<std::string>("Rush") });
    storage.replace(Artist{ std::make_shared<int>(129), std::make_shared<std::string>("Simply Red") });
    storage.replace(Artist{ std::make_shared<int>(130), std::make_shared<std::string>("Skank") });
    storage.replace(Artist{ std::make_shared<int>(131), std::make_shared<std::string>("Smashing Pumpkins") });
    storage.replace(Artist{ std::make_shared<int>(132), std::make_shared<std::string>("Soundgarden") });
    storage.replace(Artist{ std::make_shared<int>(133), std::make_shared<std::string>("Stevie Ray Vaughan & Double Trouble") });
    storage.replace(Artist{ std::make_shared<int>(134), std::make_shared<std::string>("Stone Temple Pilots") });
    storage.replace(Artist{ std::make_shared<int>(135), std::make_shared<std::string>("System Of A Down") });
    storage.replace(Artist{ std::make_shared<int>(136), std::make_shared<std::string>("Terry Bozzio, Tony Levin & Steve Stevens") });
    storage.replace(Artist{ std::make_shared<int>(137), std::make_shared<std::string>("The Black Crowes") });
    storage.replace(Artist{ std::make_shared<int>(138), std::make_shared<std::string>("The Clash") });
    storage.replace(Artist{ std::make_shared<int>(139), std::make_shared<std::string>("The Cult") });
    storage.replace(Artist{ std::make_shared<int>(140), std::make_shared<std::string>("The Doors") });
    storage.replace(Artist{ std::make_shared<int>(141), std::make_shared<std::string>("The Police") });
    storage.replace(Artist{ std::make_shared<int>(142), std::make_shared<std::string>("The Rolling Stones") });
    storage.replace(Artist{ std::make_shared<int>(143), std::make_shared<std::string>("The Tea Party") });
    storage.replace(Artist{ std::make_shared<int>(144), std::make_shared<std::string>("The Who") });
    storage.replace(Artist{ std::make_shared<int>(145), std::make_shared<std::string>("Tim Maia") });
    storage.replace(Artist{ std::make_shared<int>(146), std::make_shared<std::string>("Titãs") });
    storage.replace(Artist{ std::make_shared<int>(147), std::make_shared<std::string>("Battlestar Galactica") });
    storage.replace(Artist{ std::make_shared<int>(148), std::make_shared<std::string>("Heroes") });
    storage.replace(Artist{ std::make_shared<int>(149), std::make_shared<std::string>("Lost") });
    storage.replace(Artist{ std::make_shared<int>(150), std::make_shared<std::string>("U2") });
    storage.replace(Artist{ std::make_shared<int>(151), std::make_shared<std::string>("UB40") });
    storage.replace(Artist{ std::make_shared<int>(152), std::make_shared<std::string>("Van Halen") });
    storage.replace(Artist{ std::make_shared<int>(153), std::make_shared<std::string>("Velvet Revolver") });
    storage.replace(Artist{ std::make_shared<int>(154), std::make_shared<std::string>("Whitesnake") });
    storage.replace(Artist{ std::make_shared<int>(155), std::make_shared<std::string>("Zeca Pagodinho") });
    storage.replace(Artist{ std::make_shared<int>(156), std::make_shared<std::string>("The Office") });
    storage.replace(Artist{ std::make_shared<int>(157), std::make_shared<std::string>("Dread Zeppelin") });
    storage.replace(Artist{ std::make_shared<int>(158), std::make_shared<std::string>("Battlestar Galactica (Classic)") });
    storage.replace(Artist{ std::make_shared<int>(159), std::make_shared<std::string>("Aquaman") });
    storage.replace(Artist{ std::make_shared<int>(160), std::make_shared<std::string>("Christina Aguilera featuring BigElf") });
    storage.replace(Artist{ std::make_shared<int>(161), std::make_shared<std::string>("Aerosmith & Sierra Leone's Refugee Allstars") });
    storage.replace(Artist{ std::make_shared<int>(162), std::make_shared<std::string>("Los Lonely Boys") });
    storage.replace(Artist{ std::make_shared<int>(163), std::make_shared<std::string>("Corinne Bailey Rae") });
    storage.replace(Artist{ std::make_shared<int>(164), std::make_shared<std::string>("Dhani Harrison & Jakob Dylan") });
    storage.replace(Artist{ std::make_shared<int>(165), std::make_shared<std::string>("Jackson Browne") });
    storage.replace(Artist{ std::make_shared<int>(166), std::make_shared<std::string>("Avril Lavigne") });
    storage.replace(Artist{ std::make_shared<int>(167), std::make_shared<std::string>("Big & Rich") });
    storage.replace(Artist{ std::make_shared<int>(168), std::make_shared<std::string>("Youssou N'Dour") });
    storage.replace(Artist{ std::make_shared<int>(169), std::make_shared<std::string>("Black Eyed Peas") });
    storage.replace(Artist{ std::make_shared<int>(170), std::make_shared<std::string>("Jack Johnson") });
    storage.replace(Artist{ std::make_shared<int>(171), std::make_shared<std::string>("Ben Harper") });
    storage.replace(Artist{ std::make_shared<int>(172), std::make_shared<std::string>("Snow Patrol") });
    storage.replace(Artist{ std::make_shared<int>(173), std::make_shared<std::string>("Matisyahu") });
    storage.replace(Artist{ std::make_shared<int>(174), std::make_shared<std::string>("The Postal Service") });
    storage.replace(Artist{ std::make_shared<int>(175), std::make_shared<std::string>("Jaguares") });
    storage.replace(Artist{ std::make_shared<int>(176), std::make_shared<std::string>("The Flaming Lips") });
    storage.replace(Artist{ std::make_shared<int>(177), std::make_shared<std::string>("Jack's Mannequin & Mick Fleetwood") });
    storage.replace(Artist{ std::make_shared<int>(178), std::make_shared<std::string>("Regina Spektor") });
    storage.replace(Artist{ std::make_shared<int>(179), std::make_shared<std::string>("Scorpions") });
    storage.replace(Artist{ std::make_shared<int>(180), std::make_shared<std::string>("House Of Pain") });
    storage.replace(Artist{ std::make_shared<int>(181), std::make_shared<std::string>("Xis") });
    storage.replace(Artist{ std::make_shared<int>(182), std::make_shared<std::string>("Nega Gizza") });
    storage.replace(Artist{ std::make_shared<int>(183), std::make_shared<std::string>("Gustavo & Andres Veiga & Salazar") });
    storage.replace(Artist{ std::make_shared<int>(184), std::make_shared<std::string>("Rodox") });
    storage.replace(Artist{ std::make_shared<int>(185), std::make_shared<std::string>("Charlie Brown Jr.") });
    storage.replace(Artist{ std::make_shared<int>(186), std::make_shared<std::string>("Pedro Luís E A Parede") });
    storage.replace(Artist{ std::make_shared<int>(187), std::make_shared<std::string>("Los Hermanos") });
    storage.replace(Artist{ std::make_shared<int>(188), std::make_shared<std::string>("Mundo Livre S/A") });
    storage.replace(Artist{ std::make_shared<int>(189), std::make_shared<std::string>("Otto") });
    storage.replace(Artist{ std::make_shared<int>(190), std::make_shared<std::string>("Instituto") });
    storage.replace(Artist{ std::make_shared<int>(191), std::make_shared<std::string>("Nação Zumbi") });
    storage.replace(Artist{ std::make_shared<int>(192), std::make_shared<std::string>("DJ Dolores & Orchestra Santa Massa") });
    storage.replace(Artist{ std::make_shared<int>(193), std::make_shared<std::string>("Seu Jorge") });
    storage.replace(Artist{ std::make_shared<int>(194), std::make_shared<std::string>("Sabotage E Instituto") });
    storage.replace(Artist{ std::make_shared<int>(195), std::make_shared<std::string>("Stereo Maracana") });
    storage.replace(Artist{ std::make_shared<int>(196), std::make_shared<std::string>("Cake") });
    storage.replace(Artist{ std::make_shared<int>(197), std::make_shared<std::string>("Aisha Duo") });
    storage.replace(Artist{ std::make_shared<int>(198), std::make_shared<std::string>("Habib Koité and Bamada") });
    storage.replace(Artist{ std::make_shared<int>(199), std::make_shared<std::string>("Karsh Kale") });
    storage.replace(Artist{ std::make_shared<int>(200), std::make_shared<std::string>("The Posies") });
    storage.replace(Artist{ std::make_shared<int>(201), std::make_shared<std::string>("Luciana Souza/Romero Lubambo") });
    storage.replace(Artist{ std::make_shared<int>(202), std::make_shared<std::string>("Aaron Goldberg") });
    storage.replace(Artist{ std::make_shared<int>(203), std::make_shared<std::string>("Nicolaus Esterhazy Sinfonia") });
    storage.replace(Artist{ std::make_shared<int>(204), std::make_shared<std::string>("Temple of the Dog") });
    storage.replace(Artist{ std::make_shared<int>(205), std::make_shared<std::string>("Chris Cornell") });
    storage.replace(Artist{ std::make_shared<int>(206), std::make_shared<std::string>("Alberto Turco & Nova Schola Gregoriana") });
    storage.replace(Artist{ std::make_shared<int>(207), std::make_shared<std::string>("Richard Marlow & The Choir of Trinity College, Cambridge") });
    storage.replace(Artist{ std::make_shared<int>(208), std::make_shared<std::string>("English Concert & Trevor Pinnock") });
    storage.replace(Artist{ std::make_shared<int>(209), std::make_shared<std::string>("Anne-Sophie Mutter, Herbert Von Karajan & Wiener Philharmoniker") });
    storage.replace(Artist{ std::make_shared<int>(210), std::make_shared<std::string>("Hilary Hahn, Jeffrey Kahane, Los Angeles Chamber Orchestra & Margaret Batjer") });
    storage.replace(Artist{ std::make_shared<int>(211), std::make_shared<std::string>("Wilhelm Kempff") });
    storage.replace(Artist{ std::make_shared<int>(212), std::make_shared<std::string>("Yo-Yo Ma") });
    storage.replace(Artist{ std::make_shared<int>(213), std::make_shared<std::string>("Scholars Baroque Ensemble") });
    storage.replace(Artist{ std::make_shared<int>(214), std::make_shared<std::string>("Academy of St. Martin in the Fields & Sir Neville Marriner") });
    storage.replace(Artist{ std::make_shared<int>(215), std::make_shared<std::string>("Academy of St. Martin in the Fields Chamber Ensemble & Sir Neville Marriner") });
    storage.replace(Artist{ std::make_shared<int>(216), std::make_shared<std::string>("Berliner Philharmoniker, Claudio Abbado & Sabine Meyer") });
    storage.replace(Artist{ std::make_shared<int>(217), std::make_shared<std::string>("Royal Philharmonic Orchestra & Sir Thomas Beecham") });
    storage.replace(Artist{ std::make_shared<int>(218), std::make_shared<std::string>("Orchestre Révolutionnaire et Romantique & John Eliot Gardiner") });
    storage.replace(Artist{ std::make_shared<int>(219), std::make_shared<std::string>("Britten Sinfonia, Ivor Bolton & Lesley Garrett") });
    storage.replace(Artist{ std::make_shared<int>(220), std::make_shared<std::string>("Chicago Symphony Chorus, Chicago Symphony Orchestra & Sir Georg Solti") });
    storage.replace(Artist{ std::make_shared<int>(221), std::make_shared<std::string>("Sir Georg Solti & Wiener Philharmoniker") });
    storage.replace(Artist{ std::make_shared<int>(222), std::make_shared<std::string>("Academy of St. Martin in the Fields, John Birch, Sir Neville Marriner & Sylvia McNair") });
    storage.replace(Artist{ std::make_shared<int>(223), std::make_shared<std::string>("London Symphony Orchestra & Sir Charles Mackerras") });
    storage.replace(Artist{ std::make_shared<int>(224), std::make_shared<std::string>("Barry Wordsworth & BBC Concert Orchestra") });
    storage.replace(Artist{ std::make_shared<int>(225), std::make_shared<std::string>("Herbert Von Karajan, Mirella Freni & Wiener Philharmoniker") });
    storage.replace(Artist{ std::make_shared<int>(226), std::make_shared<std::string>("Eugene Ormandy") });
    storage.replace(Artist{ std::make_shared<int>(227), std::make_shared<std::string>("Luciano Pavarotti") });
    storage.replace(Artist{ std::make_shared<int>(228), std::make_shared<std::string>("Leonard Bernstein & New York Philharmonic") });
    storage.replace(Artist{ std::make_shared<int>(229), std::make_shared<std::string>("Boston Symphony Orchestra & Seiji Ozawa") });
    storage.replace(Artist{ std::make_shared<int>(230), std::make_shared<std::string>("Aaron Copland & London Symphony Orchestra") });
    storage.replace(Artist{ std::make_shared<int>(231), std::make_shared<std::string>("Ton Koopman") });
    storage.replace(Artist{ std::make_shared<int>(232), std::make_shared<std::string>("Sergei Prokofiev & Yuri Temirkanov") });
    storage.replace(Artist{ std::make_shared<int>(233), std::make_shared<std::string>("Chicago Symphony Orchestra & Fritz Reiner") });
    storage.replace(Artist{ std::make_shared<int>(234), std::make_shared<std::string>("Orchestra of The Age of Enlightenment") });
    storage.replace(Artist{ std::make_shared<int>(235), std::make_shared<std::string>("Emanuel Ax, Eugene Ormandy & Philadelphia Orchestra") });
    storage.replace(Artist{ std::make_shared<int>(236), std::make_shared<std::string>("James Levine") });
    storage.replace(Artist{ std::make_shared<int>(237), std::make_shared<std::string>("Berliner Philharmoniker & Hans Rosbaud") });
    storage.replace(Artist{ std::make_shared<int>(238), std::make_shared<std::string>("Maurizio Pollini") });
    storage.replace(Artist{ std::make_shared<int>(239), std::make_shared<std::string>("Academy of St. Martin in the Fields, Sir Neville Marriner & William Bennett") });
    storage.replace(Artist{ std::make_shared<int>(240), std::make_shared<std::string>("Gustav Mahler") });
    storage.replace(Artist{ std::make_shared<int>(241), std::make_shared<std::string>("Felix Schmidt, London Symphony Orchestra & Rafael Frühbeck de Burgos") });
    storage.replace(Artist{ std::make_shared<int>(242), std::make_shared<std::string>("Edo de Waart & San Francisco Symphony") });
    storage.replace(Artist{ std::make_shared<int>(243), std::make_shared<std::string>("Antal Doráti & London Symphony Orchestra") });
    storage.replace(Artist{ std::make_shared<int>(244), std::make_shared<std::string>("Choir Of Westminster Abbey & Simon Preston") });
    storage.replace(Artist{ std::make_shared<int>(245), std::make_shared<std::string>("Michael Tilson Thomas & San Francisco Symphony") });
    storage.replace(Artist{ std::make_shared<int>(246), std::make_shared<std::string>("Chor der Wiener Staatsoper, Herbert Von Karajan & Wiener Philharmoniker") });
    storage.replace(Artist{ std::make_shared<int>(247), std::make_shared<std::string>("The King's Singers") });
    storage.replace(Artist{ std::make_shared<int>(248), std::make_shared<std::string>("Berliner Philharmoniker & Herbert Von Karajan") });
    storage.replace(Artist{ std::make_shared<int>(249), std::make_shared<std::string>("Sir Georg Solti, Sumi Jo & Wiener Philharmoniker") });
    storage.replace(Artist{ std::make_shared<int>(250), std::make_shared<std::string>("Christopher O'Riley") });
    storage.replace(Artist{ std::make_shared<int>(251), std::make_shared<std::string>("Fretwork") });
    storage.replace(Artist{ std::make_shared<int>(252), std::make_shared<std::string>("Amy Winehouse") });
    storage.replace(Artist{ std::make_shared<int>(253), std::make_shared<std::string>("Calexico") });
    storage.replace(Artist{ std::make_shared<int>(254), std::make_shared<std::string>("Otto Klemperer & Philharmonia Orchestra") });
    storage.replace(Artist{ std::make_shared<int>(255), std::make_shared<std::string>("Yehudi Menuhin") });
    storage.replace(Artist{ std::make_shared<int>(256), std::make_shared<std::string>("Philharmonia Orchestra & Sir Neville Marriner") });
    storage.replace(Artist{ std::make_shared<int>(257), std::make_shared<std::string>("Academy of St. Martin in the Fields, Sir Neville Marriner & Thurston Dart") });
    storage.replace(Artist{ std::make_shared<int>(258), std::make_shared<std::string>("Les Arts Florissants & William Christie") });
    storage.replace(Artist{ std::make_shared<int>(259), std::make_shared<std::string>("The 12 Cellists of The Berlin Philharmonic") });
    storage.replace(Artist{ std::make_shared<int>(260), std::make_shared<std::string>("Adrian Leaper & Doreen de Feis") });
    storage.replace(Artist{ std::make_shared<int>(261), std::make_shared<std::string>("Roger Norrington, London Classical Players") });
    storage.replace(Artist{ std::make_shared<int>(262), std::make_shared<std::string>("Charles Dutoit & L'Orchestre Symphonique de Montréal") });
    storage.replace(Artist{ std::make_shared<int>(263), std::make_shared<std::string>("Equale Brass Ensemble, John Eliot Gardiner & Munich Monteverdi Orchestra and Choir") });
    storage.replace(Artist{ std::make_shared<int>(264), std::make_shared<std::string>("Kent Nagano and Orchestre de l'Opéra de Lyon") });
    storage.replace(Artist{ std::make_shared<int>(265), std::make_shared<std::string>("Julian Bream") });
    storage.replace(Artist{ std::make_shared<int>(266), std::make_shared<std::string>("Martin Roscoe") });
    storage.replace(Artist{ std::make_shared<int>(267), std::make_shared<std::string>("Göteborgs Symfoniker & Neeme Järvi") });
    storage.replace(Artist{ std::make_shared<int>(268), std::make_shared<std::string>("Itzhak Perlman") });
    storage.replace(Artist{ std::make_shared<int>(269), std::make_shared<std::string>("Michele Campanella") });
    storage.replace(Artist{ std::make_shared<int>(270), std::make_shared<std::string>("Gerald Moore") });
    storage.replace(Artist{ std::make_shared<int>(271), std::make_shared<std::string>("Mela Tenenbaum, Pro Musica Prague & Richard Kapp") });
    storage.replace(Artist{ std::make_shared<int>(272), std::make_shared<std::string>("Emerson String Quartet") });
    storage.replace(Artist{ std::make_shared<int>(273), std::make_shared<std::string>("C. Monteverdi, Nigel Rogers - Chiaroscuro; London Baroque; London Cornett & Sackbu") });
    storage.replace(Artist{ std::make_shared<int>(274), std::make_shared<std::string>("Nash Ensemble") });
    storage.replace(Artist{ std::make_shared<int>(275), std::make_shared<std::string>("Philip Glass Ensemble") });
    storage.replace(Album{ std::make_shared<int>(1), std::make_shared<std::string>("For Those About To Rock We Salute You"), std::make_shared<int>(1) });
    storage.replace(Album{ std::make_shared<int>(2), std::make_shared<std::string>("Balls to the Wall"), std::make_shared<int>(2) });
    storage.replace(Album{ std::make_shared<int>(3), std::make_shared<std::string>("Restless and Wild"), std::make_shared<int>(2) });
    storage.replace(Album{ std::make_shared<int>(4), std::make_shared<std::string>("Let There Be Rock"), std::make_shared<int>(1) });
    storage.replace(Album{ std::make_shared<int>(5), std::make_shared<std::string>("Big Ones"), std::make_shared<int>(3) });
    storage.replace(Album{ std::make_shared<int>(6), std::make_shared<std::string>("Jagged Little Pill"), std::make_shared<int>(4) });
    storage.replace(Album{ std::make_shared<int>(7), std::make_shared<std::string>("Facelift"), std::make_shared<int>(5) });
    storage.replace(Album{ std::make_shared<int>(8), std::make_shared<std::string>("Warner 25 Anos"), std::make_shared<int>(6) });
    storage.replace(Album{ std::make_shared<int>(9), std::make_shared<std::string>("Plays Metallica By Four Cellos"), std::make_shared<int>(7) });
    storage.replace(Album{ std::make_shared<int>(10), std::make_shared<std::string>("Audioslave"), std::make_shared<int>(8) });
    storage.replace(Album{ std::make_shared<int>(11), std::make_shared<std::string>("Out Of Exile"), std::make_shared<int>(8) });
    storage.replace(Album{ std::make_shared<int>(12), std::make_shared<std::string>("BackBeat Soundtrack"), std::make_shared<int>(9) });
    storage.replace(Album{ std::make_shared<int>(13), std::make_shared<std::string>("The Best Of Billy Cobham"), std::make_shared<int>(10) });
    storage.replace(Album{ std::make_shared<int>(14), std::make_shared<std::string>("Alcohol Fueled Brewtality Live! [Disc 1]"), std::make_shared<int>(11) });
    storage.replace(Album{ std::make_shared<int>(15), std::make_shared<std::string>("Alcohol Fueled Brewtality Live! [Disc 2]"), std::make_shared<int>(11) });
    storage.replace(Album{ std::make_shared<int>(16), std::make_shared<std::string>("Black Sabbath"), std::make_shared<int>(12) });
    storage.replace(Album{ std::make_shared<int>(17), std::make_shared<std::string>("Black Sabbath Vol. 4 (Remaster)"), std::make_shared<int>(12) });
    storage.replace(Album{ std::make_shared<int>(18), std::make_shared<std::string>("Body Count"), std::make_shared<int>(13) });
    storage.replace(Album{ std::make_shared<int>(19), std::make_shared<std::string>("Chemical Wedding"), std::make_shared<int>(14) });
    storage.replace(Album{ std::make_shared<int>(20), std::make_shared<std::string>("The Best Of Buddy Guy - The Millenium Collection"), std::make_shared<int>(15) });
    storage.replace(Album{ std::make_shared<int>(21), std::make_shared<std::string>("Prenda Minha"), std::make_shared<int>(16) });
    storage.replace(Album{ std::make_shared<int>(22), std::make_shared<std::string>("Sozinho Remix Ao Vivo"), std::make_shared<int>(16) });
    storage.replace(Album{ std::make_shared<int>(23), std::make_shared<std::string>("Minha Historia"), std::make_shared<int>(17) });
    storage.replace(Album{ std::make_shared<int>(24), std::make_shared<std::string>("Afrociberdelia"), std::make_shared<int>(18) });
    storage.replace(Album{ std::make_shared<int>(25), std::make_shared<std::string>("Da Lama Ao Caos"), std::make_shared<int>(18) });
    storage.replace(Album{ std::make_shared<int>(26), std::make_shared<std::string>("Acústico MTV [Live]"), std::make_shared<int>(19) });
    storage.replace(Album{ std::make_shared<int>(27), std::make_shared<std::string>("Cidade Negra - Hits"), std::make_shared<int>(19) });
    storage.replace(Album{ std::make_shared<int>(28), std::make_shared<std::string>("Na Pista"), std::make_shared<int>(20) });
    storage.replace(Album{ std::make_shared<int>(29), std::make_shared<std::string>("Axé Bahia 2001"), std::make_shared<int>(21) });
    storage.replace(Album{ std::make_shared<int>(30), std::make_shared<std::string>("BBC Sessions [Disc 1] [Live]"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(31), std::make_shared<std::string>("Bongo Fury"), std::make_shared<int>(23) });
    storage.replace(Album{ std::make_shared<int>(32), std::make_shared<std::string>("Carnaval 2001"), std::make_shared<int>(21) });
    storage.replace(Album{ std::make_shared<int>(33), std::make_shared<std::string>("Chill: Brazil (Disc 1)"), std::make_shared<int>(24) });
    storage.replace(Album{ std::make_shared<int>(34), std::make_shared<std::string>("Chill: Brazil (Disc 2)"), std::make_shared<int>(6) });
    storage.replace(Album{ std::make_shared<int>(35), std::make_shared<std::string>("Garage Inc. (Disc 1)"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(36), std::make_shared<std::string>("Greatest Hits II"), std::make_shared<int>(51) });
    storage.replace(Album{ std::make_shared<int>(37), std::make_shared<std::string>("Greatest Kiss"), std::make_shared<int>(52) });
    storage.replace(Album{ std::make_shared<int>(38), std::make_shared<std::string>("Heart of the Night"), std::make_shared<int>(53) });
    storage.replace(Album{ std::make_shared<int>(39), std::make_shared<std::string>("International Superhits"), std::make_shared<int>(54) });
    storage.replace(Album{ std::make_shared<int>(40), std::make_shared<std::string>("Into The Light"), std::make_shared<int>(55) });
    storage.replace(Album{ std::make_shared<int>(41), std::make_shared<std::string>("Meus Momentos"), std::make_shared<int>(56) });
    storage.replace(Album{ std::make_shared<int>(42), std::make_shared<std::string>("Minha História"), std::make_shared<int>(57) });
    storage.replace(Album{ std::make_shared<int>(43), std::make_shared<std::string>("MK III The Final Concerts [Disc 1]"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(44), std::make_shared<std::string>("Physical Graffiti [Disc 1]"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(45), std::make_shared<std::string>("Sambas De Enredo 2001"), std::make_shared<int>(21) });
    storage.replace(Album{ std::make_shared<int>(46), std::make_shared<std::string>("Supernatural"), std::make_shared<int>(59) });
    storage.replace(Album{ std::make_shared<int>(47), std::make_shared<std::string>("The Best of Ed Motta"), std::make_shared<int>(37) });
    storage.replace(Album{ std::make_shared<int>(48), std::make_shared<std::string>("The Essential Miles Davis [Disc 1]"), std::make_shared<int>(68) });
    storage.replace(Album{ std::make_shared<int>(49), std::make_shared<std::string>("The Essential Miles Davis [Disc 2]"), std::make_shared<int>(68) });
    storage.replace(Album{ std::make_shared<int>(50), std::make_shared<std::string>("The Final Concerts (Disc 2)"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(51), std::make_shared<std::string>("Up An' Atom"), std::make_shared<int>(69) });
    storage.replace(Album{ std::make_shared<int>(52), std::make_shared<std::string>("Vinícius De Moraes - Sem Limite"), std::make_shared<int>(70) });
    storage.replace(Album{ std::make_shared<int>(53), std::make_shared<std::string>("Vozes do MPB"), std::make_shared<int>(21) });
    storage.replace(Album{ std::make_shared<int>(54), std::make_shared<std::string>("Chronicle, Vol. 1"), std::make_shared<int>(76) });
    storage.replace(Album{ std::make_shared<int>(55), std::make_shared<std::string>("Chronicle, Vol. 2"), std::make_shared<int>(76) });
    storage.replace(Album{ std::make_shared<int>(56), std::make_shared<std::string>("Cássia Eller - Coleção Sem Limite [Disc 2]"), std::make_shared<int>(77) });
    storage.replace(Album{ std::make_shared<int>(57), std::make_shared<std::string>("Cássia Eller - Sem Limite [Disc 1]"), std::make_shared<int>(77) });
    storage.replace(Album{ std::make_shared<int>(58), std::make_shared<std::string>("Come Taste The Band"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(59), std::make_shared<std::string>("Deep Purple In Rock"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(60), std::make_shared<std::string>("Fireball"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(61), std::make_shared<std::string>("Knocking at Your Back Door: The Best Of Deep Purple in the 80's"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(62), std::make_shared<std::string>("Machine Head"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(63), std::make_shared<std::string>("Purpendicular"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(64), std::make_shared<std::string>("Slaves And Masters"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(65), std::make_shared<std::string>("Stormbringer"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(66), std::make_shared<std::string>("The Battle Rages On"), std::make_shared<int>(58) });
    storage.replace(Album{ std::make_shared<int>(67), std::make_shared<std::string>("Vault: Def Leppard's Greatest Hits"), std::make_shared<int>(78) });
    storage.replace(Album{ std::make_shared<int>(68), std::make_shared<std::string>("Outbreak"), std::make_shared<int>(79) });
    storage.replace(Album{ std::make_shared<int>(69), std::make_shared<std::string>("Djavan Ao Vivo - Vol. 02"), std::make_shared<int>(80) });
    storage.replace(Album{ std::make_shared<int>(70), std::make_shared<std::string>("Djavan Ao Vivo - Vol. 1"), std::make_shared<int>(80) });
    storage.replace(Album{ std::make_shared<int>(71), std::make_shared<std::string>("Elis Regina-Minha História"), std::make_shared<int>(41) });
    storage.replace(Album{ std::make_shared<int>(72), std::make_shared<std::string>("The Cream Of Clapton"), std::make_shared<int>(81) });
    storage.replace(Album{ std::make_shared<int>(73), std::make_shared<std::string>("Unplugged"), std::make_shared<int>(81) });
    storage.replace(Album{ std::make_shared<int>(74), std::make_shared<std::string>("Album Of The Year"), std::make_shared<int>(82) });
    storage.replace(Album{ std::make_shared<int>(75), std::make_shared<std::string>("Angel Dust"), std::make_shared<int>(82) });
    storage.replace(Album{ std::make_shared<int>(76), std::make_shared<std::string>("King For A Day Fool For A Lifetime"), std::make_shared<int>(82) });
    storage.replace(Album{ std::make_shared<int>(77), std::make_shared<std::string>("The Real Thing"), std::make_shared<int>(82) });
    storage.replace(Album{ std::make_shared<int>(78), std::make_shared<std::string>("Deixa Entrar"), std::make_shared<int>(83) });
    storage.replace(Album{ std::make_shared<int>(79), std::make_shared<std::string>("In Your Honor [Disc 1]"), std::make_shared<int>(84) });
    storage.replace(Album{ std::make_shared<int>(80), std::make_shared<std::string>("In Your Honor [Disc 2]"), std::make_shared<int>(84) });
    storage.replace(Album{ std::make_shared<int>(81), std::make_shared<std::string>("One By One"), std::make_shared<int>(84) });
    storage.replace(Album{ std::make_shared<int>(82), std::make_shared<std::string>("The Colour And The Shape"), std::make_shared<int>(84) });
    storage.replace(Album{ std::make_shared<int>(83), std::make_shared<std::string>("My Way: The Best Of Frank Sinatra [Disc 1]"), std::make_shared<int>(85) });
    storage.replace(Album{ std::make_shared<int>(84), std::make_shared<std::string>("Roda De Funk"), std::make_shared<int>(86) });
    storage.replace(Album{ std::make_shared<int>(85), std::make_shared<std::string>("As Canções de Eu Tu Eles"), std::make_shared<int>(27) });
    storage.replace(Album{ std::make_shared<int>(86), std::make_shared<std::string>("Quanta Gente Veio Ver (Live)"), std::make_shared<int>(27) });
    storage.replace(Album{ std::make_shared<int>(87), std::make_shared<std::string>("Quanta Gente Veio ver--Bônus De Carnaval"), std::make_shared<int>(27) });
    storage.replace(Album{ std::make_shared<int>(88), std::make_shared<std::string>("Faceless"), std::make_shared<int>(87) });
    storage.replace(Album{ std::make_shared<int>(89), std::make_shared<std::string>("American Idiot"), std::make_shared<int>(54) });
    storage.replace(Album{ std::make_shared<int>(90), std::make_shared<std::string>("Appetite for Destruction"), std::make_shared<int>(88) });
    storage.replace(Album{ std::make_shared<int>(91), std::make_shared<std::string>("Use Your Illusion I"), std::make_shared<int>(88) });
    storage.replace(Album{ std::make_shared<int>(92), std::make_shared<std::string>("Use Your Illusion II"), std::make_shared<int>(88) });
    storage.replace(Album{ std::make_shared<int>(93), std::make_shared<std::string>("Blue Moods"), std::make_shared<int>(89) });
    storage.replace(Album{ std::make_shared<int>(94), std::make_shared<std::string>("A Matter of Life and Death"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(95), std::make_shared<std::string>("A Real Dead One"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(96), std::make_shared<std::string>("A Real Live One"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(97), std::make_shared<std::string>("Brave New World"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(98), std::make_shared<std::string>("Dance Of Death"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(99), std::make_shared<std::string>("Fear Of The Dark"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(100), std::make_shared<std::string>("Iron Maiden"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(101), std::make_shared<std::string>("Killers"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(102), std::make_shared<std::string>("Live After Death"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(103), std::make_shared<std::string>("Live At Donington 1992 (Disc 1)"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(104), std::make_shared<std::string>("Live At Donington 1992 (Disc 2)"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(105), std::make_shared<std::string>("No Prayer For The Dying"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(106), std::make_shared<std::string>("Piece Of Mind"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(107), std::make_shared<std::string>("Powerslave"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(108), std::make_shared<std::string>("Rock In Rio [CD1]"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(109), std::make_shared<std::string>("Rock In Rio [CD2]"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(110), std::make_shared<std::string>("Seventh Son of a Seventh Son"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(111), std::make_shared<std::string>("Somewhere in Time"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(112), std::make_shared<std::string>("The Number of The Beast"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(113), std::make_shared<std::string>("The X Factor"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(114), std::make_shared<std::string>("Virtual XI"), std::make_shared<int>(90) });
    storage.replace(Album{ std::make_shared<int>(115), std::make_shared<std::string>("Sex Machine"), std::make_shared<int>(91) });
    storage.replace(Album{ std::make_shared<int>(116), std::make_shared<std::string>("Emergency On Planet Earth"), std::make_shared<int>(92) });
    storage.replace(Album{ std::make_shared<int>(117), std::make_shared<std::string>("Synkronized"), std::make_shared<int>(92) });
    storage.replace(Album{ std::make_shared<int>(118), std::make_shared<std::string>("The Return Of The Space Cowboy"), std::make_shared<int>(92) });
    storage.replace(Album{ std::make_shared<int>(119), std::make_shared<std::string>("Get Born"), std::make_shared<int>(93) });
    storage.replace(Album{ std::make_shared<int>(120), std::make_shared<std::string>("Are You Experienced?"), std::make_shared<int>(94) });
    storage.replace(Album{ std::make_shared<int>(121), std::make_shared<std::string>("Surfing with the Alien (Remastered)"), std::make_shared<int>(95) });
    storage.replace(Album{ std::make_shared<int>(122), std::make_shared<std::string>("Jorge Ben Jor 25 Anos"), std::make_shared<int>(46) });
    storage.replace(Album{ std::make_shared<int>(123), std::make_shared<std::string>("Jota Quest-1995"), std::make_shared<int>(96) });
    storage.replace(Album{ std::make_shared<int>(124), std::make_shared<std::string>("Cafezinho"), std::make_shared<int>(97) });
    storage.replace(Album{ std::make_shared<int>(125), std::make_shared<std::string>("Living After Midnight"), std::make_shared<int>(98) });
    storage.replace(Album{ std::make_shared<int>(126), std::make_shared<std::string>("Unplugged [Live]"), std::make_shared<int>(52) });
    storage.replace(Album{ std::make_shared<int>(127), std::make_shared<std::string>("BBC Sessions [Disc 2] [Live]"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(128), std::make_shared<std::string>("Coda"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(129), std::make_shared<std::string>("Houses Of The Holy"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(130), std::make_shared<std::string>("In Through The Out Door"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(131), std::make_shared<std::string>("IV"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(132), std::make_shared<std::string>("Led Zeppelin I"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(133), std::make_shared<std::string>("Led Zeppelin II"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(134), std::make_shared<std::string>("Led Zeppelin III"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(135), std::make_shared<std::string>("Physical Graffiti [Disc 2]"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(136), std::make_shared<std::string>("Presence"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(137), std::make_shared<std::string>("The Song Remains The Same (Disc 1)"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(138), std::make_shared<std::string>("The Song Remains The Same (Disc 2)"), std::make_shared<int>(22) });
    storage.replace(Album{ std::make_shared<int>(139), std::make_shared<std::string>("A TempestadeTempestade Ou O Livro Dos Dias"), std::make_shared<int>(99) });
    storage.replace(Album{ std::make_shared<int>(140), std::make_shared<std::string>("Mais Do Mesmo"), std::make_shared<int>(99) });
    storage.replace(Album{ std::make_shared<int>(141), std::make_shared<std::string>("Greatest Hits"), std::make_shared<int>(100) });
    storage.replace(Album{ std::make_shared<int>(142), std::make_shared<std::string>("Lulu Santos - RCA 100 Anos De Música - Álbum 01"), std::make_shared<int>(101) });
    storage.replace(Album{ std::make_shared<int>(143), std::make_shared<std::string>("Lulu Santos - RCA 100 Anos De Música - Álbum 02"), std::make_shared<int>(101) });
    storage.replace(Album{ std::make_shared<int>(144), std::make_shared<std::string>("Misplaced Childhood"), std::make_shared<int>(102) });
    storage.replace(Album{ std::make_shared<int>(145), std::make_shared<std::string>("Barulhinho Bom"), std::make_shared<int>(103) });
    storage.replace(Album{ std::make_shared<int>(146), std::make_shared<std::string>("Seek And Shall Find: More Of The Best (1963-1981)"), std::make_shared<int>(104) });
    storage.replace(Album{ std::make_shared<int>(147), std::make_shared<std::string>("The Best Of Men At Work"), std::make_shared<int>(105) });
    storage.replace(Album{ std::make_shared<int>(148), std::make_shared<std::string>("Black Album"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(149), std::make_shared<std::string>("Garage Inc. (Disc 2)"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(150), std::make_shared<std::string>("Kill 'Em All"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(151), std::make_shared<std::string>("Load"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(152), std::make_shared<std::string>("Master Of Puppets"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(153), std::make_shared<std::string>("ReLoad"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(154), std::make_shared<std::string>("Ride The Lightning"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(155), std::make_shared<std::string>("St. Anger"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(156), std::make_shared<std::string>("...And Justice For All"), std::make_shared<int>(50) });
    storage.replace(Album{ std::make_shared<int>(157), std::make_shared<std::string>("Miles Ahead"), std::make_shared<int>(68) });
    storage.replace(Album{ std::make_shared<int>(158), std::make_shared<std::string>("Milton Nascimento Ao Vivo"), std::make_shared<int>(42) });
    storage.replace(Album{ std::make_shared<int>(159), std::make_shared<std::string>("Minas"), std::make_shared<int>(42) });
    storage.replace(Album{ std::make_shared<int>(160), std::make_shared<std::string>("Ace Of Spades"), std::make_shared<int>(106) });
    storage.replace(Album{ std::make_shared<int>(161), std::make_shared<std::string>("Demorou..."), std::make_shared<int>(108) });
    storage.replace(Album{ std::make_shared<int>(162), std::make_shared<std::string>("Motley Crue Greatest Hits"), std::make_shared<int>(109) });
    storage.replace(Album{ std::make_shared<int>(163), std::make_shared<std::string>("From The Muddy Banks Of The Wishkah [Live]"), std::make_shared<int>(110) });
    storage.replace(Album{ std::make_shared<int>(164), std::make_shared<std::string>("Nevermind"), std::make_shared<int>(110) });
    storage.replace(Album{ std::make_shared<int>(165), std::make_shared<std::string>("Compositores"), std::make_shared<int>(111) });
    storage.replace(Album{ std::make_shared<int>(166), std::make_shared<std::string>("Olodum"), std::make_shared<int>(112) });
    storage.replace(Album{ std::make_shared<int>(167), std::make_shared<std::string>("Acústico MTV"), std::make_shared<int>(113) });
    storage.replace(Album{ std::make_shared<int>(168), std::make_shared<std::string>("Arquivo II"), std::make_shared<int>(113) });
    storage.replace(Album{ std::make_shared<int>(169), std::make_shared<std::string>("Arquivo Os Paralamas Do Sucesso"), std::make_shared<int>(113) });
    storage.replace(Album{ std::make_shared<int>(170), std::make_shared<std::string>("Bark at the Moon (Remastered)"), std::make_shared<int>(114) });
    storage.replace(Album{ std::make_shared<int>(171), std::make_shared<std::string>("Blizzard of Ozz"), std::make_shared<int>(114) });
    storage.replace(Album{ std::make_shared<int>(172), std::make_shared<std::string>("Diary of a Madman (Remastered)"), std::make_shared<int>(114) });
    storage.replace(Album{ std::make_shared<int>(173), std::make_shared<std::string>("No More Tears (Remastered)"), std::make_shared<int>(114) });
    storage.replace(Album{ std::make_shared<int>(174), std::make_shared<std::string>("Tribute"), std::make_shared<int>(114) });
    storage.replace(Album{ std::make_shared<int>(175), std::make_shared<std::string>("Walking Into Clarksdale"), std::make_shared<int>(115) });
    storage.replace(Album{ std::make_shared<int>(176), std::make_shared<std::string>("Original Soundtracks 1"), std::make_shared<int>(116) });
    storage.replace(Album{ std::make_shared<int>(177), std::make_shared<std::string>("The Beast Live"), std::make_shared<int>(117) });
    storage.replace(Album{ std::make_shared<int>(178), std::make_shared<std::string>("Live On Two Legs [Live]"), std::make_shared<int>(118) });
    storage.replace(Album{ std::make_shared<int>(179), std::make_shared<std::string>("Pearl Jam"), std::make_shared<int>(118) });
    storage.replace(Album{ std::make_shared<int>(180), std::make_shared<std::string>("Riot Act"), std::make_shared<int>(118) });
    storage.replace(Album{ std::make_shared<int>(181), std::make_shared<std::string>("Ten"), std::make_shared<int>(118) });
    storage.replace(Album{ std::make_shared<int>(182), std::make_shared<std::string>("Vs."), std::make_shared<int>(118) });
    storage.replace(Album{ std::make_shared<int>(183), std::make_shared<std::string>("Dark Side Of The Moon"), std::make_shared<int>(120) });
    storage.replace(Album{ std::make_shared<int>(184), std::make_shared<std::string>("Os Cães Ladram Mas A Caravana Não Pára"), std::make_shared<int>(121) });
    storage.replace(Album{ std::make_shared<int>(185), std::make_shared<std::string>("Greatest Hits I"), std::make_shared<int>(51) });
    storage.replace(Album{ std::make_shared<int>(186), std::make_shared<std::string>("News Of The World"), std::make_shared<int>(51) });
    storage.replace(Album{ std::make_shared<int>(187), std::make_shared<std::string>("Out Of Time"), std::make_shared<int>(122) });
    storage.replace(Album{ std::make_shared<int>(188), std::make_shared<std::string>("Green"), std::make_shared<int>(124) });
    storage.replace(Album{ std::make_shared<int>(189), std::make_shared<std::string>("New Adventures In Hi-Fi"), std::make_shared<int>(124) });
    storage.replace(Album{ std::make_shared<int>(190), std::make_shared<std::string>("The Best Of R.E.M.: The IRS Years"), std::make_shared<int>(124) });
    storage.replace(Album{ std::make_shared<int>(191), std::make_shared<std::string>("Cesta Básica"), std::make_shared<int>(125) });
    storage.replace(Album{ std::make_shared<int>(192), std::make_shared<std::string>("Raul Seixas"), std::make_shared<int>(126) });
    storage.replace(Album{ std::make_shared<int>(193), std::make_shared<std::string>("Blood Sugar Sex Magik"), std::make_shared<int>(127) });
    storage.replace(Album{ std::make_shared<int>(194), std::make_shared<std::string>("By The Way"), std::make_shared<int>(127) });
    storage.replace(Album{ std::make_shared<int>(195), std::make_shared<std::string>("Californication"), std::make_shared<int>(127) });
    storage.replace(Album{ std::make_shared<int>(196), std::make_shared<std::string>("Retrospective I (1974-1980)"), std::make_shared<int>(128) });
    storage.replace(Album{ std::make_shared<int>(197), std::make_shared<std::string>("Santana - As Years Go By"), std::make_shared<int>(59) });
    storage.replace(Album{ std::make_shared<int>(198), std::make_shared<std::string>("Santana Live"), std::make_shared<int>(59) });
    storage.replace(Album{ std::make_shared<int>(199), std::make_shared<std::string>("Maquinarama"), std::make_shared<int>(130) });
    storage.replace(Album{ std::make_shared<int>(200), std::make_shared<std::string>("O Samba Poconé"), std::make_shared<int>(130) });
    storage.replace(Album{ std::make_shared<int>(201), std::make_shared<std::string>("Judas 0: B-Sides and Rarities"), std::make_shared<int>(131) });
    storage.replace(Album{ std::make_shared<int>(202), std::make_shared<std::string>("Rotten Apples: Greatest Hits"), std::make_shared<int>(131) });
    storage.replace(Album{ std::make_shared<int>(203), std::make_shared<std::string>("A-Sides"), std::make_shared<int>(132) });
    storage.replace(Album{ std::make_shared<int>(204), std::make_shared<std::string>("Morning Dance"), std::make_shared<int>(53) });
    storage.replace(Album{ std::make_shared<int>(205), std::make_shared<std::string>("In Step"), std::make_shared<int>(133) });
    storage.replace(Album{ std::make_shared<int>(206), std::make_shared<std::string>("Core"), std::make_shared<int>(134) });
    storage.replace(Album{ std::make_shared<int>(207), std::make_shared<std::string>("Mezmerize"), std::make_shared<int>(135) });
    storage.replace(Album{ std::make_shared<int>(208), std::make_shared<std::string>("[1997] Black Light Syndrome"), std::make_shared<int>(136) });
    storage.replace(Album{ std::make_shared<int>(209), std::make_shared<std::string>("Live [Disc 1]"), std::make_shared<int>(137) });
    storage.replace(Album{ std::make_shared<int>(210), std::make_shared<std::string>("Live [Disc 2]"), std::make_shared<int>(137) });
    storage.replace(Album{ std::make_shared<int>(211), std::make_shared<std::string>("The Singles"), std::make_shared<int>(138) });
    storage.replace(Album{ std::make_shared<int>(212), std::make_shared<std::string>("Beyond Good And Evil"), std::make_shared<int>(139) });
    storage.replace(Album{ std::make_shared<int>(213), std::make_shared<std::string>("Pure Cult: The Best Of The Cult (For Rockers, Ravers, Lovers & Sinners) [UK]"), std::make_shared<int>(139) });
    storage.replace(Album{ std::make_shared<int>(214), std::make_shared<std::string>("The Doors"), std::make_shared<int>(140) });
    storage.replace(Album{ std::make_shared<int>(215), std::make_shared<std::string>("The Police Greatest Hits"), std::make_shared<int>(141) });
    storage.replace(Album{ std::make_shared<int>(216), std::make_shared<std::string>("Hot Rocks, 1964-1971 (Disc 1)"), std::make_shared<int>(142) });
    storage.replace(Album{ std::make_shared<int>(217), std::make_shared<std::string>("No Security"), std::make_shared<int>(142) });
    storage.replace(Album{ std::make_shared<int>(218), std::make_shared<std::string>("Voodoo Lounge"), std::make_shared<int>(142) });
    storage.replace(Album{ std::make_shared<int>(219), std::make_shared<std::string>("Tangents"), std::make_shared<int>(143) });
    storage.replace(Album{ std::make_shared<int>(220), std::make_shared<std::string>("Transmission"), std::make_shared<int>(143) });
    storage.replace(Album{ std::make_shared<int>(221), std::make_shared<std::string>("My Generation - The Very Best Of The Who"), std::make_shared<int>(144) });
    storage.replace(Album{ std::make_shared<int>(222), std::make_shared<std::string>("Serie Sem Limite (Disc 1)"), std::make_shared<int>(145) });
    storage.replace(Album{ std::make_shared<int>(223), std::make_shared<std::string>("Serie Sem Limite (Disc 2)"), std::make_shared<int>(145) });
    storage.replace(Album{ std::make_shared<int>(224), std::make_shared<std::string>("Acústico"), std::make_shared<int>(146) });
    storage.replace(Album{ std::make_shared<int>(225), std::make_shared<std::string>("Volume Dois"), std::make_shared<int>(146) });
    storage.replace(Album{ std::make_shared<int>(226), std::make_shared<std::string>("Battlestar Galactica: The Story So Far"), std::make_shared<int>(147) });
    storage.replace(Album{ std::make_shared<int>(227), std::make_shared<std::string>("Battlestar Galactica, Season 3"), std::make_shared<int>(147) });
    storage.replace(Album{ std::make_shared<int>(228), std::make_shared<std::string>("Heroes, Season 1"), std::make_shared<int>(148) });
    storage.replace(Album{ std::make_shared<int>(229), std::make_shared<std::string>("Lost, Season 3"), std::make_shared<int>(149) });
    storage.replace(Album{ std::make_shared<int>(230), std::make_shared<std::string>("Lost, Season 1"), std::make_shared<int>(149) });
    storage.replace(Album{ std::make_shared<int>(231), std::make_shared<std::string>("Lost, Season 2"), std::make_shared<int>(149) });
    storage.replace(Album{ std::make_shared<int>(232), std::make_shared<std::string>("Achtung Baby"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(233), std::make_shared<std::string>("All That You Can't Leave Behind"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(234), std::make_shared<std::string>("B-Sides 1980-1990"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(235), std::make_shared<std::string>("How To Dismantle An Atomic Bomb"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(236), std::make_shared<std::string>("Pop"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(237), std::make_shared<std::string>("Rattle And Hum"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(238), std::make_shared<std::string>("The Best Of 1980-1990"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(239), std::make_shared<std::string>("War"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(240), std::make_shared<std::string>("Zooropa"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(241), std::make_shared<std::string>("UB40 The Best Of - Volume Two [UK]"), std::make_shared<int>(151) });
    storage.replace(Album{ std::make_shared<int>(242), std::make_shared<std::string>("Diver Down"), std::make_shared<int>(152) });
    storage.replace(Album{ std::make_shared<int>(243), std::make_shared<std::string>("The Best Of Van Halen, Vol. I"), std::make_shared<int>(152) });
    storage.replace(Album{ std::make_shared<int>(244), std::make_shared<std::string>("Van Halen"), std::make_shared<int>(152) });
    storage.replace(Album{ std::make_shared<int>(245), std::make_shared<std::string>("Van Halen III"), std::make_shared<int>(152) });
    storage.replace(Album{ std::make_shared<int>(246), std::make_shared<std::string>("Contraband"), std::make_shared<int>(153) });
    storage.replace(Album{ std::make_shared<int>(247), std::make_shared<std::string>("Vinicius De Moraes"), std::make_shared<int>(72) });
    storage.replace(Album{ std::make_shared<int>(248), std::make_shared<std::string>("Ao Vivo [IMPORT]"), std::make_shared<int>(155) });
    storage.replace(Album{ std::make_shared<int>(249), std::make_shared<std::string>("The Office, Season 1"), std::make_shared<int>(156) });
    storage.replace(Album{ std::make_shared<int>(250), std::make_shared<std::string>("The Office, Season 2"), std::make_shared<int>(156) });
    storage.replace(Album{ std::make_shared<int>(251), std::make_shared<std::string>("The Office, Season 3"), std::make_shared<int>(156) });
    storage.replace(Album{ std::make_shared<int>(252), std::make_shared<std::string>("Un-Led-Ed"), std::make_shared<int>(157) });
    storage.replace(Album{ std::make_shared<int>(253), std::make_shared<std::string>("Battlestar Galactica (Classic), Season 1"), std::make_shared<int>(158) });
    storage.replace(Album{ std::make_shared<int>(254), std::make_shared<std::string>("Aquaman"), std::make_shared<int>(159) });
    storage.replace(Album{ std::make_shared<int>(255), std::make_shared<std::string>("Instant Karma: The Amnesty International Campaign to Save Darfur"), std::make_shared<int>(150) });
    storage.replace(Album{ std::make_shared<int>(256), std::make_shared<std::string>("Speak of the Devil"), std::make_shared<int>(114) });
    storage.replace(Album{ std::make_shared<int>(257), std::make_shared<std::string>("20th Century Masters - The Millennium Collection: The Best of Scorpions"), std::make_shared<int>(179) });
    storage.replace(Album{ std::make_shared<int>(258), std::make_shared<std::string>("House of Pain"), std::make_shared<int>(180) });
    storage.replace(Album{ std::make_shared<int>(259), std::make_shared<std::string>("Radio Brasil (O Som da Jovem Vanguarda) - Seleccao de Henrique Amaro"), std::make_shared<int>(36) });
    storage.replace(Album{ std::make_shared<int>(260), std::make_shared<std::string>("Cake: B-Sides and Rarities"), std::make_shared<int>(196) });
    storage.replace(Album{ std::make_shared<int>(261), std::make_shared<std::string>("LOST, Season 4"), std::make_shared<int>(149) });
    storage.replace(Album{ std::make_shared<int>(262), std::make_shared<std::string>("Quiet Songs"), std::make_shared<int>(197) });
    storage.replace(Album{ std::make_shared<int>(263), std::make_shared<std::string>("Muso Ko"), std::make_shared<int>(198) });
    storage.replace(Album{ std::make_shared<int>(264), std::make_shared<std::string>("Realize"), std::make_shared<int>(199) });
    storage.replace(Album{ std::make_shared<int>(265), std::make_shared<std::string>("Every Kind of Light"), std::make_shared<int>(200) });
    storage.replace(Album{ std::make_shared<int>(266), std::make_shared<std::string>("Duos II"), std::make_shared<int>(201) });
    storage.replace(Album{ std::make_shared<int>(267), std::make_shared<std::string>("Worlds"), std::make_shared<int>(202) });
    storage.replace(Album{ std::make_shared<int>(268), std::make_shared<std::string>("The Best of Beethoven"), std::make_shared<int>(203) });
    storage.replace(Album{ std::make_shared<int>(269), std::make_shared<std::string>("Temple of the Dog"), std::make_shared<int>(204) });
    storage.replace(Album{ std::make_shared<int>(270), std::make_shared<std::string>("Carry On"), std::make_shared<int>(205) });
    storage.replace(Album{ std::make_shared<int>(271), std::make_shared<std::string>("Revelations"), std::make_shared<int>(8) });
    storage.replace(Album{ std::make_shared<int>(272), std::make_shared<std::string>("Adorate Deum: Gregorian Chant from the Proper of the Mass"), std::make_shared<int>(206) });
    storage.replace(Album{ std::make_shared<int>(273), std::make_shared<std::string>("Allegri: Miserere"), std::make_shared<int>(207) });
    storage.replace(Album{ std::make_shared<int>(274), std::make_shared<std::string>("Pachelbel: Canon & Gigue"), std::make_shared<int>(208) });
    storage.replace(Album{ std::make_shared<int>(275), std::make_shared<std::string>("Vivaldi: The Four Seasons"), std::make_shared<int>(209) });
    storage.replace(Album{ std::make_shared<int>(276), std::make_shared<std::string>("Bach: Violin Concertos"), std::make_shared<int>(210) });
    storage.replace(Album{ std::make_shared<int>(277), std::make_shared<std::string>("Bach: Goldberg Variations"), std::make_shared<int>(211) });
    storage.replace(Album{ std::make_shared<int>(278), std::make_shared<std::string>("Bach: The Cello Suites"), std::make_shared<int>(212) });
    storage.replace(Album{ std::make_shared<int>(279), std::make_shared<std::string>("Handel: The Messiah (Highlights)"), std::make_shared<int>(213) });
    storage.replace(Album{ std::make_shared<int>(280), std::make_shared<std::string>("The World of Classical Favourites"), std::make_shared<int>(214) });
    storage.replace(Album{ std::make_shared<int>(281), std::make_shared<std::string>("Sir Neville Marriner: A Celebration"), std::make_shared<int>(215) });
    storage.replace(Album{ std::make_shared<int>(282), std::make_shared<std::string>("Mozart: Wind Concertos"), std::make_shared<int>(216) });
    storage.replace(Album{ std::make_shared<int>(283), std::make_shared<std::string>("Haydn: Symphonies 99 - 104"), std::make_shared<int>(217) });
    storage.replace(Album{ std::make_shared<int>(284), std::make_shared<std::string>("Beethoven: Symhonies Nos. 5 & 6"), std::make_shared<int>(218) });
    storage.replace(Album{ std::make_shared<int>(285), std::make_shared<std::string>("A Soprano Inspired"), std::make_shared<int>(219) });
    storage.replace(Album{ std::make_shared<int>(286), std::make_shared<std::string>("Great Opera Choruses"), std::make_shared<int>(220) });
    storage.replace(Album{ std::make_shared<int>(287), std::make_shared<std::string>("Wagner: Favourite Overtures"), std::make_shared<int>(221) });
    storage.replace(Album{ std::make_shared<int>(288), std::make_shared<std::string>("Fauré: Requiem, Ravel: Pavane & Others"), std::make_shared<int>(222) });
    storage.replace(Album{ std::make_shared<int>(289), std::make_shared<std::string>("Tchaikovsky: The Nutcracker"), std::make_shared<int>(223) });
    storage.replace(Album{ std::make_shared<int>(290), std::make_shared<std::string>("The Last Night of the Proms"), std::make_shared<int>(224) });
    storage.replace(Album{ std::make_shared<int>(291), std::make_shared<std::string>("Puccini: Madama Butterfly - Highlights"), std::make_shared<int>(225) });
    storage.replace(Album{ std::make_shared<int>(292), std::make_shared<std::string>("Holst: The Planets, Op. 32 & Vaughan Williams: Fantasies"), std::make_shared<int>(226) });
    storage.replace(Album{ std::make_shared<int>(293), std::make_shared<std::string>("Pavarotti's Opera Made Easy"), std::make_shared<int>(227) });
    storage.replace(Album{ std::make_shared<int>(294), std::make_shared<std::string>("Great Performances - Barber's Adagio and Other Romantic Favorites for Strings"), std::make_shared<int>(228) });
    storage.replace(Album{ std::make_shared<int>(295), std::make_shared<std::string>("Carmina Burana"), std::make_shared<int>(229) });
    storage.replace(Album{ std::make_shared<int>(296), std::make_shared<std::string>("A Copland Celebration, Vol. I"), std::make_shared<int>(230) });
    storage.replace(Album{ std::make_shared<int>(297), std::make_shared<std::string>("Bach: Toccata & Fugue in D Minor"), std::make_shared<int>(231) });
    storage.replace(Album{ std::make_shared<int>(298), std::make_shared<std::string>("Prokofiev: Symphony No.1"), std::make_shared<int>(232) });
    storage.replace(Album{ std::make_shared<int>(299), std::make_shared<std::string>("Scheherazade"), std::make_shared<int>(233) });
    storage.replace(Album{ std::make_shared<int>(300), std::make_shared<std::string>("Bach: The Brandenburg Concertos"), std::make_shared<int>(234) });
    storage.replace(Album{ std::make_shared<int>(301), std::make_shared<std::string>("Chopin: Piano Concertos Nos. 1 & 2"), std::make_shared<int>(235) });
    storage.replace(Album{ std::make_shared<int>(302), std::make_shared<std::string>("Mascagni: Cavalleria Rusticana"), std::make_shared<int>(236) });
    storage.replace(Album{ std::make_shared<int>(303), std::make_shared<std::string>("Sibelius: Finlandia"), std::make_shared<int>(237) });
    storage.replace(Album{ std::make_shared<int>(304), std::make_shared<std::string>("Beethoven Piano Sonatas: Moonlight & Pastorale"), std::make_shared<int>(238) });
    storage.replace(Album{ std::make_shared<int>(305), std::make_shared<std::string>("Great Recordings of the Century - Mahler: Das Lied von der Erde"), std::make_shared<int>(240) });
    storage.replace(Album{ std::make_shared<int>(306), std::make_shared<std::string>("Elgar: Cello Concerto & Vaughan Williams: Fantasias"), std::make_shared<int>(241) });
    storage.replace(Album{ std::make_shared<int>(307), std::make_shared<std::string>("Adams, John: The Chairman Dances"), std::make_shared<int>(242) });
    storage.replace(Album{ std::make_shared<int>(308), std::make_shared<std::string>("Tchaikovsky: 1812 Festival Overture, Op.49, Capriccio Italien & Beethoven: Wellington's Victory"), std::make_shared<int>(243) });
    storage.replace(Album{ std::make_shared<int>(309), std::make_shared<std::string>("Palestrina: Missa Papae Marcelli & Allegri: Miserere"), std::make_shared<int>(244) });
    storage.replace(Album{ std::make_shared<int>(310), std::make_shared<std::string>("Prokofiev: Romeo & Juliet"), std::make_shared<int>(245) });
    storage.replace(Album{ std::make_shared<int>(311), std::make_shared<std::string>("Strauss: Waltzes"), std::make_shared<int>(226) });
    storage.replace(Album{ std::make_shared<int>(312), std::make_shared<std::string>("Berlioz: Symphonie Fantastique"), std::make_shared<int>(245) });
    storage.replace(Album{ std::make_shared<int>(313), std::make_shared<std::string>("Bizet: Carmen Highlights"), std::make_shared<int>(246) });
    storage.replace(Album{ std::make_shared<int>(314), std::make_shared<std::string>("English Renaissance"), std::make_shared<int>(247) });
    storage.replace(Album{ std::make_shared<int>(315), std::make_shared<std::string>("Handel: Music for the Royal Fireworks (Original Version 1749)"), std::make_shared<int>(208) });
    storage.replace(Album{ std::make_shared<int>(316), std::make_shared<std::string>("Grieg: Peer Gynt Suites & Sibelius: Pelléas et Mélisande"), std::make_shared<int>(248) });
    storage.replace(Album{ std::make_shared<int>(317), std::make_shared<std::string>("Mozart Gala: Famous Arias"), std::make_shared<int>(249) });
    storage.replace(Album{ std::make_shared<int>(318), std::make_shared<std::string>("SCRIABIN: Vers la flamme"), std::make_shared<int>(250) });
    storage.replace(Album{ std::make_shared<int>(319), std::make_shared<std::string>("Armada: Music from the Courts of England and Spain"), std::make_shared<int>(251) });
    storage.replace(Album{ std::make_shared<int>(320), std::make_shared<std::string>("Mozart: Symphonies Nos. 40 & 41"), std::make_shared<int>(248) });
    storage.replace(Album{ std::make_shared<int>(321), std::make_shared<std::string>("Back to Black"), std::make_shared<int>(252) });
    storage.replace(Album{ std::make_shared<int>(322), std::make_shared<std::string>("Frank"), std::make_shared<int>(252) });
    storage.replace(Album{ std::make_shared<int>(323), std::make_shared<std::string>("Carried to Dust (Bonus Track Version)"), std::make_shared<int>(253) });
    storage.replace(Album{ std::make_shared<int>(324), std::make_shared<std::string>("Beethoven: Symphony No. 6 'Pastoral' Etc."), std::make_shared<int>(254) });
    storage.replace(Album{ std::make_shared<int>(325), std::make_shared<std::string>("Bartok: Violin & Viola Concertos"), std::make_shared<int>(255) });
    storage.replace(Album{ std::make_shared<int>(326), std::make_shared<std::string>("Mendelssohn: A Midsummer Night's Dream"), std::make_shared<int>(256) });
    storage.replace(Album{ std::make_shared<int>(327), std::make_shared<std::string>("Bach: Orchestral Suites Nos. 1 - 4"), std::make_shared<int>(257) });
    storage.replace(Album{ std::make_shared<int>(328), std::make_shared<std::string>("Charpentier: Divertissements, Airs & Concerts"), std::make_shared<int>(258) });
    storage.replace(Album{ std::make_shared<int>(329), std::make_shared<std::string>("South American Getaway"), std::make_shared<int>(259) });
    storage.replace(Album{ std::make_shared<int>(330), std::make_shared<std::string>("Górecki: Symphony No. 3"), std::make_shared<int>(260) });
    storage.replace(Album{ std::make_shared<int>(331), std::make_shared<std::string>("Purcell: The Fairy Queen"), std::make_shared<int>(261) });
    storage.replace(Album{ std::make_shared<int>(332), std::make_shared<std::string>("The Ultimate Relexation Album"), std::make_shared<int>(262) });
    storage.replace(Album{ std::make_shared<int>(333), std::make_shared<std::string>("Purcell: Music for the Queen Mary"), std::make_shared<int>(263) });
    storage.replace(Album{ std::make_shared<int>(334), std::make_shared<std::string>("Weill: The Seven Deadly Sins"), std::make_shared<int>(264) });
    storage.replace(Album{ std::make_shared<int>(335), std::make_shared<std::string>("J.S. Bach: Chaconne, Suite in E Minor, Partita in E Major & Prelude, Fugue and Allegro"), std::make_shared<int>(265) });
    storage.replace(Album{ std::make_shared<int>(336), std::make_shared<std::string>("Prokofiev: Symphony No.5 & Stravinksy: Le Sacre Du Printemps"), std::make_shared<int>(248) });
    storage.replace(Album{ std::make_shared<int>(337), std::make_shared<std::string>("Szymanowski: Piano Works, Vol. 1"), std::make_shared<int>(266) });
    storage.replace(Album{ std::make_shared<int>(338), std::make_shared<std::string>("Nielsen: The Six Symphonies"), std::make_shared<int>(267) });
    storage.replace(Album{ std::make_shared<int>(339), std::make_shared<std::string>("Great Recordings of the Century: Paganini's 24 Caprices"), std::make_shared<int>(268) });
    storage.replace(Album{ std::make_shared<int>(340), std::make_shared<std::string>("Liszt - 12 Études D'Execution Transcendante"), std::make_shared<int>(269) });
    storage.replace(Album{ std::make_shared<int>(341), std::make_shared<std::string>("Great Recordings of the Century - Shubert: Schwanengesang, 4 Lieder"), std::make_shared<int>(270) });
    storage.replace(Album{ std::make_shared<int>(342), std::make_shared<std::string>("Locatelli: Concertos for Violin, Strings and Continuo, Vol. 3"), std::make_shared<int>(271) });
    storage.replace(Album{ std::make_shared<int>(343), std::make_shared<std::string>("Respighi:Pines of Rome"), std::make_shared<int>(226) });
    storage.replace(Album{ std::make_shared<int>(344), std::make_shared<std::string>("Schubert: The Late String Quartets & String Quintet (3 CD's)"), std::make_shared<int>(272) });
    storage.replace(Album{ std::make_shared<int>(345), std::make_shared<std::string>("Monteverdi: L'Orfeo"), std::make_shared<int>(273) });
    storage.replace(Album{ std::make_shared<int>(346), std::make_shared<std::string>("Mozart: Chamber Music"), std::make_shared<int>(274) });
    storage.replace(Album{ std::make_shared<int>(347), std::make_shared<std::string>("Koyaanisqatsi (Soundtrack from the Motion Picture)"), std::make_shared<int>(275) });

    
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
    
    return 0;
}
