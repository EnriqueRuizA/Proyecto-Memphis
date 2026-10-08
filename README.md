# Medieval Conquest — RTS

Un juego de estrategia en tiempo real en C++17 usando Raylib.

## Requisitos

- Windows
- Compilador C++17: **w64devkit (MinGW-w64, GCC 14.2)** en `C:\raylib\w64devkit` *(verificado)*
- **Raylib 5.5** (headers y `libraylib.a` vienen con el toolchain)

## Compilación (modular — `src/`)

```bat
rem Un solo comando desde la raíz del repositorio:
build.bat

rem Compila src\*.cpp y genera rts_game.exe en la raíz.
rem Log completo (errores/warnings): build\build.log
```

La línea manual equivalente (toolchain w64devkit en PATH):

```bat
g++ -std=c++17 -O1 -Wall -Wextra -Isrc src\*.cpp -o rts_game.exe -L"C:\raylib\raylib\src" -lraylib -lopengl32 -lgdi32 -lwinmm 2> build\build.log
```

## Ejecución

```bat
rem Desde la raíz del repositorio (assets/ y settings.ini se buscan en el CWD):
rts_game.exe
```

Si `assets/sprites/` no existe, el juego arranca igualmente con los sprites
procedurales originales (fallback automático).

## Configuración

- Copiar `settings.example.ini` a `settings.ini` (local, ignorado por Git).
- Resolución, volumen, dificultad e idioma se ajustan desde el menú o el `.ini`.
- Default único de resolución: **1280×768**; sin `.ini` el juego arranca con los defaults.

## Estructura

- `src/` — 12 módulos `.cpp` + 11 headers `.h` (reestructuración FASE 1 completada).
- `assets/sprites/` — hojas de unidades Kenney (CC0) con fallback procedural.
- `tools/preview_sprites.cpp` — genera `build/sprite_preview.png` para verificar
  la integración de sprites sin arrancar el juego.
- `PLAN.md` — plan de reestructuración y estado de ejecución.
- `settings.example.ini` — plantilla versionada de la configuración.

## Créditos de arte

- Unidades: **"Medieval RTS" por Kenney (kenney.nl)** — licencia
  [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) (dominio público;
  la atribución es opcional). Archivo de licencia incluido en
  `assets/sprites/LICENSE-Kenney.txt`. Fuente:
  <https://opengameart.org/content/medieval-rts-120>
