// Project Euler Problem 17: letters used when spelling the numbers one through one thousand.
public class Problem017 {
    private static final String[] ONES = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
    private static final String[] TEENS = {"ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
    private static final String[] TENS = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};

    public static void main(String[] args) {
        int total = 0;
        for (int number = 1; number <= 1000; number++) total += words(number).length();
        System.out.println(total);
    }

    private static String words(int number) {
        if (number == 1000) return "onethousand";
        if (number >= 100) {
            String result = ONES[number / 100] + "hundred";
            return number % 100 == 0 ? result : result + "and" + words(number % 100);
        }
        if (number >= 20) return TENS[number / 10] + ONES[number % 10];
        if (number >= 10) return TEENS[number - 10];
        return ONES[number];
    }
}
