# Language Rules: C / C++

## Code Style
- **Brace Style:** Allman Style (Braces always on a new line).
- **Switch Statements:** Align `case` labels vertically with the `switch` keyword.
- **Include Guards:** Use `#pragma once`.

## Naming Conventions
| Entity | Case / Style | Example |
| :--- | :--- | :--- |
| **Files** | snake_case (lowercase) | `ecc_curve_defs.h` |
| **Private Members** | camelCase + trailing `_` | `value_` |
| **Macros** | UPPER_SNAKE_CASE | `CACHE_SIZE_DEFAULT` |
| **Variables** | snake_case | `item_width` |
| **Functions** | snake_case | `string_size()` |
| **Methods** | snake_case | `reset_geometry()` |
| **Constants** | PascalCase | `Fps` |
| **Structs** | PascalCase | `MovingCorrelation` |
| **Enums** | UPPER_SNAKE_CASE (Elements) | `BMP`, `JPEG` |
| **Classes** | PascalCase | `URLSecurityManager` |
| **Namespaces** | lowercase (start w/ project) | `projectplot` |

## Snippet: Switch Alignment
```c
switch (suffix)
{
case 'G': // Aligned with switch
    mem <<= 30;
    break;
default:
    break;
}
```
