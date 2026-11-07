@@ANDROID@@

val nativeDir = file("src/main/cpp")

fun bash(script: String): List<String> {
    return listOf("bash", "-lc", script)
}

@@DESKTOP@@

@@IOS@@

@@WEB@@

tasks.register<Delete>("nativeClean") {
    group = "build"
    delete(nativeDir.resolve("build"))
}
