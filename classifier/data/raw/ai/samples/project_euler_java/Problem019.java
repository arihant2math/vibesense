import java.time.DayOfWeek;
import java.time.LocalDate;

// Project Euler Problem 19: Sundays that fell on the first of a month during the twentieth century.
public class Problem019 {
    public static void main(String[] args) {
        int count = 0;
        for (int year = 1901; year <= 2000; year++) {
            for (int month = 1; month <= 12; month++) {
                if (LocalDate.of(year, month, 1).getDayOfWeek() == DayOfWeek.SUNDAY) count++;
            }
        }
        System.out.println(count);
    }
}
