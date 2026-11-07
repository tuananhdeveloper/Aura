val iosBuildDir = nativeDir.resolve("build/ios")
val iosConfigure = tasks.register<Exec>("iosConfigure") {
    group = "build"
    workingDir = nativeDir
    commandLine("cmake", "-S", ".", "-B", iosBuildDir.absolutePath, "-G", "Xcode",
        "-DCMAKE_SYSTEM_NAME=iOS", "-DCMAKE_OSX_ARCHITECTURES=arm64",
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=18.0",
        "-DCMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER=@@PACKAGE@@")
    providers.gradleProperty("iosTeam").orNull?.let {
        args("-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=$it")
    }
}
tasks.register<Exec>("iosBuild") {
    group = "build"
    dependsOn(iosConfigure)
    workingDir = nativeDir
    commandLine("cmake", "--build", iosBuildDir.absolutePath, "--config", "Release")
}
tasks.register<Exec>("iosInstall") {
    group = "application"
    dependsOn("iosBuild")
    workingDir = nativeDir
    doFirst {
        val device = providers.gradleProperty("iosDevice").orNull
            ?: throw GradleException("Set -PiosDevice to the UDID of a connected iOS device")
        commandLine("xcrun", "devicectl", "device", "install", "app", "--device", device,
            iosBuildDir.resolve("Release-iphoneos/myraylibgame.app").absolutePath)
    }
}
tasks.register<Exec>("iosRun") {
    group = "application"
    dependsOn("iosInstall")
    workingDir = nativeDir
    doFirst {
        val device = providers.gradleProperty("iosDevice").orNull
            ?: throw GradleException("Set -PiosDevice to the UDID of a connected iOS device")
        commandLine("xcrun", "devicectl", "device", "process", "launch", "--device", device, "@@PACKAGE@@")
    }
}
