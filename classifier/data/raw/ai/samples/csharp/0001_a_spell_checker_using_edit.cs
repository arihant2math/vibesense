using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public sealed class SpellChecker
{
    private readonly string[] _words;

    public SpellChecker(IEnumerable<string> wordList)
    {
        _words = wordList
            .Where(word => !string.IsNullOrWhiteSpace(word))
            .Select(word => word.Trim().ToLowerInvariant())
            .Distinct()
            .ToArray();
    }

    public bool IsCorrect(string word)
    {
        return _words.Contains(word.Trim().ToLowerInvariant());
    }

    public IReadOnlyList<string> Suggest(string word, int maxSuggestions = 5)
    {
        if (string.IsNullOrWhiteSpace(word))
            return Array.Empty<string>();

        word = word.Trim().ToLowerInvariant();

        return _words
            .Select(candidate => new
            {
                Word = candidate,
                Distance = EditDistance(word, candidate)
            })
            .OrderBy(result => result.Distance)
            .ThenBy(result => result.Word)
            .Take(maxSuggestions)
            .Select(result => result.Word)
            .ToArray();
    }

    private static int EditDistance(string source, string target)
    {
        var previous = new int[target.Length + 1];
        var current = new int[target.Length + 1];

        for (var j = 0; j <= target.Length; j++)
            previous[j] = j;

        for (var i = 1; i <= source.Length; i++)
        {
            current[0] = i;

            for (var j = 1; j <= target.Length; j++)
            {
                var substitutionCost = source[i - 1] == target[j - 1] ? 0 : 1;

                current[j] = Math.Min(
                    Math.Min(current[j - 1] + 1, previous[j] + 1),
                    previous[j - 1] + substitutionCost);
            }

            (previous, current) = (current, previous);
        }

        return previous[target.Length];
    }
}

public static class Program
{
    public static void Main(string[] args)
    {
        var wordListPath = args.Length > 0 ? args[0] : "words.txt";

        if (!File.Exists(wordListPath))
        {
            Console.Error.WriteLine($"Word list not found: {wordListPath}");
            return;
        }

        var checker = new SpellChecker(File.ReadLines(wordListPath));

        Console.WriteLine("Enter a word to check, or type 'exit' to quit.");

        while (true)
        {
            Console.Write("> ");
            var input = Console.ReadLine();

            if (input == null || input.Equals("exit", StringComparison.OrdinalIgnoreCase))
                break;

            if (checker.IsCorrect(input))
            {
                Console.WriteLine("Correct");
            }
            else
            {
                Console.WriteLine("Not found");
                Console.WriteLine("Suggestions: " +
                    string.Join(", ", checker.Suggest(input)));
            }
        }
    }
}
