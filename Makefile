# ==============================================================================
# Makefile para Laboratorio de Ball Pivoting (Linux)
# Generación de Malla y Medición de Rendimiento CPU vs CUDA
# ==============================================================================

CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall
LIBS_GL = -lGL -lGLU -lglut

# Nombres de ejecutables
BIN_BENCHMARK = medicion_tiempos_bpa
BIN_OPENGL = visualizador_bpa

# Archivos de código fuente
SRC_BENCHMARK = medicion_tiempos_bpa.cpp
SRC_OPENGL = Stanford_Bunny_BPA_OpenGL.cpp

.PHONY: all benchmark visualizador run-2503 run-20000 run-50000 run-all clean help

all: benchmark visualizador

# Compilación del programa de medición de tiempos (sin interfaz gráfica, ideal para servidores o terminal)
benchmark: $(BIN_BENCHMARK)

$(BIN_BENCHMARK): $(SRC_BENCHMARK)
	@echo "[Compilando] Programa de medición de tiempos ($@)..."
	$(CXX) $(CXXFLAGS) $(SRC_BENCHMARK) -o $(BIN_BENCHMARK)
	@echo "[OK] Compilación completada con éxito."

# Compilación del visualizador interactivo 3D con OpenGL / GLUT
visualizador: $(BIN_OPENGL)

$(BIN_OPENGL): $(SRC_OPENGL)
	@echo "[Compilando] Visualizador OpenGL ($@)..."
	$(CXX) $(CXXFLAGS) $(SRC_OPENGL) $(LIBS_GL) -o $(BIN_OPENGL)
	@echo "[OK] Compilación con OpenGL completada."

# Ejecutables directos para las tareas del laboratorio
run-2503: benchmark
	./$(BIN_BENCHMARK) Stanford_Bunny_2503.xyz 0.007

run-20000: benchmark
	./$(BIN_BENCHMARK) Stanford_Bunny_20000.xyz 0.007

run-50000: benchmark
	./$(BIN_BENCHMARK) Stanford_Bunny_50000.xyz 0.007

run-all: benchmark
	./$(BIN_BENCHMARK) --all

clean:
	@echo "Limpiando ejecutables y mallas generadas..."
	rm -f $(BIN_BENCHMARK) $(BIN_OPENGL) Bunny_*_reconstruido.obj Bunny_BPA_generado.obj
	@echo "Listo."

help:
	@echo "Opciones disponibles en este Makefile:"
	@echo "  make benchmark     : Compila el programa de medición de tiempos CPU (Punto 5)"
	@echo "  make visualizador  : Compila el visor 3D interactivo con OpenGL"
	@echo "  make run-2503      : Ejecuta la medición para 2503 puntos"
	@echo "  make run-20000     : Ejecuta la medición para 20000 puntos"
	@echo "  make run-all       : Ejecuta la medición comparativa completa (2503, 20000 y 50000)"
	@echo "  make clean         : Elimina binarios y archivos generados"
