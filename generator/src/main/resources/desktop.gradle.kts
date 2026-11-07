val desktopBuildDir = nativeDir.resolve("build/desktop")

tasks.register<Exec>("desktopBuild") {
    group = "application"
    description = "Configure and build the native desktop game"
    workingDir = nativeDir

    commandLine = bash(
        """
        cmake -S . -B """ + desktopBuildDir + """
        cmake --build """ + desktopBuildDir + """
        """.trimIndent()
    )
}

tasks.register<Exec>("run") {
    group = "application"
    description = "Build and run the desktop game"

    dependsOn("desktopBuild")

    workingDir = desktopBuildDir

    commandLine = bash("./" + rootProject.name.lowercase())
}

