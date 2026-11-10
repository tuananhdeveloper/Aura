@@ANDROID@@

val nativeDir = file("src/main/cpp")
val isWindows = System.getProperty("os.name").lowercase().contains("windows")
val generatorFlag = if (isWindows) "-G Ninja" else ""

fun bash(script: String): List<String> {
    if (isWindows) {
        val windowsCmd = script
            .lines()
            .map { it.trim() }
            .filter { it.isNotEmpty() && it != "set -e" }
            .joinToString(" && ")
        return listOf("cmd", "/c", windowsCmd)
    }
    return listOf("bash", "-lc", script)
}

tasks.register<Delete>("nativeClean") {
    group = "build"
    delete(nativeDir.resolve("build"))
}

@@DESKTOP@@

@@IOS@@

@@WEB@@