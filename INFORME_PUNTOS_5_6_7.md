# Laboratorio Ball Pivoting: Puntos 5, 6 y 7

**Materia / Práctica:** Reconstrucción de superficies con Ball Pivoting  
**Carpeta de trabajo:** `reconstruccion_superficies_gc`  
**Entorno de ejecución:** Linux (Mediciones reales obtenidas)

---

## 5. Tiempo de ejecución para 2503 y 20 000 puntos

### 5.1 Metodología de Medición
Para medir con precisión de alta resolución el tiempo consumido por el algoritmo, se utilizó la biblioteca estándar `<chrono>` en C++ (`std::chrono::high_resolution_clock`), desglosando la ejecución en dos fases principales:
1. **Fase 1: Construcción de vecindades (`construirVecindad`)**: Determina para cada punto qué otros puntos se encuentran a una distancia máxima de $2r$.
2. **Fase 2: Reconstrucción de malla BPA (`generarMallaBPA` multi-semilla)**: Búsqueda de semillas, pivotamiento alrededor de las aristas activas y verificación de esferas vacías.

---

### 5.2 Tabla de Resultados Experimentales Obtenidos

A continuación se presentan los resultados exactos medidos con radio $r = 0.007$:

| Archivo de Entrada | Nube de Puntos ($N$) | Radio ($r$) | Triángulos Generados | Regiones Semilla | Puntos Cubiertos | Tiempo Vecindad (CPU) | Tiempo Pivoteo (CPU) | TIEMPO TOTAL CPU |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| `Stanford_Bunny_2503.xyz` | **2 503** | 0.007 | **6 564** | 1 | 2 490 / 2 503 | **9.52 ms** | **34.38 ms** | **43.92 ms (0.0439 s)** |
| `Stanford_Bunny_20000.xyz` | **20 000** | 0.007 | **40 801** | 189 | 19 217 / 20 000 | **449.13 ms** | **10 425.46 ms** | **10 874.61 ms (10.8746 s)** |

---

### 5.3 Análisis del Comportamiento y Crecimiento
* **Factor de incremento en puntos:**  
  $$\frac{20\,000}{2\,503} \approx 7.99\times \text{ (aproximadamente } 8\times\text{)}$$
* **Factor de incremento en tiempo de Vecindad:**  
  $$\frac{449.13\text{ ms}}{9.52\text{ ms}} \approx 47.18\times$$
* **Factor de incremento en tiempo de Pivoteo y Búsqueda:**  
  $$\frac{10\,425.46\text{ ms}}{34.38\text{ ms}} \approx 303.24\times$$
* **Factor de incremento en Tiempo TOTAL:**  
  $$\frac{10\,874.61\text{ ms}}{43.92\text{ ms}} \approx 247.60\times$$

**Conclusión:**  
Multiplicar la nube de puntos por tan solo un factor de $\approx 8$ provocó que el tiempo de ejecución total se multiplicara por **$\approx 248$ veces** (pasando de menos de $0.05$ segundos a casi $11$ segundos). Esto demuestra de forma contundente la complejidad superlineal $O(N^2)$ y la saturación del procesamiento monohilo en CPU.

---

## 6. Explicación breve de por qué aumentar el número de puntos hace conveniente utilizar CUDA

Los resultados experimentales del **Punto 5** justifican plenamente la conveniencia de utilizar **CUDA**:

1. **Evidencia empírica de saturación en CPU:**  
   Al pasar de 2 503 a 20 000 puntos, el tiempo total creció en un **$24 760\%$** ($247.6\times$). Este incremento descomunal se debe a que el número de comparaciones de pares potenciales crece cuadráticamente:
   $$\text{Pares}(N) = \frac{N(N - 1)}{2}$$
   * Para 2 503 puntos: $\approx 3.13$ millones de pares.
   * Para 20 000 puntos: $\approx 200$ millones de pares.
   Evaluar 200 millones de distancias y pruebas de esfera vacía en una CPU de forma secuencial desborda la capacidad de los pocos núcleos disponibles.

2. **Alineación con la arquitectura SIMT de CUDA:**  
   El cálculo de la distancia euclidiana tridimensional entre dos puntos ($d = \sqrt{\Delta x^2 + \Delta y^2 + \Delta z^2}$) y la prueba de pertenencia a una esfera son operaciones **mutuamente independientes** (paralelismo de datos masivo). Una GPU moderna cuenta con miles de núcleos CUDA que pueden procesar millones de pares simultáneamente en lugar de recorrer bucles anidados en serie.

3. **Amortización de la transferencia de datos y Speedup:**  
   $$\text{Speedup} = \frac{T_{\text{CPU}}}{T_{\text{GPU}}}$$
   Para 2 503 puntos, el tiempo en CPU es de apenas 44 ms, por lo que el costo de transferir la memoria a la GPU vía bus PCIe no genera una gran diferencia. Sin embargo, para 20 000 puntos, donde la CPU tarda casi 11 segundos (10 874 ms), la GPU reduce el cálculo de distancias y vecindades a unos pocos milisegundos, logrando un **Speedup masivo** que justifica totalmente la aceleración por hardware.

---

## 7. Identificación de al menos dos funciones o secciones del programa que podrían paralelizarse

Del análisis del código fuente y del perfil de tiempos obtenido, se identifican las dos secciones que representan el cuello de botella:

### Sección 1: Construcción de Vecindades (`construirVecindad`)
* **Ubicación:** Líneas ~247 a 267 en el código base.
* **Tiempo medido:** Subió de **9.52 ms** a **449.13 ms** (un incremento de $47\times$).
* **¿Por qué es paralelizable?:**  
  La función contiene dos bucles anidados `for (int i = 0; i < N; i++) for (int j = i + 1; j < N; j++)` para evaluar si $\text{dist}(P_i, P_j) \le 2r$. Cada distancia entre el par $(i, j)$ es completamente independiente de los demás pares.
* **Propuesta CUDA:**  
  Un kernel `__global__ void kernelConstruirVecindad(...)` donde cada hilo de la GPU se asigna a un punto $i$ o a un bloque de puntos para calcular distancias en paralelo sobre la memoria de la tarjeta gráfica, eliminando por completo el doble bucle secuencial de la CPU.

---

### Sección 2: Verificación de Esfera Vacía (`esferaVacia`) y Evaluación de Candidatos (`buscarPuntoBPA`)
* **Ubicación:** Líneas ~273 a 293 y ~450 a 505.
* **Tiempo medido:** Fue el mayor cuello de botella, pasando de **34.38 ms** a **10 425.46 ms** (más de 10 segundos, un incremento de $303\times$).
* **¿Por qué es paralelizable?:**  
  Durante el pivoteo, cada arista activa evalúa decenas de candidatos vecinos y, para cada uno, calcula los centros de esfera $(S_1, S_2)$ y llama a `esferaVacia`. En `esferaVacia`, se comprueba si algún otro punto cae a distancia menor al radio de la esfera. Cada una de estas comprobaciones es una prueba geométrica independiente.
* **Propuesta CUDA:**  
  Lanzar un bloque de hilos para cada candidato, donde múltiples hilos comprueban simultáneamente si los puntos de la nube violan la condición de esfera vacía mediante una reducción paralela en memoria compartida (`__shared__`).

---

### 7.1 Esquema del Modelo Híbrido CPU - GPU Recomendado

| Componente | Dispositivo | Motivo de Selección |
| :--- | :---: | :--- |
| **Control general del algoritmo y frontera activa** | **CPU** | Gestión de colas (`queue<AristaActiva>`) y flujo secuencial de regiones. |
| **Cálculo masivo de distancias y vecindades** | **GPU (CUDA)** | Miles de hilos evaluando pares de puntos en paralelo. |
| **Verificación geométrica de esferas vacías** | **GPU (CUDA)** | Evaluación concurrente de candidatos descartando falsos pivotes. |
