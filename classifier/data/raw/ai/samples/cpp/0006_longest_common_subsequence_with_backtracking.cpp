#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

string longestCommonSubsequence(const string& first, const string& second) {
    int rows = first.size();
    int columns = second.size();

    vector<vector<int>> lengths(rows + 1, vector<int>(columns + 1, 0));

    for (int i = 1; i <= rows; ++i) {
        for (int j = 1; j <= columns; ++j) {
            if (first[i - 1] == second[j - 1]) {
                lengths[i][j] = lengths[i - 1][j - 1] + 1;
            } else {
                lengths[i][j] = max(lengths[i - 1][j], lengths[i][j - 1]);
            }
        }
    }

    string result;
    int i = rows;
    int j = columns;

    while (i > 0 && j > 0) {
        if (first[i - 1] == second[j - 1]) {
            result.push_back(first[i - 1]);
            --i;
            --j;
        } else if (lengths[i - 1][j] >= lengths[i][j - 1]) {
            --i;
        } else {
            --j;
        }
    }

    reverse(result.begin(), result.end());
    return result;
}

int main() {
    string first;
    string second;

    cin >> first >> second;

    cout << longestCommonSubsequence(first, second) << '\n';

    return 0;
}
