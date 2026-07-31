from pathlib import Path
import argparse
import re


def edit_distance(first, second):
    rows = len(first) + 1
    columns = len(second) + 1

    distances = [[0] * columns for _ in range(rows)]

    for row in range(rows):
        distances[row][0] = row

    for column in range(columns):
        distances[0][column] = column

    for row in range(1, rows):
        for column in range(1, columns):
            insertion = distances[row][column - 1] + 1
            deletion = distances[row - 1][column] + 1
            substitution = distances[row - 1][column - 1]

            if first[row - 1] != second[column - 1]:
                substitution += 1

            distances[row][column] = min(insertion, deletion, substitution)

    return distances[-1][-1]


def load_word_list(path):
    words = set()

    for line in Path(path).read_text(encoding="utf-8").splitlines():
        word = line.strip().lower()

        if word:
            words.add(word)

    return sorted(words)


def find_suggestions(word, word_list, maximum_distance):
    suggestions = []

    for candidate in word_list:
        distance = edit_distance(word, candidate)

        if distance <= maximum_distance:
            suggestions.append((distance, candidate))

    suggestions.sort()
    return [candidate for _, candidate in suggestions]


def check_text(text, word_list, maximum_distance):
    known_words = set(word_list)
    words = re.findall(r"[A-Za-z]+", text)

    misspelled_words = {}

    for word in words:
        normalized_word = word.lower()

        if normalized_word not in known_words:
            misspelled_words.setdefault(
                word,
                find_suggestions(
                    normalized_word,
                    word_list,
                    maximum_distance,
                ),
            )

    return misspelled_words


def main():
    parser = argparse.ArgumentParser(description="Check spelling using edit distance.")
    parser.add_argument("text_file", help="Path to the text file to check.")
    parser.add_argument("word_list", help="Path to a word list, one word per line.")
    parser.add_argument(
        "-d",
        "--max-distance",
        type=int,
        default=2,
        help="Maximum edit distance for suggestions.",
    )
    args = parser.parse_args()

    if args.max_distance < 0:
        parser.error("--max-distance must be zero or greater")

    word_list = load_word_list(args.word_list)
    text = Path(args.text_file).read_text(encoding="utf-8")
    misspelled_words = check_text(text, word_list, args.max_distance)

    if not misspelled_words:
        print("No spelling errors found.")
        return

    for word, suggestions in misspelled_words.items():
        if suggestions:
            print(f"{word}: {', '.join(suggestions)}")
        else:
            print(f"{word}: no suggestions")


if __name__ == "__main__":
    main()

