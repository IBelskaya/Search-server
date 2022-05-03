#pragma once
#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(...) LogDuration UNIQUE_VAR_NAME_PROFILE(__VA_ARGS__)

#include <chrono>
#include <iostream>
class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& id, std::ostream& cer);

    ~LogDuration();
       
private:
    const std::string id_;
    std::ostream& cerr;
    const Clock::time_point start_time_ = Clock::now();
};