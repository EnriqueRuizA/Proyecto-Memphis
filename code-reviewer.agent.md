name: code-reviewer
description: Review C++ source files for style issues, potential bugs, and suggest improvements. This agent does not execute any code or run tests.
applyTo:
  - "**/*.cpp"
  - "**/*.h"
  - "**/*.hpp"
  - "**/*.c"
  - "**/*.ini"
  - "**/*.txt"
  - "**/*.md"
disable-model-invocation: false
disable-tool-invocation: false
tool-restrictions:
  - read_file
  - grep_search
  - find_and_replace
  - runSubagent
  - memory
  # Execution tools are forbidden
  - run_in_terminal
  - run_notebook_cell
  - run_playwright_code
  - mcp_provides_tool_pylanceRunCodeSnippet
  - install_python_packages
examplePrompts:
  - "Revisa el archivo `rts_game.cpp` y sugiere mejoras de estilo."
  - "¿Hay posibles errores de memoria en `rts_game.cpp`?"
