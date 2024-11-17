#!/bin/bash

# Проверяем, что переданы аргументы
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 [-i | -n] <file1.cc> [file2.cc ...]"
    exit 1
fi

# Проверяем первый аргумент (флаг)
flag=$1
if [ "$flag" != "-i" ] && [ "$flag" != "-n" ]; then
    echo "Error: Invalid flag. Use -i for in-place editing or -n for dry run."
    exit 1
fi

# Удаляем флаг из списка аргументов
shift

# Обрабатываем файлы
for file in "$@"; do
    if [[ "$file" == *.cc ]] || [[ "$file" == *.h ]]; then
        echo "Formatting $file with clang-format..."
        clang-format --style="{BasedOnStyle: Google, IndentWidth: 4}" "$flag" "$file"
    else
        echo "Skipping $file: Not a .cc file."
    fi
done

echo "Formatting completed."

