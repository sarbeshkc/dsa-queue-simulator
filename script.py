import os
from pathlib import Path

def collect_cpp_files(root_dir: str, output_file: str):
    """
    Recursively collects contents of .cpp and .h files from src and include directories
    and writes them to a single output file.

    Args:
        root_dir (str): Root directory containing src and include folders
        output_file (str): Path to the output text file
    """
    # Convert root_dir to Path object for easier manipulation
    root_path = Path(root_dir)

    # List to store all found files
    cpp_files = []

    # Search in src and include directories
    for directory in ['src', 'include']:
        dir_path = root_path / directory
        if not dir_path.exists():
            print(f"Warning: {directory} directory not found in {root_dir}")
            continue

        # Recursively find all .cpp and .h files
        for ext in ['*.cpp', '*.h']:
            cpp_files.extend(dir_path.rglob(ext))

    # Sort files for consistent output
    cpp_files.sort()

    # Write contents to output file
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for file_path in cpp_files:
            try:
                # Write file header
                outfile.write(f"\n{'='*80}\n")
                outfile.write(f"File: {file_path.relative_to(root_path)}\n")
                outfile.write(f"{'='*80}\n\n")

                # Write file contents
                with open(file_path, 'r', encoding='utf-8') as infile:
                    outfile.write(infile.read())

                outfile.write('\n')  # Add extra newline between files

            except Exception as e:
                print(f"Error processing {file_path}: {str(e)}")

def main():
    # You can modify these paths as needed
    root_directory = "."  # Current directory
    output_file = "cpp_contents.txt"

    collect_cpp_files(root_directory, output_file)
    print(f"Collection complete. Output written to {output_file}")

if __name__ == "__main__":
    main()
