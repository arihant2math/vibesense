import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;

public final class AtomicFileWriteDemo {
    private AtomicFileWriteDemo() {
    }

    public static void writeAtomically(Path target, String content) throws IOException {
        Path absoluteTarget = target.toAbsolutePath();
        Path parent = absoluteTarget.getParent();

        if (parent == null) {
            throw new IOException("Target must have a parent directory");
        }

        Files.createDirectories(parent);
        Path temporary = Files.createTempFile(
                parent,
                absoluteTarget.getFileName().toString(),
                ".tmp"
        );

        try {
            Files.write(
                    temporary,
                    content.getBytes(StandardCharsets.UTF_8),
                    StandardOpenOption.WRITE,
                    StandardOpenOption.TRUNCATE_EXISTING
            );

            try {
                Files.move(
                        temporary,
                        absoluteTarget,
                        StandardCopyOption.ATOMIC_MOVE,
                        StandardCopyOption.REPLACE_EXISTING
                );
            } catch (java.nio.file.AtomicMoveNotSupportedException e) {
                throw new IOException("Atomic file moves are not supported", e);
            }
        } finally {
            Files.deleteIfExists(temporary);
        }
    }

    public static void main(String[] args) throws IOException {
        Path target = args.length > 0
                ? Path.of(args[0])
                : Path.of("demo-output.txt");

        writeAtomically(target, "This file was written atomically.\n");
        System.out.println("Wrote: " + target.toAbsolutePath());
    }
}
