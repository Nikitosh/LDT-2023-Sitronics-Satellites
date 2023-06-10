namespace {

// Returns true if `s` starts with `prefix`.
bool StartsWith(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

// Returns number padded with leading zeros with the length `len`.
std::string ToStringWithLength(long long n, int len) {
    std::vector<int> digits;
    while (n > 0) {
        digits.push_back(int(n % 10));
        n /= 10;
    }
    while ((int) digits.size() < len) {
        digits.push_back(0);
    }
    reverse(digits.begin(), digits.end());
    std::string result;
    for (int digit : digits) {
        result.push_back(char(digit + '0'));
    }
    return result;
}

// Returns number padded with spaces with the length `len`.
std::string ToStringWithLength(const std::string& s, int len) {
    return std::string(len - s.size(), ' ') + s;
}

template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

}