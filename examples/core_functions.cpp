#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <cassert>

using std::cout;
using std::endl;

struct MarvelHero {
    int id = 0;
    std::string name;
    std::string abilities;
    short points = 0;
};

struct Contact {
    int id = 0;
    std::string firstName;
    std::string lastName;
    std::string phone;
};

struct Customer {
    int id = 0;
    std::string firstName;
    std::string lastName;
    std::string company;
    std::string address;
    std::string city;
    std::string state;
    std::string country;
    std::string postalCode;
    std::string phone;
    std::unique_ptr<std::string> fax;
    std::string email;
    int supportRepId = 0;
};

int main(int, char** argv) {
    cout << "path = " << argv[0] << endl;

    using namespace sqlite_orm;
    auto storage = make_storage("core_functions.sqlite",
                                make_table("marvel",
                                           make_column("id", &MarvelHero::id, primary_key()),
                                           make_column("name", &MarvelHero::name),
                                           make_column("abilities", &MarvelHero::abilities),
                                           make_column("points", &MarvelHero::points)),
                                make_table("contacts",
                                           make_column("contact_id", &Contact::id, primary_key()),
                                           make_column("first_name", &Contact::firstName),
                                           make_column("last_name", &Contact::lastName),
                                           make_column("phone", &Contact::phone)),
                                make_table("customers",
                                           make_column("CustomerId", &Customer::id, primary_key()),
                                           make_column("FirstName", &Customer::firstName),
                                           make_column("LastName", &Customer::lastName),
                                           make_column("Company", &Customer::company),
                                           make_column("Address", &Customer::address),
                                           make_column("City", &Customer::city),
                                           make_column("State", &Customer::state),
                                           make_column("Country", &Customer::country),
                                           make_column("PostalCode", &Customer::postalCode),
                                           make_column("Phone", &Customer::phone),
                                           make_column("Fax", &Customer::fax),
                                           make_column("Email", &Customer::email),
                                           make_column("SupportRepId", &Customer::supportRepId)));
    storage.sync_schema();

    storage.remove_all<MarvelHero>();

    //  insert values..
    storage.transaction([&storage] {
        storage.insert(MarvelHero{-1, "Tony Stark", "Iron man, playboy, billionaire, philanthropist", 5});
        storage.insert(MarvelHero{-1, "Thor", "Storm god", -10});
        storage.insert(MarvelHero{-1, "Vision", "Min Stone", 4});
        storage.insert(MarvelHero{-1, "Captain America", "Vibranium shield", -3});
        storage.insert(MarvelHero{-1, "Hulk", "Strength", -15});
        storage.insert(MarvelHero{-1, "Star Lord", "Humor", 19});
        storage.insert(MarvelHero{-1, "Peter Parker", "Spiderman", 16});
        storage.insert(MarvelHero{-1, "Clint Barton", "Hawkeye", -11});
        storage.insert(MarvelHero{-1, "Natasha Romanoff", "Black widow", 8});
        storage.insert(MarvelHero{-1, "Groot", "I am Groot!", 2});

        cout << "SELECT last_insert_rowid() = " << storage.select(last_insert_rowid()).front() << endl;

        storage.replace(Customer{1,
                                 "Luís",
                                 "Gonçalves",
                                 "Embraer - Empresa Brasileira de Aeronáutica S.A.",
                                 "Av. Brigadeiro Faria Lima, 2170",
                                 "São José dos Campos",
                                 "SP",
                                 "Brazil",
                                 "12227-000",
                                 "+55 (12) 3923-5555",
                                 std::make_unique<std::string>("+55 (12) 3923-5566"),
                                 "luisg@embraer.com.br",
                                 3});
        storage.replace(Customer{2,
                                 "Leonie",
                                 "Köhler",
                                 "",
                                 "Theodor-Heuss-Straße 34",
                                 "Stuttgart",
                                 "",
                                 "Germany",
                                 "70174",
                                 "+49 0711 2842222",
                                 nullptr,
                                 "leonekohler@surfeu.de",
                                 5});
        storage.replace(Customer{3,
                                 "François",
                                 "Tremblay",
                                 "",
                                 "1498 rue Bélanger",
                                 "Montréal",
                                 "QC",
                                 "Canada",
                                 "H2G 1A7",
                                 "+1 (514) 721-4711",
                                 nullptr,
                                 "ftremblay@gmail.com",
                                 3});
        storage.replace(Customer{4,
                                 "Bjørn",
                                 "Hansen",
                                 "",
                                 "Ullevålsveien 14",
                                 "Oslo",
                                 "",
                                 "Norway",
                                 "0171",
                                 "+47 22 44 22 22",
                                 nullptr,
                                 "bjorn.hansen@yahoo.no",
                                 4});
        storage.replace(Customer{5,
                                 "František",
                                 "Wichterlová",
                                 "JetBrains s.r.o.",
                                 "Klanova 9/506",
                                 "Prague",
                                 "",
                                 "Czech Republic",
                                 "14700",
                                 "+420 2 4172 5555",
                                 std::make_unique<std::string>("+420 2 4172 5555"),
                                 "frantisekw@jetbrains.com",
                                 4});
        storage.replace(Customer{6,
                                 "Helena",
                                 "Holý",
                                 "",
                                 "Rilská 3174/6",
                                 "Prague",
                                 "",
                                 "Czech Republic",
                                 "14300",
                                 "+420 2 4177 0449",
                                 nullptr,
                                 "hholy@gmail.com",
                                 5});
        storage.replace(Customer{7,
                                 "Astrid",
                                 "Gruber",
                                 "",
                                 "Rotenturmstraße 4, 1010 Innere Stadt",
                                 "Vienne",
                                 "",
                                 "Austria",
                                 "1010",
                                 "+43 01 5134505",
                                 nullptr,
                                 "astrid.gruber@apple.at",
                                 5});
        storage.replace(Customer{8,
                                 "Daan",
                                 "Peeters",
                                 "",
                                 "Grétrystraat 63",
                                 "Brussels",
                                 "",
                                 "Belgium",
                                 "1000",
                                 "+32 02 219 03 03",
                                 nullptr,
                                 "daan_peeters@apple.be",
                                 4});
        storage.replace(Customer{9,
                                 "Kara",
                                 "Nielsen",
                                 "",
                                 "Sønder Boulevard 51",
                                 "Copenhagen",
                                 "",
                                 "Denmark",
                                 "1720",
                                 "+453 3331 9991",
                                 nullptr,
                                 "kara.nielsen@jubii.dk",
                                 4});
        storage.replace(Customer{10,
                                 "Eduardo",
                                 "Martins",
                                 "Woodstock Discos",
                                 "Rua Dr. Falcão Filho, 155",
                                 "São Paulo",
                                 "SP",
                                 "Brazil",
                                 "01007-010",
                                 "+55 (11) 3033-5446",
                                 std::make_unique<std::string>("+55 (11) 3033-4564"),
                                 "eduardo@woodstock.com.br",
                                 4});
        storage.replace(Customer{11,
                                 "Alexandre",
                                 "Rocha",
                                 "Banco do Brasil S.A.",
                                 "Av. Paulista, 2022",
                                 "São Paulo",
                                 "SP",
                                 "Brazil",
                                 "01310-200",
                                 "+55 (11) 3055-3278",
                                 std::make_unique<std::string>("+55 (11) 3055-8131"),
                                 "alero@uol.com.br",
                                 5});
        storage.replace(Customer{12,
                                 "Roberto",
                                 "Almeida",
                                 "Riotur",
                                 "Praça Pio X, 119",
                                 "Rio de Janeiro",
                                 "RJ",
                                 "Brazil",
                                 "20040-020",
                                 "+55 (21) 2271-7000",
                                 std::make_unique<std::string>("+55 (21) 2271-7070"),
                                 "roberto.almeida@riotur.gov.br",
                                 3});
        storage.replace(Customer{13,
                                 "Fernanda",
                                 "Ramos",
                                 "",
                                 "Qe 7 Bloco G",
                                 "Brasília",
                                 "DF",
                                 "Brazil",
                                 "71020-677",
                                 "+55 (61) 3363-5547",
                                 std::make_unique<std::string>("+55 (61) 3363-7855"),
                                 "fernadaramos4@uol.com.br",
                                 4});
        storage.replace(Customer{14,
                                 "Mark",
                                 "Philips",
                                 "Telus",
                                 "8210 111 ST NW",
                                 "Edmonton",
                                 "AB",
                                 "Canada",
                                 "T6G 2C7",
                                 "+1 (780) 434-4554",
                                 std::make_unique<std::string>("+1 (780) 434-5565"),
                                 "mphilips12@shaw.ca",
                                 5});
        storage.replace(Customer{15,
                                 "Jennifer",
                                 "Peterson",
                                 "Rogers Canada",
                                 "700 W Pender Street",
                                 "Vancouver",
                                 "BC",
                                 "Canada",
                                 "V6C 1G8",
                                 "+1 (604) 688-2255",
                                 std::make_unique<std::string>("+1 (604) 688-8756"),
                                 "jenniferp@rogers.ca",
                                 3});
        storage.replace(Customer{16,
                                 "Frank",
                                 "Harris",
                                 "Google Inc.",
                                 "1600 Amphitheatre Parkway",
                                 "Mountain View",
                                 "CA",
                                 "USA",
                                 "94043-1351",
                                 "+1 (650) 253-0000",
                                 std::make_unique<std::string>("+1 (650) 253-0000"),
                                 "fharris@google.com",
                                 4});
        storage.replace(Customer{17,
                                 "Jack",
                                 "Smith",
                                 "Microsoft Corporation",
                                 "1 Microsoft Way",
                                 "Redmond",
                                 "WA",
                                 "USA",
                                 "98052-8300",
                                 "+1 (425) 882-8080",
                                 std::make_unique<std::string>("+1 (425) 882-8081"),
                                 "jacksmith@microsoft.com",
                                 5});
        storage.replace(Customer{18,
                                 "Michelle",
                                 "Brooks",
                                 "",
                                 "627 Broadway",
                                 "New York",
                                 "NY",
                                 "USA",
                                 "10012-2612",
                                 "+1 (212) 221-3546",
                                 std::make_unique<std::string>("+1 (212) 221-4679"),
                                 "michelleb@aol.com",
                                 3});
        storage.replace(Customer{19,
                                 "Tim",
                                 "Goyer",
                                 "Apple Inc.",
                                 "1 Infinite Loop",
                                 "Cupertino",
                                 "CA",
                                 "USA",
                                 "95014",
                                 "+1 (408) 996-1010",
                                 std::make_unique<std::string>("+1 (408) 996-1011"),
                                 "tgoyer@apple.com",
                                 3});
        storage.replace(Customer{20,
                                 "Dan",
                                 "Miller",
                                 "",
                                 "541 Del Medio Avenue",
                                 "Mountain View",
                                 "CA",
                                 "USA",
                                 "94040-111",
                                 "+1 (650) 644-3358",
                                 nullptr,
                                 "dmiller@comcast.com",
                                 4});
        storage.replace(Customer{21,
                                 "Kathy",
                                 "Chase",
                                 "",
                                 "801 W 4th Street",
                                 "Reno",
                                 "NV",
                                 "USA",
                                 "89503",
                                 "+1 (775) 223-7665",
                                 nullptr,
                                 "kachase@hotmail.com",
                                 5});
        storage.replace(Customer{22,
                                 "Heather",
                                 "Leacock",
                                 "",
                                 "120 S Orange Ave",
                                 "Orlando",
                                 "FL",
                                 "USA",
                                 "32801",
                                 "+1 (407) 999-7788",
                                 nullptr,
                                 "hleacock@gmail.com",
                                 4});
        storage.replace(Customer{23,
                                 "John",
                                 "Gordon",
                                 "",
                                 "69 Salem Street",
                                 "Boston",
                                 "MA",
                                 "USA",
                                 "2113",
                                 "+1 (617) 522-1333",
                                 nullptr,
                                 "johngordon22@yahoo.com",
                                 4});
        storage.replace(Customer{24,
                                 "Frank",
                                 "Ralston",
                                 "",
                                 "162 E Superior Street",
                                 "Chicago",
                                 "IL",
                                 "USA",
                                 "60611",
                                 "+1 (312) 332-3232",
                                 nullptr,
                                 "fralston@gmail.com",
                                 3});
        storage.replace(Customer{25,
                                 "Victor",
                                 "Stevens",
                                 "",
                                 "319 N. Frances Street",
                                 "Madison",
                                 "WI",
                                 "USA",
                                 "53703",
                                 "+1 (608) 257-0597",
                                 nullptr,
                                 "vstevens@yahoo.com",
                                 5});
        storage.replace(Customer{26,
                                 "Richard",
                                 "Cunningham",
                                 "",
                                 "2211 W Berry Street",
                                 "Fort Worth",
                                 "TX",
                                 "USA",
                                 "76110",
                                 "+1 (817) 924-7272",
                                 nullptr,
                                 "ricunningham@hotmail.com",
                                 4});
        storage.replace(Customer{27,
                                 "Patrick",
                                 "Gray",
                                 "",
                                 "1033 N Park Ave",
                                 "Tucson",
                                 "AZ",
                                 "USA",
                                 "85719",
                                 "+1 (520) 622-4200",
                                 nullptr,
                                 "patrick.gray@aol.com",
                                 4});
        storage.replace(Customer{28,
                                 "Julia",
                                 "Barnett",
                                 "",
                                 "302 S 700 E",
                                 "Salt Lake City",
                                 "UT",
                                 "USA",
                                 "84102",
                                 "+1 (801) 531-7272",
                                 nullptr,
                                 "jubarnett@gmail.com",
                                 5});
        storage.replace(Customer{29,
                                 "Robert",
                                 "Brown",
                                 "",
                                 "796 Dundas Street West",
                                 "Toronto",
                                 "ON",
                                 "Canada",
                                 "M6J 1V1",
                                 "+1 (416) 363-8888",
                                 nullptr,
                                 "robbrown@shaw.ca",
                                 3});
        storage.replace(Customer{30,
                                 "Edward",
                                 "Francis",
                                 "",
                                 "230 Elgin Street",
                                 "Ottawa",
                                 "ON",
                                 "Canada",
                                 "K2P 1L7",
                                 "+1 (613) 234-3322",
                                 nullptr,
                                 "edfrancis@yachoo.ca",
                                 3});
        storage.replace(Customer{31,
                                 "Martha",
                                 "Silk",
                                 "",
                                 "194A Chain Lake Drive",
                                 "Halifax",
                                 "NS",
                                 "Canada",
                                 "B3S 1C5",
                                 "+1 (902) 450-0450",
                                 nullptr,
                                 "marthasilk@gmail.com",
                                 5});
        storage.replace(Customer{32,
                                 "Aaron",
                                 "Mitchell",
                                 "",
                                 "696 Osborne Street",
                                 "Winnipeg",
                                 "MB",
                                 "Canada",
                                 "R3L 2B9",
                                 "+1 (204) 452-6452",
                                 nullptr,
                                 "aaronmitchell@yahoo.ca",
                                 4});
        storage.replace(Customer{33,
                                 "Ellie",
                                 "Sullivan",
                                 "",
                                 "5112 48 Street",
                                 "Yellowknife",
                                 "NT",
                                 "Canada",
                                 "X1A 1N6",
                                 "+1 (867) 920-2233",
                                 nullptr,
                                 "ellie.sullivan@shaw.ca",
                                 3});
        storage.replace(Customer{34,
                                 "João",
                                 "Fernandes",
                                 "",
                                 "Rua da Assunção 53",
                                 "Lisbon",
                                 "",
                                 "Portugal",
                                 "",
                                 "+351 (213) 466-111",
                                 nullptr,
                                 "jfernandes@yahoo.pt",
                                 4});
        storage.replace(Customer{35,
                                 "Madalena",
                                 "Sampaio",
                                 "",
                                 "Rua dos Campeões Europeus de Viena, 4350",
                                 "Porto",
                                 "",
                                 "Portugal",
                                 "",
                                 "+351 (225) 022-448",
                                 nullptr,
                                 "masampaio@sapo.pt",
                                 4});
        storage.replace(Customer{36,
                                 "Hannah",
                                 "Schneider",
                                 "",
                                 "Tauentzienstraße 8",
                                 "Berlin",
                                 "",
                                 "Germany",
                                 "10789",
                                 "+49 030 26550280",
                                 nullptr,
                                 "hannah.schneider@yahoo.de",
                                 5});
        storage.replace(Customer{37,
                                 "Fynn",
                                 "Zimmermann",
                                 "",
                                 "Berger Straße 10",
                                 "Frankfurt",
                                 "",
                                 "Germany",
                                 "60316",
                                 "+49 069 40598889",
                                 nullptr,
                                 "fzimmermann@yahoo.de",
                                 3});
        storage.replace(Customer{38,
                                 "Niklas",
                                 "Schröder",
                                 "",
                                 "Barbarossastraße 19",
                                 "Berlin",
                                 "",
                                 "Germany",
                                 "10779",
                                 "+49 030 2141444",
                                 nullptr,
                                 "nschroder@surfeu.de",
                                 3});
        storage.replace(Customer{39,
                                 "Camille",
                                 "Bernard",
                                 "",
                                 "4, Rue Milton",
                                 "Paris",
                                 "",
                                 "France",
                                 "75009",
                                 "+33 01 49 70 65 65",
                                 nullptr,
                                 "camille.bernard@yahoo.fr",
                                 4});
        storage.replace(Customer{40,
                                 "Dominique",
                                 "Lefebvre",
                                 "",
                                 "8, Rue Hanovre",
                                 "Paris",
                                 "",
                                 "France",
                                 "75002",
                                 "+33 01 47 42 71 71",
                                 nullptr,
                                 "dominiquelefebvre@gmail.com",
                                 4});
        storage.replace(Customer{41,
                                 "Marc",
                                 "Dubois",
                                 "",
                                 "11, Place Bellecour",
                                 "Lyon",
                                 "",
                                 "France",
                                 "69002",
                                 "+33 04 78 30 30 30",
                                 nullptr,
                                 "marc.dubois@hotmail.com",
                                 5});
        storage.replace(Customer{42,
                                 "Wyatt",
                                 "Girard",
                                 "",
                                 "9, Place Louis Barthou",
                                 "Bordeaux",
                                 "",
                                 "France",
                                 "33000",
                                 "+33 05 56 96 96 96",
                                 nullptr,
                                 "wyatt.girard@yahoo.fr",
                                 3});
        storage.replace(Customer{43,
                                 "Isabelle",
                                 "Mercier",
                                 "",
                                 "68, Rue Jouvence",
                                 "Dijon",
                                 "",
                                 "France",
                                 "21000",
                                 "+33 03 80 73 66 99",
                                 nullptr,
                                 "isabelle_mercier@apple.fr",
                                 3});
        storage.replace(Customer{44,
                                 "Terhi",
                                 "Hämäläinen",
                                 "",
                                 "Porthaninkatu 9",
                                 "Helsinki",
                                 "",
                                 "Finland",
                                 "00530",
                                 "+358 09 870 2000",
                                 nullptr,
                                 "terhi.hamalainen@apple.fi",
                                 3});
        storage.replace(Customer{45,
                                 "Ladislav",
                                 "Kovács",
                                 "",
                                 "Erzsébet krt. 58.",
                                 "Budapest",
                                 "",
                                 "Hungary",
                                 "H-1073",
                                 "",
                                 nullptr,
                                 "ladislav_kovacs@apple.hu",
                                 3});
        storage.replace(Customer{46,
                                 "Hugh",
                                 "O'Reilly",
                                 "",
                                 "3 Chatham Street",
                                 "Dublin",
                                 "Dublin",
                                 "Ireland",
                                 "",
                                 "+353 01 6792424",
                                 nullptr,
                                 "hughoreilly@apple.ie",
                                 3});
        storage.replace(Customer{47,
                                 "Lucas",
                                 "Mancini",
                                 "",
                                 "Via Degli Scipioni, 43",
                                 "Rome",
                                 "RM",
                                 "Italy",
                                 "00192",
                                 "+39 06 39733434",
                                 nullptr,
                                 "lucas.mancini@yahoo.it",
                                 5});
        storage.replace(Customer{48,
                                 "Johannes",
                                 "Van der Berg",
                                 "",
                                 "Lijnbaansgracht 120bg",
                                 "Amsterdam",
                                 "VV",
                                 "Netherlands",
                                 "1016",
                                 "+31 020 6223130",
                                 nullptr,
                                 "johavanderberg@yahoo.nl",
                                 5});
        storage.replace(Customer{49,
                                 "Stanisław",
                                 "Wójcik",
                                 "",
                                 "Ordynacka 10",
                                 "Warsaw",
                                 "",
                                 "Poland",
                                 "00-358",
                                 "+48 22 828 37 39",
                                 nullptr,
                                 "stanisław.wójcik@wp.pl",
                                 4});
        storage.replace(Customer{50,
                                 "Enrique",
                                 "Muñoz",
                                 "",
                                 "C/ San Bernardo 85",
                                 "Madrid",
                                 "",
                                 "Spain",
                                 "28015",
                                 "+34 914 454 454",
                                 nullptr,
                                 "enrique_munoz@yahoo.es",
                                 5});
        storage.replace(Customer{51,
                                 "Joakim",
                                 "Johansson",
                                 "",
                                 "Celsiusg. 9",
                                 "Stockholm",
                                 "",
                                 "Sweden",
                                 "11230",
                                 "+46 08-651 52 52",
                                 nullptr,
                                 "joakim.johansson@yahoo.se",
                                 5});
        storage.replace(Customer{52,
                                 "Emma",
                                 "Jones",
                                 "",
                                 "202 Hoxton Street",
                                 "London",
                                 "",
                                 "United Kingdom",
                                 "N1 5LH",
                                 "+44 020 7707 0707",
                                 nullptr,
                                 "emma_jones@hotmail.com",
                                 3});
        storage.replace(Customer{53,
                                 "Phil",
                                 "Hughes",
                                 "",
                                 "113 Lupus St",
                                 "London",
                                 "",
                                 "United Kingdom",
                                 "SW1V 3EN",
                                 "+44 020 7976 5722",
                                 nullptr,
                                 "phil.hughes@gmail.com",
                                 3});
        storage.replace(Customer{54,
                                 "Steve",
                                 "Murray",
                                 "",
                                 "110 Raeburn Pl",
                                 "Edinburgh ",
                                 "",
                                 "United Kingdom",
                                 "EH4 1HH",
                                 "+44 0131 315 3300",
                                 nullptr,
                                 "steve.murray@yahoo.uk",
                                 5});
        storage.replace(Customer{55,
                                 "Mark",
                                 "Taylor",
                                 "",
                                 "421 Bourke Street",
                                 "Sidney",
                                 "NSW",
                                 "Australia",
                                 "2010",
                                 "+61 (02) 9332 3633",
                                 nullptr,
                                 "mark.taylor@yahoo.au",
                                 4});
        storage.replace(Customer{56,
                                 "Diego",
                                 "Gutiérrez",
                                 "",
                                 "307 Macacha Güemes",
                                 "Buenos Aires",
                                 "",
                                 "Argentina",
                                 "1106",
                                 "+54 (0)11 4311 4333",
                                 nullptr,
                                 "diego.gutierrez@yahoo.ar",
                                 4});
        storage.replace(Customer{57,
                                 "Luis",
                                 "Rojas",
                                 "",
                                 "Calle Lira, 198",
                                 "Santiago",
                                 "",
                                 "Chile",
                                 "",
                                 "+56 (0)2 635 4444",
                                 nullptr,
                                 "luisrojas@yahoo.cl",
                                 5});
        storage.replace(Customer{58,
                                 "Manoj",
                                 "Pareek",
                                 "",
                                 "12,Community Centre",
                                 "Delhi",
                                 "",
                                 "India",
                                 "110017",
                                 "+91 0124 39883988",
                                 nullptr,
                                 "manoj.pareek@rediff.com",
                                 3});
        storage.replace(Customer{59,
                                 "Puja",
                                 "Srivastava",
                                 "",
                                 "3,Raj Bhavan Road",
                                 "Bangalore",
                                 "",
                                 "India",
                                 "560001",
                                 "+91 080 22289999",
                                 nullptr,
                                 "puja_srivastava@yahoo.in",
                                 3});

        return true;
    });

    Contact john{0, "John", "Doe", "410-555-0168"};
    Contact lily{0, "Lily", "Bush", "410-444-9862"};
    john.id = storage.insert(john);
    lily.id = storage.insert(lily);

    //  SELECT LENGTH(name)
    //  FROM marvel
    auto nameLengths = storage.select(length(&MarvelHero::name));  //  nameLengths is std::vector<int>
    cout << "nameLengths.size = " << nameLengths.size() << endl;
    for(auto& len: nameLengths) {
        cout << len << " ";
    }
    cout << endl;

    //  SELECT name, LENGTH(name)
    //  FROM marvel
    auto namesWithLengths = storage.select(
        columns(&MarvelHero::name,
                length(&MarvelHero::name)));  //  namesWithLengths is std::vector<std::tuple<std::string, int>>
    cout << "namesWithLengths.size = " << namesWithLengths.size() << endl;
    for(auto& row: namesWithLengths) {
        cout << "LENGTH(" << std::get<0>(row) << ") = " << std::get<1>(row) << endl;
    }

    //  SELECT name
    //  FROM marvel
    //  WHERE LENGTH(name) > 5
    auto namesWithLengthGreaterThan5 =
        storage.select(&MarvelHero::name, where(length(&MarvelHero::name) > 5));  //  std::vector<std::string>
    cout << "namesWithLengthGreaterThan5.size = " << namesWithLengthGreaterThan5.size() << endl;
    for(auto& name: namesWithLengthGreaterThan5) {
        cout << "name = " << name << endl;
    }

    //  SELECT LENGTH('ototo')
    auto custom = storage.select(length("ototo"));
    cout << "custom = " << custom.front() << endl;

    //  SELECT LENGTH(1990), LENGTH(CURRENT_TIMESTAMP)
    auto customTwo = storage.select(columns(length(1990), length(storage.current_timestamp())));
    cout << "customTwo = {" << std::get<0>(customTwo.front()) << ", " << std::get<1>(customTwo.front()) << "}" << endl;

    //  SELECT ABS(points)
    //  FROM marvel
    auto absPoints = storage.select(abs(&MarvelHero::points));  //  std::vector<std::unique_ptr<int>>
    cout << "absPoints: ";
    for(auto& value: absPoints) {
        if(value) {
            cout << *value;
        } else {
            cout << "null";
        }
        cout << " ";
    }
    cout << endl;

    //  SELECT name
    //  FROM marvel
    //  WHERE ABS(points) < 5
    auto namesByAbs = storage.select(&MarvelHero::name, where(abs(&MarvelHero::points) < 5));
    cout << "namesByAbs.size = " << namesByAbs.size() << endl;
    for(auto& name: namesByAbs) {
        cout << name << endl;
    }
    cout << endl;

    //  SELECT length(abs(points))
    //  FROM marvel
    auto twoFunctions = storage.select(length(abs(&MarvelHero::points)));
    cout << "twoFunctions.size = " << twoFunctions.size() << endl;
    cout << endl;

    //  SELECT LOWER(name)
    //  FROM marvel
    auto lowerNames = storage.select(lower(&MarvelHero::name));
    cout << "lowerNames.size = " << lowerNames.size() << endl;
    for(auto& name: lowerNames) {
        cout << name << endl;
    }
    cout << endl;

    //  SELECT UPPER(abilities)
    //  FROM marvel
    auto upperAbilities = storage.select(upper(&MarvelHero::abilities));
    cout << "upperAbilities.size = " << upperAbilities.size() << endl;
    for(auto& abilities: upperAbilities) {
        cout << abilities << endl;
    }
    cout << endl;

    storage.transaction([&] {
        storage.remove_all<MarvelHero>();

        //  SELECT changes()
        {
            auto rowsRemoved = storage.select(changes()).front();
            cout << "rowsRemoved = " << rowsRemoved << endl;
            assert(rowsRemoved == storage.changes());
        }

        //  SELECT total_changes()
        {
            auto rowsRemoved = storage.select(total_changes()).front();
            cout << "rowsRemoved = " << rowsRemoved << endl;
            assert(rowsRemoved == storage.changes());
        }
        return false;
    });

#if SQLITE_VERSION_NUMBER >= 3007016

    //  SELECT CHAR(67, 72, 65, 82)
    auto charString = storage.select(char_(67, 72, 65, 82)).front();
    cout << "SELECT CHAR(67,72,65,82) = *" << charString << "*" << endl;

    //  SELECT LOWER(name) || '@marvel.com'
    //  FROM marvel
    auto emails = storage.select(lower(&MarvelHero::name) || c("@marvel.com"));
    cout << "emails.size = " << emails.size() << endl;
    for(auto& email: emails) {
        cout << email << endl;
    }
    cout << endl;

#endif

    //  TRIM examples are taken from here https://www.techonthenet.com/sqlite/functions/trim.php

    //  SELECT TRIM('   TechOnTheNet.com   ')
    cout << "trim '   TechOnTheNet.com   ' = '" << storage.select(trim("   TechOnTheNet.com   ")).front() << "'"
         << endl;

    //  SELECT TRIM('000123000', '0')
    cout << "TRIM('000123000', '0') = " << storage.select(trim("000123000", "0")).front() << endl;

    //  SELECT TRIM('zTOTNxyxzyyy', 'xyz')
    cout << "SELECT TRIM('zTOTNxyxzyyy', 'xyz') = " << storage.select(trim("zTOTNxyxzyyy", "xyz")).front() << endl;

    //  SELECT TRIM('42totn6372', '0123456789')
    cout << "TRIM('42totn6372', '0123456789') = " << storage.select(trim("42totn6372", "0123456789")).front() << endl;

    //  SELECT RANDOM()
    for(auto i = 0; i < 10; ++i) {
        cout << "RANDOM() = " << storage.select(sqlite_orm::random()).front() << endl;
    }

    //  SELECT * FROM marvel ORDER BY RANDOM()
    for(auto& hero: storage.iterate<MarvelHero>(order_by(sqlite_orm::random()))) {
        cout << "hero = " << storage.dump(hero) << endl;
    }

    //  https://www.techonthenet.com/sqlite/functions/ltrim.php

    //  SELECT ltrim('   TechOnTheNet.com');
    cout << "ltrim('   TechOnTheNet.com') = *" << storage.select(ltrim("   TechOnTheNet.com")).front() << "*" << endl;

    //  SELECT ltrim('   TechOnTheNet.com   ');
    cout << "ltrim('   TechOnTheNet.com   ') = *" << storage.select(ltrim("   TechOnTheNet.com   ")).front() << "*"
         << endl;

    //  SELECT ltrim('   TechOnTheNet.com    is great!');
    cout << "ltrim('   TechOnTheNet.com    is great!') = *"
         << storage.select(ltrim("   TechOnTheNet.com    is great!")).front() << "*" << endl;

    {  //  core functions can be use within prepared statements as well!

        auto lTrimStatement = storage.prepare(select(ltrim("000123", "0")));

        //  SELECT ltrim('000123', '0');
        cout << "ltrim('000123', '0') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('123123totn', '123');
        get<0>(lTrimStatement) = "123123totn";
        get<1>(lTrimStatement) = "123";
        cout << "ltrim('123123totn', '123') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('123123totn123', '123');
        get<0>(lTrimStatement) = "123123totn123";
        get<1>(lTrimStatement) = "123";
        cout << "ltrim('123123totn123', '123') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('xyxzyyyTOTN', 'xyz');
        get<0>(lTrimStatement) = "xyxzyyyTOTN";
        get<1>(lTrimStatement) = "xyz";
        cout << "ltrim('xyxzyyyTOTN', 'xyz') = " << storage.execute(lTrimStatement).front() << endl;

        //  SELECT ltrim('6372totn', '0123456789');
        get<0>(lTrimStatement) = "6372totn";
        get<1>(lTrimStatement) = "0123456789";
        cout << "ltrim('6372totn', '0123456789') = " << storage.execute(lTrimStatement).front() << endl;
    }

    //  https://www.techonthenet.com/sqlite/functions/rtrim.php

    //  SELECT rtrim('TechOnTheNet.com   ');
    cout << "rtrim('TechOnTheNet.com   ') = *" << storage.select(rtrim("TechOnTheNet.com   ")).front() << "*" << endl;

    //  SELECT rtrim('   TechOnTheNet.com   ');
    cout << "rtrim('   TechOnTheNet.com   ') = *" << storage.select(rtrim("   TechOnTheNet.com   ")).front() << "*"
         << endl;

    //  SELECT rtrim('TechOnTheNet.com    is great!   ');
    cout << "rtrim('TechOnTheNet.com    is great!   ') = *"
         << storage.select(rtrim("TechOnTheNet.com    is great!   ")).front() << "*" << endl;

    //  SELECT rtrim('123000', '0');
    cout << "rtrim('123000', '0') = *" << storage.select(rtrim("123000", "0")).front() << "*" << endl;

    //  SELECT rtrim('totn123123', '123');
    cout << "rtrim('totn123123', '123') = *" << storage.select(rtrim("totn123123", "123")).front() << "*" << endl;

    //  SELECT rtrim('123totn123123', '123');
    cout << "rtrim('123totn123123', '123') = *" << storage.select(rtrim("123totn123123", "123")).front() << "*" << endl;

    //  SELECT rtrim('TOTNxyxzyyy', 'xyz');
    cout << "rtrim('TOTNxyxzyyy', 'xyz') = *" << storage.select(rtrim("TOTNxyxzyyy", "xyz")).front() << "*" << endl;

    //  SELECT rtrim('totn6372', '0123456789');
    cout << "rtrim('totn6372', '0123456789') = *" << storage.select(rtrim("totn6372", "0123456789")).front() << "*"
         << endl;

    //  SELECT coalesce(10,20);
    cout << "coalesce(10,20) = " << storage.select(coalesce<int>(10, 20)).front() << endl;

    //  SELECT substr('SQLite substr', 8);
    cout << "substr('SQLite substr', 8) = " << storage.select(substr("SQLite substr", 8)).front() << endl;

    //  SELECT substr('SQLite substr', 1, 6);
    cout << "substr('SQLite substr', 1, 6) = " << storage.select(substr("SQLite substr", 1, 6)).front() << endl;

    //  SELECT hex(67);
    cout << "hex(67) = " << storage.select(hex(67)).front() << endl;

    //  SELECT quote('hi')
    cout << "SELECT quote('hi') = " << storage.select(quote("hi")).front() << endl;

    //  SELECT hex(randomblob(10))
    cout << "SELECT hex(randomblob(10)) = " << storage.select(hex(randomblob(10))).front() << endl;

    //  SELECT instr('something about it', 't')
    cout << "SELECT instr('something about it', 't') = " << storage.select(instr("something about it", "t")).front()
         << endl;

    {
        cout << endl;
        struct o_pos : alias_tag {
            static const std::string& get() {
                static const std::string res = "o_pos";
                return res;
            }
        };

        //  SELECT name, instr(abilities, 'o') o_pos
        //  FROM marvel
        //  WHERE o_pos > 0
        //  ORDER BY o_pos
        auto rows = storage.select(columns(&MarvelHero::name, as<o_pos>(instr(&MarvelHero::abilities, "o"))),
                                   where(greater_than(get<o_pos>(), 0)),
                                   order_by(get<o_pos>()));
        for(auto& row: rows) {
            cout << get<0>(row) << '\t' << get<1>(row) << endl;
        }
        cout << endl;
    }

    //  SELECT replace('AA B CC AAA','A','Z')
    cout << "SELECT replace('AA B CC AAA','A','Z') = " << storage.select(replace("AA B CC AAA", "A", "Z")).front()
         << endl;

    //  SELECT replace('This is a cat','This','That')
    cout << "SELECT replace('This is a cat','This','That') = "
         << storage.select(replace("This is a cat", "This", "That")).front() << endl;

    //  UPDATE contacts
    //  SET phone = REPLACE(phone, '410', '+1-410')
    storage.update_all(set(c(&Contact::phone) = replace(&Contact::phone, "410", "+1-410")));
    cout << "Contacts:" << endl;
    for(auto& contact: storage.iterate<Contact>()) {
        cout << storage.dump(contact) << endl;
    }

    //  SELECT round(1929.236, 2)
    cout << "SELECT round(1929.236, 2) = " << storage.select(round(1929.236, 2)).front() << endl;

    //  SELECT round(1929.236, 1)
    cout << "SELECT round(1929.236, 1) = " << storage.select(round(1929.236, 1)).front() << endl;

    //  SELECT round(1929.236)
    cout << "SELECT round(1929.236) = " << storage.select(round(1929.236)).front() << endl;

    //  SELECT round(0.5)
    cout << "SELECT round(0.5) = " << storage.select(round(0.5)).front() << endl;
#ifdef SQLITE_SOUNDEX
    //  SELECT soundex('Schn Thomson')
    cout << "SELECT soundex('Schn Thomson') = " << storage.select(soundex("Schn Thomson")).front() << endl;
#endif

    //  SELECT unicode('A')
    cout << "SELECT unicode('A') = " << storage.select(unicode("A")).front() << endl;

    //  SELECT unicode('Brush')
    cout << "SELECT unicode('Brush') = " << storage.select(unicode("Brush")).front() << endl;

    //  SELECT unicode(substr('Brush', 2))
    cout << "SELECT unicode(substr('Brush', 2)) = " << storage.select(unicode(substr("Brush", 2))).front() << endl;

    //  SELECT typeof(1)
    cout << "SELECT typeof(1) = " << storage.select(typeof_(1)).front() << endl;

    //  SELECT firstname, lastname, IFNULL(fax, 'Call:' || phone) fax
    //  FROM customers
    //  ORDER BY firstname;
    {
        cout << endl;
        cout << "SELECT firstname, lastname, IFNULL(fax, 'Call:' || phone) fax" << endl;
        cout << "FROM customers" << endl;
        cout << "ORDER BY firstname" << endl;
        cout << endl;

        auto rows = storage.select(columns(&Customer::firstName,
                                           &Customer::lastName,
                                           ifnull<std::string>(&Customer::fax, "Call:" || c(&Customer::phone))),
                                   order_by(&Customer::firstName));
        cout << "rows count: " << rows.size() << endl;
        for(auto& row: rows) {
            cout << get<0>(row) << '\t' << get<1>(row) << '\t' << get<2>(row) << endl;
        }
        cout << endl;
    }

    storage.update_all(
        set(c(&Contact::phone) = select(&Customer::phone, from<Customer>(), where(c(&Customer::id) == 1))));
    return 0;
}
