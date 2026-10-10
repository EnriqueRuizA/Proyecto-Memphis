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
g++ -std=c++17 -O2 -Wall -Wextra -Isrc src\*.cpp -o rts_game.exe -L"C:\raylib\raylib\src" -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 2> build\build.log
```

## Ejecución

```bat
rem Desde la raíz del repositorio (assets/ y settings.ini se buscan en el CWD):
rts_game.exe

rem Self-test de red (headless, sin ventana; sale con código 0=PASS / 1=FAIL):
rts_game.exe -nettest
```

Si `assets/sprites/` no existe, el juego arranca igualmente con los sprites
procedurales originales (fallback automático).

## Configuración

- Copiar `settings.example.ini` a `settings.ini` (local, ignorado por Git).
- Resolución, volumen, dificultad e idioma se ajustan desde el menú o el `.ini`.
- Default único de resolución: **1280×768**; sin `.ini` el juego arranca con los defaults.

## Estructura

- `src/` — 18 módulos `.cpp` + 17 headers `.h` (reestructuración FASE 1 + fases
  de contenido A–K; red desde la Fase E: `net.h`/`net.cpp`).
- `assets/sprites/` — hojas de unidades Kenney (CC0) con fallback procedural.
- `tools/preview_sprites.cpp` — genera `build/sprite_preview.png` para verificar
  la integración de sprites sin arrancar el juego.
- `PLAN.md` — plan de reestructuración, histórico de fases (§12) y estado.
- `settings.example.ini` — plantilla versionada de la configuración.

## Red (Fase E+, work in progress)

- `src/net.h` / `src/net.cpp` — base UDP no bloqueante (Winsock2), loopback
  `127.0.0.1:7777` (evita avisos de firewall; `INADDR_ANY` llegará con el
  protocolo de juego).
- `rts_game.exe -nettest` — self-test headless: ping/pong crudo (10 rondas),
  sesión loopback (JOIN→WELCOME, PING/PONG, BYE) y runtime de lobby
  (host registra JOIN; cliente sin host no conecta). Sale con 0=PASS / 1=FAIL.
- Botón **MULTIPLAYER** en el menú → lobby: HOST (escucha UDP 7777), JOIN por
  IP, estado de conexión. Sin sincronización de juego todavía.
- Roadmap: F protocolo de sesión ✅ → G lobby ✅ → H sincronización de campaña
  (host autoritativo). Detalle en `PLAN.md` §12.

## Tests

Los tests viven en `build/` (ignorado en Git; son utilidades locales, ASCII
puro para PowerShell 5.1). Requieren `uiScale 1.000` en `settings.ini` y
1600×900. Ejecutar desde la raíz, un test por invocación:

```powershell
powershell -ExecutionPolicy Bypass -File build\test_net.ps1     # red (-nettest)
powershell -ExecutionPolicy Bypass -File build\test_g_lobby.ps1 # lobby multiplayer (UI)
powershell -ExecutionPolicy Bypass -File build\test_faseK.ps1   # campaña Great Continent
powershell -ExecutionPolicy Bypass -File build\test_faseI.ps1   # diplomacia
powershell -ExecutionPolicy Bypass -File build\test_faseJ.ps1   # ejércitos de campo
powershell -ExecutionPolicy Bypass -File build\test_faseD.ps1   # auto-resolve
powershell -ExecutionPolicy Bypass -File build\test_ranged.ps1  # unidades con alcance
```

## Créditos de arte

- Unidades: **"Medieval RTS" por Kenney (kenney.nl)** — licencia
  [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) (dominio público;
  la atribución es opcional). Archivo de licencia incluido en
  `assets/sprites/LICENSE-Kenney.txt`. Fuente:
  <https://opengameart.org/content/medieval-rts-120>
