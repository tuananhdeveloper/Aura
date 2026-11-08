val emsdkDir = nativeDir.resolve("emsdk")
val emsdkEnv = emsdkDir.resolve("emsdk_env.sh")
val emsdkPython = file("@@PYTHON@@")
val webBuildDir = nativeDir.resolve("build/web")
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