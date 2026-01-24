#!/bin/bash
# Downloads the Gradle wrapper JAR if it doesn't exist

WRAPPER_DIR="gradle/wrapper"
WRAPPER_JAR="$WRAPPER_DIR/gradle-wrapper.jar"
GRADLE_VERSION="8.4"

if [ ! -f "$WRAPPER_JAR" ]; then
    echo "Downloading Gradle wrapper JAR..."
    mkdir -p "$WRAPPER_DIR"

    # Download from Gradle's GitHub releases
    WRAPPER_URL="https://github.com/gradle/gradle/raw/v${GRADLE_VERSION}/gradle/wrapper/gradle-wrapper.jar"

    if command -v curl >/dev/null 2>&1; then
        curl -L -o "$WRAPPER_JAR" "$WRAPPER_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$WRAPPER_JAR" "$WRAPPER_URL"
    else
        echo "ERROR: Neither curl nor wget is available to download gradle-wrapper.jar"
        exit 1
    fi

    if [ -f "$WRAPPER_JAR" ]; then
        echo "Gradle wrapper JAR downloaded successfully"
    else
        echo "ERROR: Failed to download gradle-wrapper.jar"
        exit 1
    fi
else
    echo "Gradle wrapper JAR already exists"
fi
