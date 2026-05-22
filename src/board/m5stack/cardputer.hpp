#pragma once

namespace wrapper
{

class M5StackCardputer
{
    M5StackCardputer() = default;
    ~M5StackCardputer() = default;

    M5StackCardputer(const M5StackCardputer&) = delete;
    M5StackCardputer& operator=(const M5StackCardputer&) = delete;
    M5StackCardputer(M5StackCardputer&&) = delete;
    M5StackCardputer& operator=(M5StackCardputer&&) = delete;

   public:
    static M5StackCardputer& GetInstance()
    {
        static M5StackCardputer instance;
        return instance;
    }
    bool Init();
};

}  // namespace wrapper