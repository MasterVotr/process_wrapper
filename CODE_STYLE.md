# Coding style
- use .clang-format for formating
- use .clang-tidy for static analysis

## Naming convention:
- Directory name      `snake_case`
- Namespace name      `snake_case`
- Source file         `<namespace>_<class>.cpp`
- Header file         `<namespace>_<class>.hpp`
- Test file           `<namespace>_<class>.t.cpp`
- Class               `PascalCase`
- Struct              `PascalCase`
- Enum                `PascalCase`
- Enum value          `Prefix + PascalCase`
- Function            `camelCase`
- Method              `camelCase`
- Local variable      `camelCase`
- Parameter           `camelCase`
- Public member       `camelCase`
- Protected member    `camelCase`
- Private member      `camelCase_`
- Constant            `kPascalCase`
- Macro               `SCREAMING_SNAKE_CASE`

## Formating convention:
- Indentation         4 spaces
- Tabs                Never
- Braces              BSD/Allman
- Line limit          100 columns
- Function limit      70 lines
- C++ standard        C++17
