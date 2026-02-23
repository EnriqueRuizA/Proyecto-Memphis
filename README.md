# Proyecto-Memphis (Medieval Conquest)

Videojuego RTS de campaña con batallas en tiempo real. C++17 + Raylib.

## Estructura del proyecto

```
Proyecto-Memphis/
├── include/
│   ├── game_types.h    # Tipos, constantes, enums, utilidades inline
│   └── game_globals.h  # Declaraciones del estado global (extern)
├── src/
│   └── globals.cpp     # Definiciones del estado global
├── rts_game.cpp        # Lógica de juego: unidades, campaña, batalla, UI, estados
├── complilacion.bat    # Compilación en Windows (w64devkit + raylib)
├── Makefile            # Alternativa con make (ajusta RAYLIB_PATH)
└── README.md
```

- **Rendimiento:** Tipos y utilidades críticas (vdist, vnorm, etc.) en cabecera como `inline`. Un solo `.cpp` principal para favorecer optimización e incrementales.
- **Mantenibilidad:** Tipos y estado global separados; cambios en datos no obligan a tocar toda la lógica.

## Compilación

- **Windows (batch):** Ejecutar `complilacion.bat` (requiere w64devkit y raylib en `c:/raylib/raylib/src`).
- **Con make:** Ajustar `RAYLIB_PATH` en el `Makefile` y ejecutar `make`.
- **Manual:**  
  `g++ -std=c++17 -Iinclude -I<raylib_include> src/globals.cpp rts_game.cpp -o rts_game.exe -L<raylib_lib> -lraylib -lopengl32 -lgdi32 -lwinmm -lm`

## Partida guardada

La campaña se guarda en `campaign_save.dat` (al salir del juego si hay partida en curso). CONTINUE en el menú principal carga desde ese fichero.
