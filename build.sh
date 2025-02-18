#!/bin/bash

echo "Compiling junk_code_inserter..."
gcc -o junk_code_inserter src/junk_code_inserter.c -Iinclude -lcrypto || exit 1

echo "Applying junk code to modified source files..."
for file in $(git status --porcelain | awk '{print $2}' | grep '^src/.*\.c$' | grep -v 'junk_code_inserter.c'); do
    if [[ "$file" != "src/sqlite3.c" && "$file" != "src/aes.c" ]]; then
        echo "Processing $file"
        ./junk_code_inserter "$file"
    else
        echo "Skipping $file"
    fi
done
