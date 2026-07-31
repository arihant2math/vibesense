import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.util.stream.Collectors

fun reportFileSizesByExtension(inputs: List<Path>): Map<String, Long> {
    if (inputs.isEmpty()) return emptyMap()

    val sizesByExtension = mutableMapOf<String, Long>()

    for (input in inputs) {
        if (!Files.exists(input) || Files.isSymbolicLink(input)) continue

        if (Files.isRegularFile(input)) {
            addFileSize(input, sizesByExtension)
        } else if (Files.isDirectory(input)) {
            Files.walk(input).use { paths ->
                paths.filter { path ->
                    Files.isRegularFile(path) && !Files.isSymbolicLink(path)
                }.forEach { path ->
                    addFileSize(path, sizesByExtension)
                }
            }
        }
    }

    return sizesByExtension.toSortedMap()
}

private fun addFileSize(
    file: Path,
    sizesByExtension: MutableMap<String, Long>
) {
    val fileName = file.fileName?.toString() ?: return
    val dotIndex = fileName.lastIndexOf('.')

    val extension = when {
        dotIndex <= 0 || dotIndex == fileName.lastIndex -> "[no extension]"
        else -> fileName.substring(dotIndex + 1).lowercase()
    }

    val size = runCatching { Files.size(file) }.getOrDefault(0L)
    sizesByExtension[extension] =
        Math.addExact(sizesByExtension.getOrDefault(extension, 0L), size)
}

fun main(args: Array<String>) {
    val paths = args.map(Paths::get)
    reportFileSizesByExtension(paths).forEach { (extension, size) ->
        println("$extension: $size bytes")
    }
}
