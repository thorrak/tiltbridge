#!/usr/bin/env bash

declare CWD GITNAME GITROOT ENVIRONMENTS

get_git() {
    echo -e "\nDetermining git root."
    GITROOT=$(git rev-parse --show-toplevel) || (echo -e "Environment not found." && exit)
    GITNAME=$(git rev-parse --show-toplevel); GITNAME=${GITNAME##*/}
}

check_root() {
    CWD=$(pwd)
    cd "$GITROOT" || (echo -e "Environment not found." && exit)
}

get_envs() {
    echo -e "\nGathering build environments for $GITNAME."
    cd "$GITROOT" || exit
    readarray -t ENVIRONMENTS < <(pio project data | grep "env_name" | cut -d'"' -f2)
}

list_envs() {
    echo -e "\nProcessing the following environments for $GITNAME:"
    for env in "${ENVIRONMENTS[@]}"
    do
        echo -e "\t$env"
    done
    sleep 3
}

build_binaries() {
    cd "$GITROOT" || (echo -e "Environment not found." && exit)
    for env in "${ENVIRONMENTS[@]}"
    do
        echo -e "\nBuilding binaries for $env."
        sleep 3
        pio run -e "$env"
        echo -e "\nBuilding filesysyem for $env."
        sleep 3
        pio run --target buildfs -e "$env"
    done
}

copy_binaries() {
    echo
    for env in "${ENVIRONMENTS[@]}"
    do
        echo -e "Copying binaries for $env."
        cp "$GITROOT"/.pio/build/"$env"/firmware.bin "$GITROOT"/bin/"$env"_firmware.bin
        cp "$GITROOT"/.pio/build/"$env"/partitions.bin "$GITROOT"/bin/"$env"_partitions.bin
        cp "$GITROOT"/.pio/build/"$env"/spiffs.bin "$GITROOT"/bin/"$env"_spiffs.bin
    done
}

main() {
    get_git "$@"
    check_root "$@"
    get_envs "$@"
    list_envs "$@"
    build_binaries "$@"
    copy_binaries "$@"
    cd "$CWD" || exit
    echo -e "\nBuild and prep for $GITNAME complete."
}

main "$@" && exit 0
