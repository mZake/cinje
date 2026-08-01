#!/bin/bash

deactivate() {
    if [[ -n "${_OLD_PATH}" ]]; then
        export PATH="${_OLD_PATH}"
        unset _OLD_PATH
    fi

    if [[ -n "${_OLD_PS1}" ]]; then
        export PS1="${_OLD_PS1}"
        unset _OLD_PS1
    fi

    if ! [[ "${1}" = "nondestructive" ]]; then
        unset -f deactivate
    fi
}

deactivate nondestructive

_OLD_PATH="${PATH}"
export PATH="${DEVKITARM}/bin:${PATH}"

_OLD_PS1="${PS1}"
export PS1="(cinje) ${PS1}"
