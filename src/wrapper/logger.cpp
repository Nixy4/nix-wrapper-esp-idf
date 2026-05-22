#include "wrapper/logger.hpp"

using namespace wrapper;

Logger::Logger() {}

Logger::Logger(const std::string& tags)
{
    tags_list_.push_back(tags);
    RebuildTags();
}

Logger::~Logger() {}

void Logger::SetPrefix(const std::string& prefix) { prefix_ = prefix; }

void Logger::SetSeparator(const std::string& separator) { separator_ = separator; }

void Logger::SetSuffix(const std::string& suffix) { suffix_ = suffix; }

void Logger::SetTags(const std::vector<std::string>& tags)
{
    tags_list_ = tags;
    RebuildTags();
}

void Logger::SetTags(const std::initializer_list<std::string>& tags)
{
    tags_list_ = tags;
    RebuildTags();
}

void Logger::SetTag(int index, const std::string& tag)
{
    if (index >= 0 && index < static_cast<int>(tags_list_.size()))
    {
        tags_list_[index] = tag;
        RebuildTags();
    }
}

void Logger::ClearTags()
{
    tags_list_.clear();
    tags_.clear();
}

void Logger::RebuildTags()
{
    tags_.clear();

    // Pre-calculate required capacity to avoid multiple reallocations
    size_t capacity = prefix_.size() + suffix_.size();
    if (!tags_list_.empty())
    {
        for (const auto& tag : tags_list_)
        {
            capacity += tag.size();
        }
        capacity += separator_.size() * (tags_list_.size() - 1);
    }
    tags_.reserve(capacity);

    // Build the string efficiently
    tags_ += prefix_;
    if (!tags_list_.empty())
    {
        tags_ += tags_list_[0];
        for (size_t i = 1; i < tags_list_.size(); ++i)
        {
            tags_ += separator_;
            tags_ += tags_list_[i];
        }
    }
    tags_ += suffix_;
}
