
#include <sqlite_orm/sqlite_orm.h>

#include <iostream>
#include <string>

using std::cout;
using std::endl;

struct DatetimeText {
    std::string d1;
    std::string d2;
};

int main() {
    using namespace sqlite_orm;

    auto storage = make_storage(
        "",
        make_table("datetime_text", make_column("d1", &DatetimeText::d1), make_column("d2", &DatetimeText::d2)));
    storage.sync_schema();

    //  SELECT date('now');
    auto now = storage.select(date("now")).front();
    cout << "date('now') = " << now << endl;

    //  SELECT date('now','start of month','+1 month','-1 day')
    auto nowWithOffset = storage.select(date("now", "start of month", "+1 month", "-1 day")).front();
    cout << "SELECT date('now','start of month','+1 month','-1 day') = " << nowWithOffset << endl;

    //  SELECT DATETIME('now')
    auto nowTime = storage.select(datetime("now")).front();
    cout << "DATETIME('now') = " << nowTime << endl;

    //  SELECT DATETIME('now','localtime')
    auto localTime = storage.select(datetime("now", "localtime")).front();
    cout << "DATETIME('now','localtime') = " << localTime << endl;

    //  INSERT INTO datetime_text (d1, d2)
    //  VALUES
    //  (
    //      datetime('now'),
    //      datetime('now', 'localtime')
    //  );
    DatetimeText datetimeText{
        storage.select(datetime("now")).front(),
        storage.select(datetime("now", "localtime")).front(),
    };
    storage.insert(datetimeText);

    cout << "SELECT * FROM datetime_text = " << storage.dump(storage.get_all<DatetimeText>().front()) << endl;

    auto nowWithOffset2 = storage.select(date("now", "+2 day")).front();
    cout << "SELECT date('now','+2 day') = " << nowWithOffset2 << endl;

    //  SELECT julianday('now')
    auto juliandayNow = storage.select(julianday("now")).front();
    cout << "SELECT julianday('now') = " << juliandayNow << endl;

    //  SELECT julianday('1776-07-04')
    auto oldJulianday = storage.select(julianday("1776-07-04")).front();
    cout << "SELECT julianday('1776-07-04') = " << oldJulianday << endl;

    //  SELECT julianday('now') + julianday('1776-07-04')
    auto julianSum = storage.select(julianday("now") + julianday("1776-07-04")).front();
    cout << "SELECT julianday('now') + julianday('1776-07-04') = " << julianSum << endl;

    //  SELECT julianday('now') - julianday('1776-07-04')
    auto julianDiff = storage.select(julianday("now") - julianday("1776-07-04")).front();
    cout << "SELECT julianday('now') - julianday('1776-07-04') = " << julianDiff << endl;

    //  SELECT (julianday('now') - 2440587.5) * 86400.0
    auto julianConverted = storage.select((julianday("now") - 2440587.5) * 86400.0).front();
    cout << "SELECT (julianday('now') - 2440587.5) * 86400.0 = " << julianConverted << endl;

    //  SELECT time('12:00', 'localtime')
    auto time12Local = storage.select(time("12:00", "localtime")).front();
    cout << "SELECT time('12:00', 'localtime') = " << time12Local << endl;

    //  SELECT time('12:00', 'utc')
    auto time12utc = storage.select(time("12:00", "utc")).front();
    cout << "SELECT time('12:00', 'utc') = " << time12utc << endl;

    //  SELECT strftime('%Y %m %d','now')
    auto strftimeRes = storage.select(strftime("%Y %m %d", "now")).front();
    cout << "SELECT strftime('%Y %m %d','now') = " << strftimeRes << endl;

    //  SELECT strftime('%H %M %S %s','now')
    auto strftimeRes2 = storage.select(strftime("%H %M %S %s", "now")).front();
    cout << "SELECT strftime('%H %M %S %s','now') = " << strftimeRes2 << endl;

    //  SELECT strftime('%s','now') - strftime('%s','2014-10-07 02:34:56')
    auto strftimeResSub = storage.select(strftime("%s", "now") - strftime("%s", "2014-10-07 02:34:56")).front();
    cout << "SELECT strftime('%s','now') - strftime('%s','2014-10-07 02:34:56') = " << strftimeResSub << endl;

    return 0;
}
