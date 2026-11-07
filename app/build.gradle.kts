import java.util.Properties
import java.io.FileInputStream

val localProperties = Properties()
val localPropertiesFile = rootProject.file("local.properties")
if (localPropertiesFile.exists()) {
    localProperties.load(FileInputStream(localPropertiesFile))
}
val myUUID: String? = localProperties.getProperty("MY_IOS_UDID")

plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.tuananh.myraylibgame"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "com.tuananh.myraylibgame"
        minSdk = 23
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            optimization {
                enable = true
                packageScope = setOf("androidx.**", "kotlin.**", "kotlinx.**")
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }
    buildFeatures {
        viewBinding = true
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.constraintlayout)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
}

val nativeDir = file("src/main/cpp")
val desktopBuildDir = nativeDir.resolve("build/desktop")
val windowsBuildDir = nativeDir.resolve("build/windows")
val emsdkDir = nativeDir.resolve("emsdk")
val emsdkEnv = emsdkDir.resolve("emsdk_env.sh")
val emsdkPython = file("/opt/homebrew/bin/python3")
val webBuildDir = nativeDir.resolve("build/web")
val iosBuildDir = nativeDir.resolve("build/ios")
val iosSimBuildDir = nativeDir.resolve("build/ios-sim")

fun bash(script: String): List<String> {
    return listOf("bash", "-lc", script)
}

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

tasks.register<Exec>("windowsBuild") {
    group = "application"
    description = "Cross-compile the native game for Windows from Linux or macOS"

    workingDir = nativeDir
    val toolchain = file(nativeDir.path + "/windows-toolchain.cmake").absolutePath

    commandLine = bash(
        """
        cmake -S . -B """ + windowsBuildDir + """ \
            -DCMAKE_TOOLCHAIN_FILE=${toolchain}
        cmake --build """ + windowsBuildDir + """
        """.trimIndent()
    )
}

tasks.register<Exec>("htmlBuild") {
    group = "build"
    description = "Configure and build the game for the web with Emscripten"

    workingDir = nativeDir

    commandLine = bash(
        """
        export EMSDK_PYTHON="${emsdkPython}"
        source "${emsdkEnv}"
        emcmake cmake -S . -B "${webBuildDir}"
        cmake --build "${webBuildDir}"
        """.trimIndent()
    )
}

tasks.register<Exec>("nativeClean") {
    group = "build"
    description = "Remove native build outputs"

    workingDir = nativeDir

    commandLine = bash(
        """
        rm -rf build
        """.trimIndent()
    )
}

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