#include "log_duration.h"

LogDuration::LogDuration(const std::string& id, std::ostream& cer)
    : id_(id)
    , cerr(cer) {}

LogDuration::~LogDuration() {
    using namespace std::chrono;
    using namespace std::literals;
    
    const auto end_time = Clock::now();
    const auto dur = end_time - start_time_;
    stream_ << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << endl;
}
