// Project Euler 59: decrypt the three-letter lowercase XOR cipher and sum its ASCII text.
import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.zip.GZIPInputStream;

public class Problem059 {
    // The canonical Project Euler cipher file, gzip-compressed to keep this program self-contained.
    private static final String COMPRESSED_CIPHER = "H4sICL2xbWoCA3AwNTlfY2lwaGVyLnR4dADFV+2O5CAMeyF+kECBvv+LXWIHyrQzp929lU470vYDSOLYhpaWVNPIyf9q0pL0SHIm6WkMe1D9RiWJ+L0N9RH2"
            + "O1OzQXZbbZ6/OXyIP0iSk450pIbbA/cIYRclx4WvJL6GL6Z+0cTmnL6y3TYEtMH+vkQ2lolNzD6x+xr21H4+IfsCA3fCFTxi9RW6RckRSzzEwJgDI4atzXQO"
            + "/vfXXo0H6hglSNum4y5J80cDGRersQQyhhNQGSiN1xFBLMg9krzgQUAFgI5AELUh5WqwHL7C0VFEQ+HnjDbnInD1Kf4YXUJPrBcn0j39XtEUolE8/PyzgN5K"
            + "G80f0auLHcQdDS05GkKwCZd4mQJ+FLSjADYtN3QiTWGVe1s2rDDeYpR4rVgo2gtU24GWF5DEUjVoxv1JL48xB24NEXLvtEgtjRYIyeF89iK6X/gIFok6veyr"
            + "c0HpHowIfCtmsL4jevPC/G18EKzgYqQckFNjayxJToXdsZROzmbyK7j5DPeW7BypW0WNeu+kxeKN3c1mxQwlNUQvLEDAmw5ydFEusfrLSFce5aBYdaVNZpBD"
            + "ELj9l+HJnmjMgUQHZLbztQIq1FFfFhEYHZ2CvrUK2HryEyoueKXPMQc0qUEiGlO58Hpywb1ztuojPtPbqGB3ph52udno5sk0FMf+TNMuHMMGYep8MilGOSDB"
            + "nRkGbOuROF91WownUEgYckY/GeEzv7U602BNJ0F26CYWI6aUiVII1c1qeaE+AmMXq2FHjIIgqKPbr6FxWEAnlT0a0yBLTA9nRGW3dDnJt9tzlbBtmTk2SHXt"
            + "xKzibrSI3PE4TxwERJ7eoxqtlHFlcgQwV5if+cKstkdxlk7KsfmPSPbaf6lAuHK333AzHgKT7n7R1VE3620Vr8ybK54IxuOCJKuXSl99Z6C8EUEz9jkeOoq8"
            + "KylH2TLSnbckgm8PFC03yV2Rc0/n6ecZ5YNmma5roj+sWuL64oRVbl5mdDI2+saUgVdzmHoPOBqX0gllB15TA2X+dxUrz2kedzZKCUKPF/UyAMXxrM2D3zxG"
            + "lNBjuXEoL4LcXWIyqv6q4VADhbvjSLAPagGKbmfsnHO3ujO+hgLZJTwOb5mafuz0lblBQqF/7yu9vnE/3He9u4JIC9rTvlbbxe4sOAFQDbUvt/n2nrQ89u6f"
            + "ncJdggSFen6cjjSsjGchG+OnzO7cezM4x6u1cvhbj/2YZn2dXWhqOdS8TsLPw/I/HhI6TsZNV8vdF079vgd+cTNwHaLgIvGl9EYugrn38yC7GQZEekQSsXk4"
            + "x6mG8ytbOktSDW0CXGr9w+T/pOLt4LVtqchCcPKtMTc+ZXrQqOQXxb8TFXCOD9RBR5wNyHFkJzdlXKX0naOReRzX1qfRkzrlE6oc0BD6xGEcHzh/OS1SBH0S"
            + "H19i94+k2SuNxvoBYplSngI8tyKV37ZtxvST8h8YCiD63g8AAA==";

    public static void main(String[] args) throws Exception {
        int[] cipher = cipher();
        int bestScore = Integer.MIN_VALUE;
        int[] bestText = null;
        for (int a = 'a'; a <= 'z'; a++) for (int b = 'a'; b <= 'z'; b++)
            for (int c = 'a'; c <= 'z'; c++) {
                int[] key = {a, b, c};
                int[] text = decrypt(cipher, key);
                int score = englishScore(text);
                if (score > bestScore) {
                    bestScore = score;
                    bestText = text;
                }
            }
        int sum = 0;
        for (int value : bestText) sum += value;
        System.out.println(sum);
    }

    private static int[] cipher() throws Exception {
        byte[] compressed = Base64.getDecoder().decode(COMPRESSED_CIPHER);
        String csv;
        try (GZIPInputStream input = new GZIPInputStream(new ByteArrayInputStream(compressed))) {
            csv = new String(input.readAllBytes(), StandardCharsets.US_ASCII);
        }
        String[] fields = csv.trim().split(",");
        int[] result = new int[fields.length];
        for (int i = 0; i < result.length; i++) result[i] = Integer.parseInt(fields[i]);
        return result;
    }

    private static int[] decrypt(int[] cipher, int[] key) {
        int[] text = new int[cipher.length];
        for (int i = 0; i < cipher.length; i++) text[i] = cipher[i] ^ key[i % 3];
        return text;
    }

    private static int englishScore(int[] text) {
        int score = 0;
        for (int value : text) {
            if (value < 32 || value > 126) return Integer.MIN_VALUE / 2;
            char ch = Character.toLowerCase((char) value);
            if (ch == ' ') score += 12;
            else if ("etaoinshrdlu".indexOf(ch) >= 0) score += 5;
            else if (ch >= 'a' && ch <= 'z') score += 2;
            else if (".,;:'!?-()".indexOf(ch) >= 0) score++;
        }
        return score;
    }
}
