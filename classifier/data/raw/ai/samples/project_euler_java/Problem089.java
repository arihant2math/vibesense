import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.zip.GZIPInputStream;

/** Project Euler 089: characters saved by minimal Roman numeral notation. */
public class Problem089 {
    // Gzip-compressed official list of one thousand Roman numerals.
    private static final String DATA = "H4sIAAAAAAAC/21aS7LkMAjb+1pmkyp76+L+J5nEICH6zWYmL534g0FIkL33trncn2d8V/O7fv/4/tq2/Dx5vc3nGfnAvfVenvvTtO+dk8/N+f4+8iLexEu8" +
            "nQ/nbP68z30P3WfuD/4NPe9/eGt6zGv3amDcnDNWkFvwXJhNTolNvL/OeizGe0ebW7aQ679zPmfs3PCdpOxxl+FitbyfN7+7KxaK38Mad6J3tPtY7FrNcS+n" +
            "3Ydyz68lduz4/f1b0OR+sdsHU8Yr76vGpcYMnAuWGLnEXBbOAkb0VWPGSEbfyIVztBk7yAlGjQpD7hwac6u/hHXuOte38P1498R3+8vL567ZTxunVloHEod1" +
            "bfVdjuum7dlVO5+53pz1jHuI4ddwwDtnWCxiJb3wWu5g2NywlaXSka4/z6VmeM2SHvOcsbm5z45x2mEXgxPI3NejGRjXbhFJY8YJ3kNxGC+OJiYOF7sLzNHl" +
            "UPKZa+13shHHvHhwhy6eDrUqQCsELJ2a/o4AX87Z7oLE/jQl3WvFs/h53cc/yxKQXGd8lwmrXlsvh11N4GEumi3mlyM+8U+eMfwIEeGx1VjhPQ0vl1BgMOBE" +
            "rWyWH4Snv4OVvz3p/d48eWlgPoCozxKTNg1zjP/MeW9/MPLuZ40CKKxzAmzendSsq5zWjngvXCP85cA4q0LxmkeMnYOk2S+yv574ORTX/p3FuCuXdV8wq9SQ" +
            "Vqq9fRseccpx4zmFIqfyFt6YcIIdeFBAHNHWoO0U4I2VsHPUCAkoK6IZGzFNBL4eZMnuBIiYcLu5T8fGtCinS3eLhPUQ/NpZ4TCNRzTswJet/HNG6s7lzJvW" +
            "x6ZVTXKIBeakCyHicOR/UM2bw3LBC+ieqBQeB9SeLoGSVo/Iv4YB/ufy5RBMDwyJs3Lc84hjcmtLHlqVOvoRvQn2i1FD1guETcd5p+Mu4gchHZpQ8i9DPpg2" +
            "NrwMo0qMI2SmpEMkW+St1xhv/Fq538fDhMsQ1zQHwqYWMGPBVO6lOEuiWjMgny4zBwybT6U+YuBKqgB4H3m0n8cvwZEyQ2U8ot7T8SYwtpJLxKZmvdmTPvNn" +
            "psqjazoCfUgpmRyP5K8RVr+Lk+xCg/kHpZnKa+M3LMdWBLoLHMkDmCzLhZiskvReD/uy+NHQGEm+RsBEJzqOA3rHfpQ/YodLyFAiPbgmklcuKgA1gXxXmM67" +
            "rmcUHhGz7tmT3wYgFvHNHBks5JrmEd8KZi9ZvHyBvE4JEQ/tCMCQES7EabIEiSVjgFs8vWr8XN17t4I4LRu/ifbJlGPpLFaMj+QdwUjqWkNWJt8C6qujN8y1" +
            "l6ALxcWjXJwZxwcA9ggfTb9I7CCX2YWF0FBOn3gEX6m28ChTL7AchFxVQA3gSNkIcZhoctJgqUWxcIA302QCvzHj86HyciX5V0CpWfjUmMLUXbMWVnjK1N8w" +
            "Swg5QbYnjpzCkzU/yadhtg5cVGUM67G57elPZ0mMklhSE1JB8H64ZVqu+ONMUkKYKLcqZk9SVV51VFZj+LLxFG7rIHOnAvwwnseeBe1THM6T6cT/63TVlGFT" +
            "4HAHRpomc/mhYFfZPilk0qYHtvME5vuGKX4TmU8Sz6DRSOiu+eN0j7tMrYoOWIaVarEsFJT+XKHemTcaYtkMBwJhIn59qwKh3501CKIYwHUyoID9MWURn9jK" +
            "Fu8vlZCDSo50YGaeMvTOXF7RXQnMf4ToZqK1SRi18yPHewFIuJqkrQwBX88ochQM0n/UWAldhTgmKazrqBQ7VcoZSxWAUAsQs+6GCM337mUf1pQRBL2ge6ZP" +
            "ygIGk2gqGKwJOFE+SFUb0qhVPBZx9rL/Vl5AHIigGomKcLrwFltaS/BR4kg5ohe3QkFp4cC9M0D1sBaFVDruVS+iixcz5jumJcBCZMpOk/qVqwt1KS7pCTvf" +
            "WrDTwhQSdGSg8s3MtBMq3sJSe4lvhKnuuQz7C4wUCF1qzxIjeLbFRyjXpIh7aspKOYxTzv0nQOxSX8hhFHmylDSKt7CsqIK4G5uFRA1Zb5rQOERNjLRj5fM3" +
            "8qJcBXloFyfXQU2WvE8VPAlWVFCkwHqxbNpSLdJtj2LQEQGZ3oSSsBQTko+DRZQnz120aRhVKNNz48mhyVFVtCoFWS9/HtaU5qkS2kjJ3xQP7hA6R+WEqaTI" +
            "pXb+uWoWHoLjLSnkeomGMGfWS8k2WyU/lvhTTEYuGoGvm8klFriUQhDxyMPO8xSVFUQ8JYOR8Vb6CgolxtCruhJrF1K9bLWqQtUmH1nooBulRjcsxqKWqlmU" +
            "s6J0cjMJpEHvPoSIgxjG5ghgKkiLMdDEkrgjIwfFv4M6ZSXS2XLB9COhfrTUNoLBpTlQivS8f7TS09x8aQmHa2z1rpXdmvD46u5oCZ/mp6MpG/80MmqUm8J1" +
            "nbHbcU8qR6odEx38Q+9dGhYafQE1s3bYK7gu1KcaJRZC3qoDIIWlBIenszhnFEpRU5N/uVfqGiDfD0vyR+nRQNabxpJOid8kp+FCMtn03kDJmnmEZAaCN0iU" +
            "RAr8oDK7kFyjzxDdz6k6SKNpIwNlSxesPLcBuE3lfsEMwZNtkR5CdWlZTpX4GqYiNDkuWBMYdWLNUUwn0XTl3gF19J9Uq2z8SR3EMo1k+R7SuBWfYc+FRFVV" +
            "f31oZAjDBQ8UeVXIpTCFbVxI9x8yJojSott7mwagUfB5ntaHyq4pUKbmV+C+5pK4QsRkLwTsdv/hQsHIQEYW+59Vl3iaBHGyNfHQGu00qsc2RWcrh20EDdCk" +
            "i/m+dBc928ZBTaV3JHmvIqwCrtSDQc2GstiVCVX/cBkITOnr/arl6oz0PnBT1CwMJLXcOLuCV1xWqx5Y1oGVcTGkudoUgBD4BML6csAiA2R3TF0RpYQqnElW" +
            "Wip2BkvjPBOb4swOWKbsLWaHRgfjUb460IPOaDMW8pb0JgirrTUAplZBK989zBQUzIxH+jmtodEIPspKZ+w9fzW06xcKbUWtfqOa+oGaG8jMqz7WKPDLosbz" +
            "n+860hHRaR5CG0bxxt4BXiX+HmazoyrQ5VONKuoy3n9abFrfZFGnN7gfHzDyka5Q6+GM1u/bWgCBLmhdKZ5NE0LPKS9ttYrTJcqool4UAruFF796oRNVEdR+" +
            "mNlvW0w//ljiDOiNC8c0adTpmPLlCmh1KbNo9biwqYPgFlLFrmme32i11VRu+QJVln4tUOZ6enk69GNFXWJ/6dPeEWnft4Qm2vzKAEzzD1Xp1aVWukqtk7nX" +
            "iGVFj+bzx7WWVgFQzpDC/Olf2BwJL1TzRi9qbjr81G+36J4MiWgftq9gZo8gb99RJcBk0ayVE1pJD+2+1CIGIq3VqJlKI8/mH9xU7+d5JgAA";

    public static void main(String[] args) throws Exception {
        int saved = 0;
        byte[] packed = Base64.getDecoder().decode(DATA);
        try (BufferedReader in = new BufferedReader(new InputStreamReader(
                new GZIPInputStream(new ByteArrayInputStream(packed)), StandardCharsets.US_ASCII))) {
            for (String numeral; (numeral = in.readLine()) != null; )
                saved += numeral.length() - minimalLength(valueOf(numeral));
        }
        System.out.println(saved);
    }

    private static int valueOf(String numeral) {
        int value = 0, previous = 0;
        for (int i = numeral.length() - 1; i >= 0; i--) {
            int current = value(numeral.charAt(i));
            value += current < previous ? -current : current;
            if (current > previous) previous = current;
        }
        return value;
    }

    private static int minimalLength(int value) {
        int[] values = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
        int[] lengths = {1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1};
        int length = 0;
        for (int i = 0; i < values.length; i++) while (value >= values[i]) {
            value -= values[i]; length += lengths[i];
        }
        return length;
    }

    private static int value(char c) {
        switch (c) {
            case 'I': return 1; case 'V': return 5; case 'X': return 10;
            case 'L': return 50; case 'C': return 100; case 'D': return 500;
            case 'M': return 1000; default: throw new IllegalArgumentException();
        }
    }
}
