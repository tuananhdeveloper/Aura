val desktopBuildDir = nativeDir.resolve("build/desktop")

tasks.register<Exec>("desktopBuild") {
    group = "application"
    description = "Configure and build the native desktop game"
    workingDir = nativeDir

    commandLine = listOf("bash", "-c",
        """
        set -e
        cmake -S . -B "${desktopBuildDir}"
        cmake --build "${desktopBuildDir}"
        """.trimIndent()
    )
}

tasks.register<Exec>("run") {
    group = "application"
    description = "Build and run the desktop game"

    dependsOn("desktopBuild")

    workingDir = desktopBuildDir

    commandLine = listOf("bash", "-c", "./" + rootProject.name)
}

val windowsBuildDir = nativeDir.resolve("build/windows")
tasks.register<Exec>("windowsBuild") {
    group = "application"
    description = "Cross-compile the native game for Windows from Linux or macOS"
    workingDir = nativeDir
    val toolchain = nativeDir.resolve("windows-toolchain.cmake")
    commandLine = listOf("bash", "-c",
        "set -e; cmake -S . -B \"${windowsBuildDir.absolutePath}\" -DCMAKE_TOOLCHAIN_FILE=\"${toolchain.absolutePath}\"; cmake --build \"${windowsBuildDir.absolutePath}\"")
}
