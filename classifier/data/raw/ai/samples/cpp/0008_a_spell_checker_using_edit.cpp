#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int editDistance(const std::string& a, const std::string& b) {
    std::vector<int> previous(b.size() + 1), current(b.size() + 1);

    for (std::size_t j = 0; j <= b.size(); ++j)
        previous[j] = static_cast<int>(j);

    for (std::size_t i = 1; i <= a.size(); ++i) {
        current[0] = static_cast<int>(i);

        for (std::size_t j = 1; j <= b.size(); ++j) {
            int insertion = current[j - 1] + 1;
            int deletion = previous[j] + 1;
            int substitution = previous[j - 1] + (a[i - 1] != b[j - 1]);

            current[j] = std::min({insertion, deletion, substitution});
        }

        previous.swap(current);
    }

    return previous[b.size()];
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <word-list> [max-distance]\n";
        return 1;
    }

    int maxDistance = argc >= 3 ? std::stoi(argv[2]) : 2;
    std::ifstream dictionaryFile(argv[1]);

    if (!dictionaryFile) {
        std::cerr << "Unable to open word list.\n";
        return 1;
    }

    std::vector<std::string> dictionary;
    std::string word;

    while (dictionaryFile >> word)
        dictionary.push_back(word);

    while (std::cin >> word) {
        std::vector<std::pair<int, std::string>> matches;

        for (const auto& candidate : dictionary) {
            int distance = editDistance(word, candidate);
            if (distance <= maxDistance)
                matches.emplace_back(distance, candidate);
        }

        std::sort(matches.begin(), matches.end());

        std::cout << word << ":";
        if (matches.empty()) {
            std::cout << " no suggestions";
        } else {
            for (const auto& [distance, suggestion] : matches)
                std::cout << " " << suggestion << "(" << distance << ")";
        }
        std::cout << '\n';
    }

    return 0;
}
