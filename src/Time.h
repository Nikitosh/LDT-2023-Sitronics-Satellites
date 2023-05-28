namespace {

// Returns if the year is leap.
static bool IsLeap(int year) {
    if (year % 400 == 0) {
        return true;
    } else if (year % 100 == 0) {
        return false;
    } else if (year % 4 == 0) {
        return true;
    } else {
        return false;
    }
}

// Returns partial sums of amount of days in the years.
vector<int> createPartialYearDays() {
    const int MAX_YEAR = 10000;
    vector<int> result(MAX_YEAR);
    for (int i = 1; i < MAX_YEAR; i++) {
        result[i] = result[i - 1] + 365 + (IsLeap(i) ? 1 : 0);
    }
    return result;
}

vector<string> MONTHS = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
vector<int> DAYS = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

}

// Helper struct to represent timestamps.
struct Time {
    int year = 0;
    int month = 0; // 0-indexing
    int day = 0; // 0-indexing
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millis = 0;

    const static vector<int> PARTIAL_YEAR_DAYS;
    
    // Parses timestamps in the format "1 Jun 2027 00:00:01.000".
    static Time Parse(stringstream& stream) {
        Time result;
        string month;
        char c;
        stream >> result.day >> month >> result.year >> result.hour >> c 
            >> result.minute >> c >> result.second >> c >> result.millis;
        result.month = int(find(MONTHS.begin(), MONTHS.end(), month) - MONTHS.begin());
        result.day--;
        return result;
    }

    // Creates timestamp from the numeric value.
    static Time FromTimestamp(long long timestamp) {
        Time result;
        int days = int(timestamp / (24 * 3600 * 1000));
        result.year = int(upper_bound(PARTIAL_YEAR_DAYS.begin(), PARTIAL_YEAR_DAYS.end(), days) 
            - PARTIAL_YEAR_DAYS.begin());
        days -= PARTIAL_YEAR_DAYS[result.year - 1];
        for (int i = 0; i < (int) DAYS.size(); i++) {
            int month_days = DAYS[i];
            if (i == 1 && IsLeap(result.year)) {
                month_days++;
            }
            if (days < month_days) {
                break;
            }
            result.month++;
            days -= month_days;
        }
        result.day = days;
        int rest = int(timestamp % (24 * 3600 * 1000));
        result.millis = rest % 1000;
        rest /= 1000;
        result.second = rest % 60;
        rest /= 60;
        result.minute = rest % 60;
        rest /= 60;
        result.hour = rest;
        return result;
    }

    // Returns numeric value of the timestamp.
    long long ToTimestamp() const {
        long long days = PARTIAL_YEAR_DAYS[year - 1];
        for (int i = 0; i < month; i++) {
            days += DAYS[i];
            if (i == 1 && IsLeap(year)) {
                days++;
            }
        }
        days += day;
        return (((days * 24 + hour) * 60 + minute) * 60 + second) * 1000 + millis;
    }

    // Return timestamp in the format "1 Jun 2027 00:00:01.000".
    string ToString() const {
        return to_string(day + 1) + " " + MONTHS[month] + " " + to_string(year) + " " 
            + ToStringWithLength(hour, 2) + ":" + ToStringWithLength(minute, 2) + ":" 
            + ToStringWithLength(second, 2) + "." + ToStringWithLength(millis, 3); 
    }
};

const vector<int> Time::PARTIAL_YEAR_DAYS = createPartialYearDays();
