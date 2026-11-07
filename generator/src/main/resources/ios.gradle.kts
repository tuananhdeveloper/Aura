val iosBuildDir = nativeDir.resolve("build/ios")

tasks.register<Exec>("iosBuild") {
    group = "build"
    description = "Generate and build iOS app with CMake + Xcode"
    workingDir = nativeDir
    commandLine = bash(
        """
                cmake -S . \
                      -B """ + iosBuildDir + """ \
                      -G Xcode \
                      -DCMAKE_SYSTEM_NAME=iOS \
                      -DCMAKE_OSX_ARCHITECTURES=arm64 \
                      -DCMAKE_OSX_DEPLOYMENT_TARGET=18.0
                """.trimIndent()
    )
}

tasks.register<Exec>("iosRun") {
    group = "iosRun"
    description = "Run the iOS app"

    dependsOn("iosBuild")

    workingDir = iosBuildDir

    commandLine(
        "xcodebuild",
        "-project", "${iosBuildDir}/${rootProject.name.lowercase()}.xcodeproj",
        "-scheme", rootProject.name.lowercase(),
        "-destination", "platform=iOS,id=${myUUID}",
        "-configuration", "Release",
        "CODE_SIGNING_ALLOWED=NO",
        "-allowProvisioningUpdates"
    )
}