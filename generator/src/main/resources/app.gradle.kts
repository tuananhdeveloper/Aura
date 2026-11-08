@@ANDROID@@

val nativeDir = file("src/main/cpp")

fun bash(script: String): List<String> {
    return listOf("bash", "-lc", script)
}

tasks.register<Delete>("nativeClean") {
    group = "build"
    delete(nativeDir.resolve("build"))
}

@@DESKTOP@@

@@IOS@@

@@WEB@@