import os
import re
import sys

def extract_paths_from_cmake(cmake_file):
    """Extract file paths from CMakeLists.txt"""
    with open(cmake_file, 'r') as f:
        content = f.read()

    # Extract all source file paths using regex
    # Match any .cpp files defined in set() commands
    source_sets = re.findall(r'set\(\w+_SOURCES(.*?)\)', content, re.DOTALL)

    paths = []
    for source_set in source_sets:
        # Extract file paths from each source set
        file_paths = re.findall(r'src/[\w/]+\.cpp', source_set)
        paths.extend(file_paths)

    # Add main source files (not in sets)
    main_files = re.findall(r'add_executable\(\w+\s+(src/[\w/]+\.cpp)', content)
    paths.extend(main_files)

    # Find include directories
    include_dirs = re.findall(r'include_directories\((.*?)\)', content, re.DOTALL)
    include_paths = []

    for dir_block in include_dirs:
        # Extract directory paths (handling variables like ${PROJECT_SOURCE_DIR})
        project_dir = os.path.dirname(os.path.abspath(cmake_file))
        dir_block = dir_block.replace('${PROJECT_SOURCE_DIR}', project_dir)
        paths_in_block = re.findall(r'\s*([^\s]+)', dir_block)
        include_paths.extend(paths_in_block)

    return paths, include_paths

def find_header_files(include_dirs):
    """Find all header files in the include directories"""
    header_files = []

    for include_dir in include_dirs:
        # Skip if it contains variables we can't resolve
        if '${' in include_dir:
            continue

        include_dir = include_dir.strip()
        if not os.path.exists(include_dir):
            continue

        for root, _, files in os.walk(include_dir):
            for file in files:
                if file.endswith(('.h', '.hpp')):
                    header_files.append(os.path.join(root, file))

    return header_files

def read_file_content(file_path, project_dir):
    """Read and format file content with appropriate header"""
    if not os.path.exists(file_path):
        relative_path = os.path.join(project_dir, file_path)
        if os.path.exists(relative_path):
            file_path = relative_path
        else:
            return f"// File not found: {file_path}\n\n"

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Format with file path header
        formatted = f"//===== FILE: {file_path} =====//\n\n"
        formatted += content
        formatted += "\n\n//===== END FILE: {file_path} =====//\n\n"
        return formatted
    except Exception as e:
        return f"// Error reading file {file_path}: {str(e)}\n\n"

def main():
    if len(sys.argv) > 1:
        cmake_file = sys.argv[1]
    else:
        cmake_file = 'CMakeLists.txt'  # Default

    output_file = 'project_code.txt'
    project_dir = os.path.dirname(os.path.abspath(cmake_file))

    # Extract source and include paths
    source_paths, include_dirs = extract_paths_from_cmake(cmake_file)

    # Find header files
    header_files = find_header_files([os.path.join(project_dir, 'include')])

    # Combine all files
    all_files = source_paths + header_files

    print(f"Found {len(source_paths)} source files and {len(header_files)} header files")

    # Read and write file contents
    with open(output_file, 'w', encoding='utf-8') as out:
        out.write(f"// Project Code from {cmake_file}\n")
        out.write(f"// Total files: {len(all_files)}\n\n")

        for file_path in all_files:
            print(f"Processing: {file_path}")
            content = read_file_content(file_path, project_dir)
            out.write(content)

    print(f"\nCode extraction complete. Output saved to {output_file}")

if __name__ == "__main__":
    main()
