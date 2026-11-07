import java.util.Properties

@@ANDROID@@

val nativeDir = file("src/main/cpp")

@@DESKTOP@@

@@IOS@@

@@WEB@@

tasks.register<Delete>("nativeClean") {
    group = "build"
    delete(nativeDir.resolve("build"))
}
