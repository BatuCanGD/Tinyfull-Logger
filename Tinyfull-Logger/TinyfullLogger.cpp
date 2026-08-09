#include <iostream>
#include <array>
#include <cctype>
#include <limits>
#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <format>
#include <print>
#include "TinyfullLogger.hpp"

namespace {
    template <typename T>
    [[nodiscard]] T getInput(){
        T t{};
        if constexpr (std::is_same_v<T, std::string>){
            std::getline(std::cin, t);
        }else{
            while(!(std::cin >> t)){
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                Log::Message(Log::Level::Error, Log::Category::Input, "You can't shove letters or big numbers into a tiny number box");
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        return t;
    }
}

constexpr auto getPositivePhrases() {
    return std::array<std::string_view, 10>{"yes","yessir","y","yup","sure","yeah","yea","ye","ok","yep"};
}
constexpr auto getAntagonizingPhrases(){
    return std::array<std::string_view, 7>{"fuck you","fuck off","fuck no","fk no","kill yourself","what the fuck are you on","why the fuck would it be incorrect"};
}
constexpr auto getNegativePhrases() {
    return std::array<std::string_view, 10>{"no","n","dont think so","nah","nope","never","nuh uh","nopedynope","nopers","nup"};
}

int main(){
    int t = ::getInput<int>();
    Log::Message(Log::Level::Info, Log::Category::Input, "Is your number... THIS?: {}", t);

    std::string y = ::getInput<std::string>();
    std::ranges::transform(y, y.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    bool is_positive     =  std::ranges::any_of(getPositivePhrases(),     [&y](const auto& p){ return p == y; });
    bool is_negative     =  std::ranges::any_of(getNegativePhrases(),     [&y](const auto& p){ return p == y; });
    bool is_antagonizing =  std::ranges::any_of(getAntagonizingPhrases(), [&y](const auto& p){ return p == y; });

    if (is_positive)            Log::Message(Log::Level::Success, Log::Category::Input, "Thats amazing to hear!! :D");
    else if (is_negative)       Log::Message(Log::Level::Error, Log::Category::Input, "Thats sad to hear! :(");
    else if (is_antagonizing)   Log::Message(Log::Level::Fatal, Log::Category::Input, "bro...");
    else                        Log::Message(Log::Level::Info, Log::Category::Input, "ok... :P");

    std::cin.get();

    return 0;
}
