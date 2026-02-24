if ! command -v marblec &> /dev/null; then
    echo "Error: marblec not found in PATH"
    exit 1
fi

TARGET_DIR="${1:-.}"

if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' does not exist"
    exit 1
fi

echo "Processing .mr files in directory: $TARGET_DIR"

total=0
success=0
failed=0

shopt -s nullglob
for file in "$TARGET_DIR"/*.mr; do
    ((total++))
    filename="${file%.mr}"
    
    echo "=================================="
    echo "File: $file"
    echo "=================================="
    
    echo "Compiling..."
    if marblec "$file"; then
        echo "TEST $file"
        
        if [ -f "./$filename" ]; then
            "./$filename"
            status=$?
            
            if [ $status -eq 0 ]; then
                echo "TEST $file WAS SUCCESSFUL"
                ((success++))
            else
                echo "Program exited with code $status"
                ((failed++))
            fi
            
            rm "./$filename"
            echo "Removed $filename"
        else
            echo "Error: executable file $filename not found"
            ((failed++))
        fi
    else
        echo "Compilation error for $file"
        ((failed++))
    fi
    
    echo ""
done

echo "=================================="
echo "Statistics:"
echo "Total files: $total"
echo "Successful: $success"
echo "Failed: $failed"
echo "=================================="

if [ $failed -eq 0 ]; then
    echo "All tests passed successfully!"
    exit 0
else
    echo "Errors detected"
    exit 1
fi
