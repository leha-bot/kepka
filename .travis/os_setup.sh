#!/bin/bash

set -x

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
    docker pull berkus/docker-cpp-ci:latest
fi

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
    brew update
    brew install cmake qt ffmpeg opus openal-soft
    brew outdated cmake && brew upgrade cmake
    brew outdated qt && brew upgrade qt
    brew outdated ffmpeg && brew upgrade ffmpeg
    brew outdated opus && brew upgrade opus
    brew outdated openal-soft && brew upgrade openal-soft
fi
