public final class UrlParser {
    private final CharSequence input;
    private final int schemeStart, schemeEnd;
    private final int hostStart, hostEnd;
    private final int pathStart, pathEnd;
    private final int queryStart, queryEnd;

    private UrlParser(CharSequence input, int schemeStart, int schemeEnd,
                      int hostStart, int hostEnd, int pathStart, int pathEnd,
                      int queryStart, int queryEnd) {
        this.input = input;
        this.schemeStart = schemeStart;
        this.schemeEnd = schemeEnd;
        this.hostStart = hostStart;
        this.hostEnd = hostEnd;
        this.pathStart = pathStart;
        this.pathEnd = pathEnd;
        this.queryStart = queryStart;
        this.queryEnd = queryEnd;
    }

    public static UrlParser parse(CharSequence url) {
        if (url == null) {
            throw new NullPointerException("url");
        }

        int length = url.length();
        int schemeEnd = -1;
        int colon = indexOf(url, ':', 0);

        if (colon > 0 && isValidScheme(url, colon)) {
            schemeEnd = colon;
        }

        int position = schemeEnd >= 0 ? schemeEnd + 1 : 0;
        int hostStart = -1;
        int hostEnd = -1;

        if (position + 1 < length && url.charAt(position) == '/'
                && url.charAt(position + 1) == '/') {
            hostStart = position + 2;
            hostEnd = hostStart;

            while (hostEnd < length) {
                char c = url.charAt(hostEnd);
                if (c == '/' || c == '?' || c == '#') {
                    break;
                }
                hostEnd++;
            }

            position = hostEnd;
        }

        int pathStart = position;
        int queryStart = -1;
        int fragmentStart = indexOf(url, '#', position);
        int pathLimit = fragmentStart >= 0 ? fragmentStart : length;
        int questionMark = indexOf(url, '?', position);

        if (questionMark >= 0 && questionMark < pathLimit) {
            queryStart = questionMark + 1;
        }

        int pathEnd = queryStart >= 0 ? queryStart - 1 : pathLimit;
        int queryEnd = queryStart >= 0 ? pathLimit : -1;

        return new UrlParser(
                url,
                schemeEnd >= 0 ? 0 : -1,
                schemeEnd,
                hostStart,
                hostEnd,
                pathStart,
                pathEnd,
                queryStart,
                queryEnd
        );
    }

    private static boolean isValidScheme(CharSequence value, int end) {
        if (!isAsciiLetter(value.charAt(0))) {
            return false;
        }

        for (int i = 1; i < end; i++) {
            char c = value.charAt(i);
            if (!isAsciiLetter(c) && !isAsciiDigit(c) && c != '+' && c != '-' && c != '.') {
                return false;
            }
        }
        return true;
    }

    private static boolean isAsciiLetter(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    private static boolean isAsciiDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private static int indexOf(CharSequence value, char target, int start) {
        for (int i = start, length = value.length(); i < length; i++) {
            if (value.charAt(i) == target) {
                return i;
            }
        }
        return -1;
    }

    public boolean hasScheme() {
        return schemeStart >= 0;
    }

    public boolean hasHost() {
        return hostStart >= 0;
    }

    public boolean hasQuery() {
        return queryStart >= 0;
    }

    public CharSequence scheme() {
        return slice(schemeStart, schemeEnd);
    }

    public CharSequence host() {
        return slice(hostStart, hostEnd);
    }

    public CharSequence path() {
        return slice(pathStart, pathEnd);
    }

    public CharSequence query() {
        return slice(queryStart, queryEnd);
    }

    private CharSequence slice(int start, int end) {
        if (start < 0 || end < start) {
            return "";
        }
        return new Slice(input, start, end);
    }

    private static final class Slice implements CharSequence {
        private final CharSequence source;
        private final int start;
        private final int end;

        Slice(CharSequence source, int start, int end) {
            this.source = source;
            this.start = start;
            this.end = end;
        }

        @Override
        public int length() {
            return end - start;
        }

        @Override
        public char charAt(int index) {
            if (index < 0 || index >= length()) {
                throw new IndexOutOfBoundsException();
            }
            return source.charAt(start + index);
        }

        @Override
        public CharSequence subSequence(int from, int to) {
            if (from < 0 || to > length() || from > to) {
                throw new IndexOutOfBoundsException();
            }
            return new Slice(source, start + from, start + to);
        }

        @Override
        public String toString() {
            return source.subSequence(start, end).toString();
        }
    }
}
