val emsdkDir = file(providers.gradleProperty("emsdkDir").orElse("@@EMSDK@@").get())
val emsdkPython = file(providers.gradleProperty("emsdkPython").orElse("@@PYTHON@@").get())
val emsdkEnv = emsdkDir.resolve(if (System.getProperty("os.name").startsWith("Windows"))
    "emsdk_env.bat" else "emsdk_env.sh")
val webBuildDir = nativeDir.resolve("build/web")
tasks.register<Exec>("htmlBuild") {
    group = "build"
    description = "Configure and build the game for the web with Emscripten"

    workingDir = nativeDir

    doFirst {
        if (!emsdkDir.isDirectory) throw GradleException("Emscripten SDK directory not found: $emsdkDir")
        if (!emsdkEnv.isFile) throw GradleException("Emscripten environment script not found: $emsdkEnv")
        if (!emsdkPython.isFile) throw GradleException("Python executable not found: $emsdkPython")
        val pythonVersion = ProcessBuilder(emsdkPython.absolutePath, "-c",
            "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)")
            .inheritIO().start().waitFor()
        if (pythonVersion != 0) throw GradleException("Python 3.10 or newer is required: $emsdkPython")
        if (System.getProperty("os.name").startsWith("Windows")) {
            environment("EMSDK_PYTHON", emsdkPython.absolutePath)
            commandLine("cmd", "/d", "/c",
                "call \"${emsdkEnv.absolutePath}\" && emcmake cmake -S . -B \"${webBuildDir.absolutePath}\" && cmake --build \"${webBuildDir.absolutePath}\"")
        } else {
            commandLine("bash", "-c",
                "set -e; export EMSDK_PYTHON=\"\$1\"; source \"\$2\"; emcmake cmake -S . -B \"\$3\"; cmake --build \"\$3\"",
                "htmlBuild", emsdkPython.absolutePath, emsdkEnv.absolutePath, webBuildDir.absolutePath)
        }
    }
}