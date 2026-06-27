Este proyecto implementa un QuadTree desde cero para optimizar la detección de colisiones y búsqueda de vecinos en un simulador de partículas 2D, comparando su rendimiento contra una solución ingenua de fuerza bruta.

## Requisitos previos

- **Compilador C++**: `g++` (Linux) o MinGW (Windows).
- **Librería**: [raylib](https://www.raylib.com/) para la visualización gráfica.

## Instrucciones de Compilación y Ejecución

### Para usuarios de Linux (Probado en Arch Linux / Ubuntu)

1. **Instalar dependencias**:
   Asegúrate de tener el compilador y raylib instalados en tu sistema.
   En Arch Linux:
   ```bash
   sudo pacman -S base-devel raylib
Compilar:
Abre una terminal en la raíz del proyecto (carpeta quadtree) y ejecuta:

```bash
g++ main.cpp -o programa_linux -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```

Ejecutar:

```bash
./programa_linux
```

Para usuarios de Windows
El proyecto incluye los archivos precompilados de raylib (raylib.dll y raylib.lib) en los directorios correspondientes para facilitar la compilación.

Compilar:
Abre tu terminal (PowerShell o CMD) en la carpeta quadtree, asegurándote de tener g++ en tu PATH, y ejecuta:

```DOS
g++ main.cpp -o programa.exe -O1 -Wall -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm
```

Ejecutar:

```DOS
.\programa.exe
```
