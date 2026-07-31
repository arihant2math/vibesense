import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public final class CliConfig {
    private CliConfig() {
    }

    public record Config(
            String host,
            int port,
            boolean debug,
            List<String> files
    ) {
        public Config {
            Objects.requireNonNull(host, "host");
            Objects.requireNonNull(files, "files");

            if (host.isBlank()) {
                throw new IllegalArgumentException("host must not be blank");
            }
            if (port < 1 || port > 65535) {
                throw new IllegalArgumentException("port must be between 1 and 65535");
            }

            files = List.copyOf(files);
        }
    }

    public static final class ParseException extends Exception {
        public ParseException(String message) {
            super(message);
        }
    }

    public static Config parse(String[] args) throws ParseException {
        Objects.requireNonNull(args, "args");

        String host = "localhost";
        int port = 8080;
        boolean debug = false;
        List<String> files = new ArrayList<>();

        for (int i = 0; i < args.length; i++) {
            String argument = Objects.requireNonNull(args[i], "args[" + i + "]");

            switch (argument) {
                case "--host" -> {
                    ensureValueAvailable(args, i, argument);
                    host = args[++i];
                }
                case "--port" -> {
                    ensureValueAvailable(args, i, argument);
                    String value = args[++i];

                    try {
                        port = Integer.parseInt(value);
                    } catch (NumberFormatException e) {
                        throw new ParseException("invalid port: " + value);
                    }
                }
                case "--debug" -> debug = true;
                case "--no-debug" -> debug = false;
                case "--help", "-h" -> throw new ParseException(usage());
                default -> {
                    if (argument.startsWith("-")) {
                        throw new ParseException("unknown option: " + argument);
                    }
                    files.add(argument);
                }
            }
        }

        try {
            return new Config(host, port, debug, files);
        } catch (IllegalArgumentException e) {
            throw new ParseException(e.getMessage());
        }
    }

    private static void ensureValueAvailable(
            String[] args,
            int index,
            String option
    ) throws ParseException {
        if (index + 1 >= args.length || args[index + 1].startsWith("-")) {
            throw new ParseException("missing value for " + option);
        }
    }

    public static String usage() {
        return """
                Usage: app [options] [files...]

                Options:
                  --host HOST       Server host (default: localhost)
                  --port PORT       Server port (default: 8080)
                  --debug           Enable debug logging
                  --no-debug        Disable debug logging
                  -h, --help        Show this help
                """;
    }

    public static void main(String[] args) {
        try {
            Config config = parse(args);
            System.out.println(config);
        } catch (ParseException e) {
            System.err.println(e.getMessage());
            System.exit(2);
        }
    }
}
