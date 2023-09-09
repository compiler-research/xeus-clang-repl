#!/bin/bash

if [ "$#" -eq 0 ]; then
    user="root"
else
    user="$1"
fi

docker exec -u "$user" -t -i xeus-clang-repl-c /bin/bash --login
