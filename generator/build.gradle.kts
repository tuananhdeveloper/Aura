plugins {
    application
}

application {
    mainClass = "starter.StarterGenerator"
}

java {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
}

val templateZip = tasks.register<Zip>("templateZip") {
    archiveFileName = "starter-template.zip"
    destinationDirectory = layout.buildDirectory.dir("template")
    from(rootDir.parentFile) {
        include("LICENSE", "app/src/main/**", "gradle/**", "gradlew", "gradlew.bat",
                "gradle.properties", "build.gradle.kts", "settings.gradle.kts")
        exclude("gradle/gradle-daemon-jvm.properties", "app/src/main/cpp/emsdk/**", "app/src/main/cpp/build/**",
                "app/src/main/cpp/tests/**")
    }
}

tasks.processResources {
    from(templateZip)
}

tasks.jar {
    manifest {
        attributes["Main-Class"] = application.mainClass.get()
    }
}

tasks.register<JavaExec>("generatorTest") {
    dependsOn(tasks.testClasses)
    classpath = sourceSets.test.get().runtimeClasspath
    mainClass = "starter.GeneratorTest"
}

tasks.check {
    dependsOn("generatorTest")
}

tasks.test {
    failOnNoDiscoveredTests = false
}
