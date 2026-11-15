# Reconstrucción de Superficies con Ball Pivoting (Puntos 5, 6 y 7)

Este directorio contiene todo el código fuente, archivos de puntos y scripts configurados para compilar y ejecutar en **Linux**.

---

## Estructura de Archivos

* `Stanford_Bunny_2503.xyz`: Nube de puntos original reducida (2 503 puntos).
* `Stanford_Bunny_20000.xyz`: Nube de puntos intermedia (20 000 puntos).
* `Stanford_Bunny_50000.xyz`: Nube de puntos grande para prueba de estrés (50 000 puntos).
* `medicion_tiempos_bpa.cpp`: Programa de medición de tiempos en consola usando `std::chrono` (Punto 5). No requiere entorno gráfico X11.
* `Stanford_Bunny_BPA_OpenGL.cpp`: Visualizador interactivo 3D con OpenGL/GLUT con temporizador de alta resolución integrado.
* `Makefile`: Automatización de compilación y ejecución para Linux.
* `ejecutar_mediciones.sh`: Script interactivo de consola para ejecutar las pruebas con un solo comando.
* `INFORME_PUNTOS_5_6_7.md`: Documento con el desarrollo teórico y respuestas de los puntos 5, 6 y 7.

---

## 1. Requisitos previos en Linux

En sistemas basados en Debian / Ubuntu / Linux Mint / WSL, instala el compilador y las librerías gráficas necesarias:

```bash
sudo apt update
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev
```

---

## 2. Comandos para compilar y ejecutar

### Opción A: Mediante el script interactivo (La más rápida)

```bash
# 1. Dar permisos de ejecución al script
chmod +x ejecutar_mediciones.sh

# 2. Ejecutar el script
./ejecutar_mediciones.sh
```

---

### Opción B: Mediante Makefile

```bash
# Para compilar todo (benchmark y visualizador):
make

# Para ejecutar la medición de 2503 puntos:
make run-2503

# Para ejecutar la medición de 20 000 puntos:
make run-20000

# Para ejecutar la comparativa completa (2503, 20000 y 50000 puntos):
make run-all
```

---

### Opción C: Compilación manual con `g++`

```bash
# 1. Compilar el programa de medición (optimizado con -O3):
g++ -O3 -std=c++11 -Wall medicion_tiempos_bpa.cpp -o medicion_tiempos_bpa

# 2. Ejecutar para 2503 puntos:
./medicion_tiempos_bpa Stanford_Bunny_2503.xyz 0.007

# 3. Ejecutar para 20 000 puntos:
./medicion_tiempos_bpa Stanford_Bunny_20000.xyz 0.007

# 4. Si quieres abrir el visualizador gráfico 3D (requiere entorno con monitor):
g++ -O3 -std=c++11 -Wall Stanford_Bunny_BPA_OpenGL.cpp -o visualizador_bpa -lGL -lGLU -lglut
./visualizador_bpa Stanford_Bunny_2503.xyz
```
*(Dentro del visualizador interactivo, presiona `1`, `2`, `3` o `B` para reconstruir la malla y ver el tiempo de ejecución en la terminal).*
