$files = (git ls-files --exclude-standard);
foreach ($file in $files) { if ((get-item $file).Extension -in ".cpp", ".hpp", ".c", ".cc", ".cxx", ".hxx", ".ixx") { clang-format -i -style=file $file } }