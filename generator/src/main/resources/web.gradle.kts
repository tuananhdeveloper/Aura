// Override these with -PemsdkDir=/path/to/emsdk and -PemsdkPython=/path/to/python3.
val emsdkDir = file(providers.gradleProperty("emsdkDir").orElse("src/main/cpp/emsdk").get())
val emsdkPython = file(providers.gradleProperty("emsdkPython").orElse("@@PYTHON@@").get())
val webBuildDir = nativeDir.resolve("build/web")
tasks.register<Exec>("htmlBuild") {
    group = "build"
    workingDir = nativeDir
    doFirst {
        if (!emsdkDir.isDirectory) throw GradleException("Emscripten SDK directory not found: $emsdkDir")
        if (!emsdkPython.isFile) throw GradleException("Python executable not found: $emsdkPython")
        val python = try {
            ProcessBuilder(emsdkPython.absolutePath, "-c",
                "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)")
                .inheritIO().start().waitFor()
        } catch (e: java.io.IOException) {
            throw GradleException("Cannot run Python: $emsdkPython", e)
        }
        if (python != 0) throw GradleException("Python 3.10 or newer is required: $emsdkPython")
        val windows = System.getProperty("os.name").startsWith("Windows")
        val env = emsdkDir.resolve(if (windows) "emsdk_env.bat" else "emsdk_env.sh")
        if (!env.isFile) throw GradleException("Emscripten environment script not found: $env")
        environment("EMSDK_PYTHON", emsdkPython.absolutePath)
        if (windows) {
            commandLine("cmd", "/d", "/c",
                "call \"${env.absolutePath}\" && emcmake cmake -S . -B \"${webBuildDir.absolutePath}\" && cmake --build \"${webBuildDir.absolutePath}\"")
        } else {
            commandLine("bash", "-c",
                "set -e; source \"\$1\"; emcmake cmake -S . -B \"\$2\"; cmake --build \"\$2\"",
                "web-build", env.absolutePath, webBuildDir.absolutePath)
        }
    }
}
