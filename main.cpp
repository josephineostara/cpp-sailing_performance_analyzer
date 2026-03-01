#include <vector>
#include <iostream>
#include <string>
#include <cctype>    
#include <fstream>   
#include <stdexcept> 
#include <iomanip>   

/* A structure type to represent a year/month/day combination */
struct Date
{
    int day{ 0 };
    int month{ 0 };
    int year{ 0 };
};

/* A structure type to represent an hour:minute pair */
struct TimeOfDay
{
    int hour{ 0 };
    int minute{ 0 };
};

/* A structure type to store the parameters of a particular sailing */
struct Sailing
{
    int route_number{ 0 };
    std::string source_terminal{ "" };
    std::string dest_terminal{ "" };
    std::string vessel_name{ "" };

    Date departure_date{};
    TimeOfDay scheduled_departure_time{};

    int expected_duration{ 0 };
    int actual_duration{ 0 };
};

/* A structure type to store aggregated performance data for a single
   route. */
struct RouteStatistics
{
    int route_number{ 0 };
    int total_sailings{ 0 };
    int late_sailings{ 0 };
};

/* A structure type to store aggregated performance data for a single
   day. */
struct DayStatistics
{
    Date date{};
    int total_sailings{ 0 };
    int late_sailings{ 0 };
};

/* Structure types to represent various issues that may occur during parsing */
struct IncompleteLineException
{
    unsigned int num_fields{}; 
};

struct EmptyFieldException
{
    unsigned int which_field{}; 
};

struct NonNumericDataException
{
    std::string bad_field{};
};

struct InvalidTimeException
{
    TimeOfDay bad_time{};
};


static inline bool is_all_whitespace(std::string const& s) {
    for (unsigned char ch : s) {
        if (!std::isspace(ch)) return false;
    }
    return true;
}

static std::vector<std::string> split_csv_commas(std::string const& line) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : line) {
        if (ch == ',') {
            out.push_back(cur);
            cur.clear();
        }
        else {
            cur.push_back(ch);
        }
    }
    out.push_back(cur); 
    return out;
}


static bool begins_with_digit(std::string const& s) {
    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    if (i >= s.size()) return false;
    return std::isdigit(static_cast<unsigned char>(s[i])) != 0;
}

static int parse_leading_int(std::string const& s) {
    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    bool neg = false;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        neg = (s[i] == '-');
        ++i;
    }
    long long val = 0;
    bool any = false;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
        any = true;
        val = val * 10 + (s[i] - '0');
        ++i;
    }
    if (!any) return 0;
    return static_cast<int>(neg ? -val : val);
}

Sailing parse_sailing(std::string const& input_line)
{
    auto fields = split_csv_commas(input_line);
    if (fields.size() != 11) {
        IncompleteLineException ex;
        ex.num_fields = static_cast<unsigned int>(fields.size());
        throw ex;
    }

    for (unsigned int i = 0; i < fields.size(); ++i) {
        if (fields[i].empty() || is_all_whitespace(fields[i])) {
            EmptyFieldException ex;
            ex.which_field = i;
            throw ex;
        }
    }

    const int numeric_idx[8] = { 0,3,4,5,6,7,9,10 };
    for (int k = 0; k < 8; ++k) {
        int idx = numeric_idx[k];
        if (!begins_with_digit(fields[idx])) {
            NonNumericDataException ex;
            ex.bad_field = fields[idx];
            throw ex;
        }
    }

    Sailing s;
    s.route_number = parse_leading_int(fields[0]);
    s.source_terminal = fields[1];
    s.dest_terminal = fields[2];
    s.departure_date.year = parse_leading_int(fields[3]);
    s.departure_date.month = parse_leading_int(fields[4]);
    s.departure_date.day = parse_leading_int(fields[5]);
    s.scheduled_departure_time.hour = parse_leading_int(fields[6]);
    s.scheduled_departure_time.minute = parse_leading_int(fields[7]);
    s.vessel_name = fields[8];
    s.expected_duration = parse_leading_int(fields[9]);
    s.actual_duration = parse_leading_int(fields[10]);

    if (s.scheduled_departure_time.hour < 0 || s.scheduled_departure_time.hour   > 23 ||
        s.scheduled_departure_time.minute < 0 || s.scheduled_departure_time.minute > 59) {
        InvalidTimeException ex;
        ex.bad_time = s.scheduled_departure_time;
        throw ex;
    }

    return s;
}

/* performance_by_route(sailings) */
std::vector<RouteStatistics> performance_by_route(std::vector<Sailing> const& sailings)
{
    std::vector<RouteStatistics> out;
    for (auto const& s : sailings) {
        int idx = -1;
        for (int i = 0; i < (int)out.size(); ++i) {
            if (out[i].route_number == s.route_number) { idx = i; break; }
        }
        if (idx == -1) {
            RouteStatistics rs;
            rs.route_number = s.route_number;
            rs.total_sailings = 0;
            rs.late_sailings = 0;
            out.push_back(rs);
            idx = (int)out.size() - 1;
        }
        out[idx].total_sailings += 1;
        if (s.actual_duration >= s.expected_duration + 5) {
            out[idx].late_sailings += 1;
        }
    }
    return out;
}

/* best_days(sailings) */
std::vector<DayStatistics> best_days(std::vector<Sailing> const& sailings)
{
    std::vector<DayStatistics> days; 
    for (auto const& s : sailings) {
        int idx = -1;
        for (int i = 0; i < (int)days.size(); ++i) {
            if (days[i].date.year == s.departure_date.year &&
                days[i].date.month == s.departure_date.month &&
                days[i].date.day == s.departure_date.day) {
                idx = i; break;
            }
        }
        if (idx == -1) {
            DayStatistics ds;
            ds.date = s.departure_date;
            ds.total_sailings = 0;
            ds.late_sailings = 0;
            days.push_back(ds);
            idx = (int)days.size() - 1;
        }
        days[idx].total_sailings += 1;
        if (s.actual_duration >= s.expected_duration + 5) {
            days[idx].late_sailings += 1;
        }
    }

    if (days.empty()) return {};

    double best_ratio = 1e300; 
    for (auto const& d : days) {
        if (d.total_sailings > 0) {
            double r = (double)d.late_sailings / (double)d.total_sailings;
            if (r < best_ratio) best_ratio = r;
        }
    }

    std::vector<DayStatistics> result;
    for (auto const& d : days) {
        if (d.total_sailings > 0) {
            double r = (double)d.late_sailings / (double)d.total_sailings;
            if (r == best_ratio) result.push_back(d);
        }
    }
    return result;
}

/* worst_days(sailings) */
std::vector<DayStatistics> worst_days(std::vector<Sailing> const& sailings)
{
    std::vector<DayStatistics> days; 
    for (auto const& s : sailings) {
        int idx = -1;
        for (int i = 0; i < (int)days.size(); ++i) {
            if (days[i].date.year == s.departure_date.year &&
                days[i].date.month == s.departure_date.month &&
                days[i].date.day == s.departure_date.day) {
                idx = i; break;
            }
        }
        if (idx == -1) {
            DayStatistics ds;
            ds.date = s.departure_date;
            ds.total_sailings = 0;
            ds.late_sailings = 0;
            days.push_back(ds);
            idx = (int)days.size() - 1;
        }
        days[idx].total_sailings += 1;
        if (s.actual_duration >= s.expected_duration + 5) {
            days[idx].late_sailings += 1;
        }
    }

    if (days.empty()) return {};

    double worst_ratio = -1.0;
    for (auto const& d : days) {
        if (d.total_sailings > 0) {
            double r = (double)d.late_sailings / (double)d.total_sailings;
            if (r > worst_ratio) worst_ratio = r;
        }
    }

    std::vector<DayStatistics> result;
    for (auto const& d : days) {
        if (d.total_sailings > 0) {
            double r = (double)d.late_sailings / (double)d.total_sailings;
            if (r == worst_ratio) result.push_back(d);
        }
    }
    return result;
}

/* You do not have to understand or modify these functions (although they
   are of the same level of difficulty as the other parts of the assignment) */
std::vector<Sailing> read_sailings(std::string const& input_filename)
{
    std::vector<Sailing> all_sailings;
    std::ifstream input_file;
    input_file.open(input_filename);

    int valid_sailings{ 0 };
    int total_lines{ 0 };

    if (input_file.is_open())
    {
        std::string line;
        while (std::getline(input_file, line))
        {
            total_lines++;
            try
            {
                Sailing s{ parse_sailing(line) };
                valid_sailings++;
                all_sailings.push_back(s);
            }
            catch (IncompleteLineException& e)
            {
                std::cout << "Line " << total_lines << " is invalid: ";
                std::cout << e.num_fields << " fields found." << std::endl;
            }
            catch (EmptyFieldException& e)
            {
                std::cout << "Line " << total_lines << " is invalid: ";
                std::cout << "Field " << e.which_field << " is empty." << std::endl;
            }
            catch (NonNumericDataException& e)
            {
                std::cout << "Line " << total_lines << " is invalid: ";
                std::cout << "\"" << e.bad_field << "\" is non-numeric." << std::endl;
            }
            catch (InvalidTimeException& e)
            {
                std::cout << "Line " << total_lines << " is invalid: ";
                std::cout << e.bad_time.hour << ":" << e.bad_time.minute << " is not a valid time." << std::endl;
            }
        }
        input_file.close();
    }
    else
    {
        throw std::runtime_error("Unable to open input file");
    }
    int invalid_sailings{ total_lines - valid_sailings };
    std::cout << "Read " << valid_sailings << " records." << std::endl;
    std::cout << "Skipped " << invalid_sailings << " invalid records." << std::endl;
    return all_sailings;
}

void print_sailing(Sailing const& sailing)
{
    std::cout << "Route " << sailing.route_number;
    std::cout << " (" << sailing.source_terminal << " -> " << sailing.dest_terminal << "): ";
    std::cout << sailing.departure_date.year << "-";
    std::cout << std::setfill('0') << std::setw(2) << sailing.departure_date.month << "-";
    std::cout << std::setfill('0') << std::setw(2) << sailing.departure_date.day << " ";
    std::cout << std::setfill('0') << std::setw(2) << sailing.scheduled_departure_time.hour << ":";
    std::cout << std::setfill('0') << std::setw(2) << sailing.scheduled_departure_time.minute << " ";
    std::cout << "[Vessel: " << sailing.vessel_name << "] ";
    std::cout << sailing.actual_duration << " minutes (" << sailing.expected_duration << " expected)" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: ./assignment_2 action input_filename" << std::endl;
        std::cout << "       where action is either 'route_summary' or 'days'" << std::endl;
        return 1;
    }

    std::string action{ argv[1] };
    std::string input_filename{ argv[2] };

    auto all_sailings{ read_sailings(input_filename) };

    if (action == "route_summary")
    {
        std::cout << "Performance by route:" << std::endl;
        auto statistics{ performance_by_route(all_sailings) };
        for (auto stats : statistics)
        {
            std::cout << "Route " << stats.route_number << ": " << stats.total_sailings << " sailings (" << stats.late_sailings << " late)" << std::endl;
        }
    }
    else if (action == "days")
    {
        auto best{ best_days(all_sailings) };
        auto worst{ worst_days(all_sailings) };
        std::cout << "Best days:" << std::endl;
        for (auto stats : best)
        {
            std::cout << stats.date.year << "-" << stats.date.month << "-" << stats.date.day << ": ";
            std::cout << stats.total_sailings << " sailings (" << stats.late_sailings << " late)" << std::endl;
        }
        std::cout << "Worst days:" << std::endl;
        for (auto stats : worst)
        {
            std::cout << stats.date.year << "-" << stats.date.month << "-" << stats.date.day << ": ";
            std::cout << stats.total_sailings << " sailings (" << stats.late_sailings << " late)" << std::endl;
        }
    }
    else
    {
        std::cout << "Invalid action " << action << std::endl;
    }

    return 0;
}
