import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public final class LogTailer {
    private final Path file;
    private final long pollIntervalMillis;

    public LogTailer(Path file, long pollIntervalMillis) {
        this.file = file;
        this.pollIntervalMillis = pollIntervalMillis;
    }

    public void follow() throws IOException, InterruptedException {
        long position = 0;
        long fileKey = getFileKey();

        try (RandomAccessFile reader = new RandomAccessFile(file.toFile(), "r")) {
            while (!Thread.currentThread().isInterrupted()) {
                long length = reader.length();
                long currentFileKey = getFileKey();

                if (currentFileKey != fileKey || length < position) {
                    position = 0;
                    fileKey = currentFileKey;
                    reader.seek(0);
                }

                if (length > position) {
                    reader.seek(position);

                    while (true) {
                        long lineStart = reader.getFilePointer();
                        String line = reader.readLine();

                        if (line == null) {
                            reader.seek(lineStart);
                            break;
                        }

                        if (reader.getFilePointer() > length && !line.endsWith("\n")) {
                            reader.seek(lineStart);
                            break;
                        }

                        System.out.println(
                            new String(line.getBytes(StandardCharsets.ISO_8859_1),
                                       StandardCharsets.UTF_8)
                        );
                        position = reader.getFilePointer();
                    }
                }

                Thread.sleep(pollIntervalMillis);
            }
        }
    }

    private long getFileKey() throws IOException {
        try {
            Object key = Files.getAttribute(file, "basic:fileKey");
            return key == null ? 0 : key.hashCode();
        } catch (UnsupportedOperationException e) {
            return 0;
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1 || args.length > 2) {
            System.err.println("Usage: java LogTailer <file> [pollIntervalMillis]");
            System.exit(1);
        }

        Path file = Paths.get(args[0]);
        long interval = args.length == 2 ? Long.parseLong(args[1]) : 500L;

        new LogTailer(file, interval).follow();
    }
}
