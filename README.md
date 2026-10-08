# Medieval Conquest — RTS

Un juego de estrategia en tiempo real en C++17 usando Raylib.

## Requisitos

- Windows
- Compilador C++17: **w64devkit (MinGW-w64, GCC 14.2)** en `C:\raylib\w64devkit` *(verificado)*
- **Raylib 5.5** (headers y `libraylib.a` vienen con el toolchain)

## Compilación (modular — `src/`)

```bat
rem 1) Toolchain en PATH
set PATH=C:\raylib\w64devkit\bin;%PATH%

rem 2) Compilar los 12 módulos
mkdir build
g++ -std=c++17 -Isrc -O2 -c src\config.cpp   -o build\config.o
g++ -std=c++17 -Isrc -O2 -c src\util.cpp     -o build\util.o
g++ -std=c++17 -Isrc -O2 -c src\ui.cpp       -o build\ui.o
g++ -std=c++17 -Isrc -O2 -c src\camera.cpp   -o build\camera.o
g++ -std=c++17 -Isrc -O2 -c src\sprite.cpp   -o build\sprite.o
g++ -std=c++17 -Isrc -O2 -c src\terrain.cpp  -o build\terrain.o
g++ -std=c++17 -Isrc -O2 -c src\elements.cpp -o build\elements.o
g++ -std=c++17 -Isrc -O2 -c src\combat.cpp   -o build\combat.o
g++ -std=c++17 -Isrc -O2 -c src\campaign.cpp -o build\campaign.o
g++ -std=c++17 -Isrc -O2 -c src\city.cpp     -o build\city.o
g++ -std=c++17 -Isrc -O2 -c src\save.cpp     -o build\save.o
g++ -std=c++17 -Isrc -O2 -c src\main.cpp     -o build\main.o

rem 3) Enlazar (cmd/PowerShell no expande *.o: listarlos)
g++ -o rts_game.exe build\config.o build\util.o build\ui.o build\camera.o build\sprite.o build\terrain.o build\elements.o build\combat.o build\campaign.o build\city.o build\save.o build\main.o -lraylib -lopengl32 -lgdi32 -lwinmm
```

## Ejecución

```bat
rem Desde la raíz del repositorio (assets/ y settings.ini se buscan en el CWD):
build\rts_game_mod.exe
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
