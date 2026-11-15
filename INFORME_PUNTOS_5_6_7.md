# Laboratorio Ball Pivoting: Puntos 5, 6 y 7

**Materia / Práctica:** Reconstrucción de superficies con Ball Pivoting  
**Carpeta de trabajo:** `reconstruccion_superficies_gc`  
**Entorno de ejecución:** Linux (Ubuntu / Debian / WSL / Fedora)

---

## 5. Tiempo de ejecución para 2503 y 20 000 puntos

### 5.1 Metodología de Medición
Para medir con precisión de alta resolución el tiempo consumido por el algoritmo, se utilizó la biblioteca estándar `<chrono>` en C++ (`std::chrono::high_resolution_clock`), tal como lo indica la guía del laboratorio:

```cpp
#include <chrono>

auto inicio = chrono::high_resolution_clock::now();
generarMallaBPA(radioBPA);
auto fin = chrono::high_resolution_clock::now();

auto ms = chrono::duration_cast<chrono::milliseconds>(fin - inicio).count();
cout << "Tiempo CPU: " << ms << " ms (" << ms / 1000.0 << " s)" << endl;
```

El algoritmo se descompuso en dos fases críticas para aislar el cuello de botella:
1. **Fase 1: Construcción de vecindades (`construirVecindad`)**: Determina para cada punto qué otros puntos se encuentran a una distancia máxima de $2r$.
2. **Fase 2: Reconstrucción de malla BPA (`generarMallaBPA` multi-semilla)**: Búsqueda de semillas, pivotamiento alrededor de las aristas de frontera activa y verificación de esferas vacías.

---

### 5.2 Tabla de Resultados Empíricos

Al ejecutar el programa en tu máquina Linux (`./ejecutar_mediciones.sh` o `make run-all`), completa los valores exactos en la siguiente tabla de tu informe:

| Archivo de Entrada | Nube de Puntos ($N$) | Radio BPA ($r$) | Triángulos Generados | Tiempo Vecindad (CPU) | Tiempo Pivoteo (CPU) | Tiempo TOTAL CPU |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| `Stanford_Bunny_2503.xyz` | **2 503** | 0.007 | ~4 500 – 4 900 | *(anotar ms / s)* | *(anotar ms / s)* | **< 1 segundo** |
| `Stanford_Bunny_20000.xyz` | **20 000** | 0.007 | ~38 000 – 40 000 | *(anotar ms / s)* | *(anotar ms / s)* | **~10 – 35 segundos** |
| *(Opcional / Estrés)* `Stanford_Bunny_50000.xyz` | **50 000** | 0.007 | ~95 000 – 100 000 | *(anotar ms / s)* | *(anotar ms / s)* | **~1.5 – 4 minutos** |

> **Nota sobre variaciones de tiempo:** Los tiempos exactos varían en función de la CPU (frecuencia de reloj, número de núcleos y memoria caché L3). En procesadores modernos con compilación optimizada (`g++ -O3`), 2 503 puntos toma fracciones de segundo (~200 a 600 ms), mientras que 20 000 puntos toma típicamente entre 15 y 30 segundos.

---

### 5.3 Análisis del Comportamiento y Crecimiento
* **Aumento de la nube:** De 2 503 a 20 000 puntos representa un incremento de **~8 veces** en cantidad de vértices ($20\,000 / 2\,503 \approx 7.99$).
* **Aumento de pares potenciales de puntos:**
  $$\text{Pares}(N) = \frac{N(N - 1)}{2}$$
  * Para $N = 2\,503$: $\approx 3.13 \times 10^6$ pares (3.13 millones).
  * Para $N = 20\,000$: $\approx 2.00 \times 10^8$ pares (200 millones).
  * El número de comparaciones crece por un factor de **$\approx 64$ veces** ($\approx 8^2$).
* **Conclusión de la medición:** El tiempo de ejecución no escala de forma lineal $O(N)$, sino que exhibe un crecimiento superlineal con tendencia cuadrática $O(N^2)$ en su versión CPU sin indexación espacial acelerada por hardware, evidenciando la saturación de los núcleos secuenciales de la CPU.

---

## 6. Explicación breve de por qué aumentar el número de puntos hace conveniente utilizar CUDA

Aumentar el número de puntos hace conveniente utilizar **CUDA** por las siguientes razones fundamentales de arquitectura de hardware y complejidad computacional:

1. **Explosión cuadrática de cálculos geométricos independientes:**  
   Al pasar de 2 503 puntos a 20 000 (o 50 000), la cantidad de distancias euclidianas tridimensionales, normas vectoriales y comprobaciones de pertenencia dentro de esferas pasa de pocos millones a cientos de millones. Todas estas operaciones matemáticas ($d = \sqrt{\Delta x^2 + \Delta y^2 + \Delta z^2}$) son **mutuamente independientes** y no requieren comunicación entre puntos.

2. **Diferencia arquitectónica entre CPU y GPU (SIMT):**  
   * La **CPU** está diseñada con pocos núcleos (4 a 16 núcleos) optimizados para minimizar la latencia de instrucciones complejas y secuencias con muchas ramas condicionales.
   * La **GPU**, mediante el modelo **SIMT (Single Instruction, Multiple Threads)** de CUDA, posee miles de unidades de cómputo (CUDA cores) diseñadas para procesamiento masivo de datos en paralelo (*high throughput*).

3. **Paralelismo de datos ("Data Parallelism"):**  
   En la CPU, evaluar los 200 millones de pares de distancias se realiza mediante bucles anidados secuenciales. En CUDA, se puede asignar un hilo a cada punto o a cada par candidato. Miles de hilos ejecutan simultáneamente la misma operación aritmética sobre diferentes coordenadas de puntos en memoria, reduciendo el tiempo de procesamiento de decenas de segundos en CPU a **milisegundos** en GPU.

4. **Mejora del Speedup:**  
   $$\text{Speedup} = \frac{T_{\text{CPU}}}{T_{\text{GPU}}}$$  
   Para nubes pequeñas (como 2 503 puntos), el costo de transferir datos entre la memoria del host (RAM) y la memoria del device (VRAM de la GPU por PCIe) atenúa la ganancia. Sin embargo, conforme $N \ge 20\,000$, el tiempo de cómputo domina ampliamente sobre el tiempo de transferencia, haciendo que el Speedup aumente drásticamente, justificando el uso de CUDA.

---

## 7. Identificación de al menos dos funciones o secciones del programa que podrían paralelizarse

De acuerdo con el análisis del código fuente (`03_Stanford_Bunny_BPA_MULTISEMILLA_CENTRADO.cpp`), el algoritmo de Ball Pivoting presenta dos secciones computacionales intensivas que representan los principales cuellos de botella y son ideales para paralelizarse con CUDA:

### Sección 1: Construcción de Vecindades (`construirVecindad`)

* **Ubicación en el código:**
  ```cpp
  void construirVecindad(float radio) {
      vecinosBPA.assign(puntos.size(), vector<int>());
      float limite = 2.0f * radio;

      for (int i = 0; i < (int)puntos.size(); i++) {
          for (int j = i + 1; j < (int)puntos.size(); j++) {
              if (distancia(puntos[i], puntos[j]) <= limite) {
                  vecinosBPA[i].push_back(j);
                  vecinosBPA[j].push_back(i);
              }
          }
      }
  }
  ```
* **Por qué es paralelizable:**  
  Es un problema clásicamente clasificado como *"embarrassingly parallel"* (altamente paralelizable). El cálculo de la distancia euclidiana entre el punto $P_i$ y el punto $P_j$ es completamente independiente del cálculo entre $P_i$ y $P_k$. Ningún hilo necesita esperar el resultado de otro hilo.
* **Estrategia en CUDA:**  
  * Se transfiere el arreglo de puntos a la memoria global de la GPU.
  * Se lanza un kernel 1D o 2D donde cada hilo de GPU con índice global `idx = blockIdx.x * blockDim.x + threadIdx.x` se encarga de un punto $P_i$ y calcula en paralelo la distancia con los demás puntos.
  * Opcionalmente, se puede optimizar aún más asignando los puntos a una grilla espacial uniforme (*spatial hashing / voxel grid*) calculada en GPU, reduciendo las comparaciones únicamente a celdas contiguas.

---

### Sección 2: Verificación de Esfera Vacía (`esferaVacia`) y Evaluación de Candidatos

* **Ubicación en el código:**
  ```cpp
  bool esferaVacia(const Punto& centro, float radio, int ia, int ib, int ic) {
      float limite = radio - 1e-6f;
      const vector<int>& candidatos = vecinosBPA[ia];

      for (int i : candidatos) {
          if (i == ia || i == ib || i == ic) continue;
          if (distancia(centro, puntos[i]) < limite) return false;
      }
      return true;
  }
  ```
* **Por qué es paralelizable:**  
  Durante la fase de pivotamiento, cada vez que una arista activa $(A, B)$ busca su siguiente triángulo en `buscarPuntoBPA`, se generan dos centros de esfera posibles ($S_1, S_2$) para cada vecino de $A$. La función `esferaVacia` debe recorrer toda la lista de vecinos para verificar si algún punto cae estrictamente dentro de la esfera.  
  * La condición de que un punto $k$ esté o no dentro de la esfera $(\|S - P_k\| < r)$ no altera la comprobación de los demás puntos vecinos.
* **Estrategia en CUDA:**  
  * En lugar de iterar secuencialmente sobre la lista de candidatos en la CPU, se puede lanzar un kernel de verificación donde un grupo de hilos evalúa simultáneamente todos los vecinos respecto al centro de la esfera $S$.
  * Se utiliza una operación de reducción booleana o una variable en memoria compartida (`__shared__`) para registrar si alguno de los puntos viola la condición de esfera vacía.

---

### 7.1 Síntesis del Modelo Híbrido CPU - GPU

Siguiendo las recomendaciones de la práctica, no es conveniente migrar la totalidad del programa a la GPU:

| Tarea / Componente | Dispositivo Óptimo | Justificación Técnica |
| :--- | :---: | :--- |
| **Control del flujo del algoritmo** | **CPU** | Lógica secuencial, gestión de estados y excepciones. |
| **Cola de frontera activa (`queue<AristaActiva>`)** | **CPU** | Operaciones de inserción/extracción dinámicas con dependencias de orden. |
| **Cálculo masivo de distancias** | **GPU (CUDA)** | Operaciones aritméticas uniformes sobre millones de pares. |
| **Construcción de vecindades locales** | **GPU (CUDA)** | Miles de hilos filtrando puntos con $\|P_i - P_j\| \le 2r$. |
| **Comprobación de esfera vacía** | **GPU (CUDA)** | Verificación geométrica simultánea para múltiples candidatos. |
