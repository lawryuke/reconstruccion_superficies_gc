#!/bin/bash
# ==============================================================================
# Script de ejecución para Laboratorio de Ball Pivoting (Linux)
# Punto 5: Medición de tiempos de ejecución para 2503 y 20 000 puntos
# ==============================================================================

set -e

echo "=================================================================="
echo "    LABORATORIO BALL PIVOTING - MEDICIÓN DE TIEMPOS EN LINUX     "
echo "=================================================================="

# 1. Comprobar compilador g++
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ no está instalado en este sistema."
    echo "Puedes instalarlo en Ubuntu/Debian con: sudo apt install build-essential"
    exit 1
fi

# 2. Compilar el programa de benchmarking con optimizaciones (-O3)
echo "[1/3] Compilando el programa de medición de tiempos (medicion_tiempos_bpa.cpp)..."
g++ -O3 -std=c++11 -Wall medicion_tiempos_bpa.cpp -o medicion_tiempos_bpa
echo "      Compilación exitosa."

# 3. Comprobar que los archivos XYZ existan
if [ ! -f "Stanford_Bunny_2503.xyz" ] || [ ! -f "Stanford_Bunny_20000.xyz" ]; then
    echo "ERROR: No se encontraron los archivos .xyz en este directorio."
    exit 1
fi

# 4. Menú de ejecución interactivo
echo ""
echo "Selecciona una opción de ejecución:"
echo "  1) Medir solo 2503 puntos (Stanford_Bunny_2503.xyz)"
echo "  2) Medir solo 20 000 puntos (Stanford_Bunny_20000.xyz)"
echo "  3) Medir AMBOS (2503 y 20 000 puntos) y generar tabla comparativa [Recomendado]"
echo "  4) Medir TODO incluyendo prueba de estrés de 50 000 puntos"
echo ""
read -p "Ingresa tu opción (1-4, por defecto 3): " OPCION
OPCION=${OPCION:-3}

echo ""
case $OPCION in
    1)
        echo ">>> Ejecutando medición para 2503 puntos..."
        ./medicion_tiempos_bpa Stanford_Bunny_2503.xyz 0.007
        ;;
    2)
        echo ">>> Ejecutando medición para 20 000 puntos..."
        ./medicion_tiempos_bpa Stanford_Bunny_20000.xyz 0.007
        ;;
    3)
        echo ">>> Ejecutando medición para 2503 y 20 000 puntos..."
        ./medicion_tiempos_bpa Stanford_Bunny_2503.xyz 0.007
        ./medicion_tiempos_bpa Stanford_Bunny_20000.xyz 0.007
        ;;
    4)
        echo ">>> Ejecutando medición completa con prueba de estrés (50 000)..."
        ./medicion_tiempos_bpa --all
        ;;
    *)
        echo "Opción no válida. Ejecutando la opción 3 por defecto..."
        ./medicion_tiempos_bpa
        ;;
esac

echo ""
echo "=================================================================="
echo " Mediciones finalizadas. Revisa los tiempos obtenidos en ms y s.  "
echo " Los archivos .obj reconstruidos han sido guardados.              "
echo "=================================================================="
