import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Stream;

public class Main {
    public static Map<String, Long> getFileSizesByExtension(Path directory) throws IOException {
        Map<String, Long> sizes = new TreeMap<>();

        try (Stream<Path> paths = Files.walk(directory)) {
            paths.filter(Files::isRegularFile).forEach(file -> {
                String name = file.getFileName().toString();
                int dot = name.lastIndexOf('.');
                String extension = dot > 0 && dot < name.length() - 1
                        ? name.substring(dot + 1).toLowerCase()
                        : "[no extension]";

                try {
                    sizes.merge(extension, Files.size(file), Long::sum);
                } catch (IOException e) {
                    System.err.println("Unable to read: " + file);
                }
            });
        }

        return sizes;
    }

    public static void main(String[] args) {
        Path directory = args.length > 0
                ? Paths.get(args[0])
                : Paths.get(".");

        try {
            Map<String, Long> sizes = getFileSizesByExtension(directory);

            System.out.println("File sizes under: " + directory.toAbsolutePath());
            sizes.forEach((extension, size) ->
                    System.out.printf("%s: %d bytes%n", extension, size));
        } catch (IOException e) {
            System.err.println("Error walking directory: " + e.getMessage());
        }
    }
}
