namespace {

// Returns true if `s` starts with `prefix`.
bool StartsWith(const string& s, const string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

// Returns number padded with leading zeros with the length `len`.
string ToStringWithLength(long long n, int len) {
    vector<int> digits;
    while (n > 0) {
        digits.push_back(int(n % 10));
        n /= 10;
    }
    while ((int) digits.size() < len) {
        digits.push_back(0);
    }
    reverse(digits.begin(), digits.end());
    string result;
    for (int digit : digits) {
        result.push_back(char(digit + '0'));
    }
    return result;
}

// Returns number padded with spaces with the length `len`.
string ToStringWithLength(const string& s, int len) {
    return string(len - s.size(), ' ') + s;
}

}