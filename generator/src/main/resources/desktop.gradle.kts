val desktopBuildDir = nativeDir.resolve("build/desktop")

val desktopConfigure = tasks.register<Exec>("desktopConfigure") {
    group = "application"
    workingDir = nativeDir
    commandLine("cmake", "-S", ".", "-B", desktopBuildDir.absolutePath)
}
tasks.register<Exec>("desktopBuild") {
    group = "application"
    dependsOn(desktopConfigure)
    workingDir = nativeDir
    commandLine("cmake", "--build", desktopBuildDir.absolutePath)
}
tasks.register<Exec>("run") {
    group = "application"
    dependsOn("desktopBuild")
    workingDir = desktopBuildDir
    doFirst {
        val windows = System.getProperty("os.name").startsWith("Windows")
        val executable = if (windows) {
            val debugExecutable = desktopBuildDir.resolve("Debug/myraylibgame.exe")
            if (debugExecutable.isFile) debugExecutable else desktopBuildDir.resolve("myraylibgame.exe")
        } else desktopBuildDir.resolve("myraylibgame")
        commandLine(executable.absolutePath)
    }
}
