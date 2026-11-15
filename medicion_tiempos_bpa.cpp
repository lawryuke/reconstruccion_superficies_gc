#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <queue>
#include <set>
#include <tuple>
#include <chrono>
#include <iomanip>

using namespace std;

const float PI  = 3.14159265358979323846f;
const float EPS = 1e-12f;

struct Punto
{
    float x, y, z;
};

struct Triangulo
{
    int a, b, c;
};

struct AristaActiva
{
    int a, b;
    Punto centro;
};

struct ResultadoPivot
{
    int punto;
    Punto centro;
    float angulo;
    bool encontrado;
};

struct MetricasBPA
{
    int numPuntos;
    float radio;
    int numTriangulos;
    int numRegiones;
    int puntosCubiertos;
    double tiempoVecindadMs;
    double tiempoPivoteoMs;
    double tiempoTotalMs;
};

// Variables globales para el algoritmo
vector<Punto> puntos;
vector<Triangulo> malla;
vector<vector<int>> vecinosBPA;
Punto centroModelo = {0, 0, 0};

// Operaciones vectoriales
inline Punto sumar(const Punto& A, const Punto& B)
{
    return {A.x + B.x, A.y + B.y, A.z + B.z};
}

inline Punto restar(const Punto& A, const Punto& B)
{
    return {A.x - B.x, A.y - B.y, A.z - B.z};
}

inline Punto multiplicar(const Punto& A, float k)
{
    return {A.x * k, A.y * k, A.z * k};
}

inline float productoPunto(const Punto& A, const Punto& B)
{
    return A.x * B.x + A.y * B.y + A.z * B.z;
}

inline Punto productoCruz(const Punto& A, const Punto& B)
{
    return {
        A.y * B.z - A.z * B.y,
        A.z * B.x - A.x * B.z,
        A.x * B.y - A.y * B.x
    };
}

inline float norma(const Punto& A)
{
    return sqrt(productoPunto(A, A));
}

inline Punto normalizar(const Punto& A)
{
    float n = norma(A);
    if (n < EPS)
        return {0, 0, 0};
    return {A.x / n, A.y / n, A.z / n};
}

inline float distancia(const Punto& A, const Punto& B)
{
    return norma(restar(A, B));
}

// Cargar nube XYZ
bool cargarXYZ(const string& nombre)
{
    ifstream archivo(nombre.c_str());
    if (!archivo)
        return false;

    puntos.clear();
    string linea;
    while (getline(archivo, linea))
    {
        if (linea.empty() || linea[0] == '#')
            continue;

        stringstream ss(linea);
        Punto p;
        if (ss >> p.x >> p.y >> p.z)
            puntos.push_back(p);
    }
    return !puntos.empty();
}

void calcularCentroModelo()
{
    if (puntos.empty())
        return;

    float minX = puntos[0].x, maxX = puntos[0].x;
    float minY = puntos[0].y, maxY = puntos[0].y;
    float minZ = puntos[0].z, maxZ = puntos[0].z;

    for (const Punto& p : puntos)
    {
        minX = min(minX, p.x); maxX = max(maxX, p.x);
        minY = min(minY, p.y); maxY = max(maxY, p.y);
        minZ = min(minZ, p.z); maxZ = max(maxZ, p.z);
    }

    centroModelo.x = (minX + maxX) / 2.0f;
    centroModelo.y = (minY + maxY) / 2.0f;
    centroModelo.z = (minZ + maxZ) / 2.0f;
}

bool centrosEsfera(const Punto& A, const Punto& B, const Punto& C, float radio, Punto& S1, Punto& S2)
{
    Punto u = restar(B, A);
    Punto v = restar(C, A);
    Punto w = productoCruz(u, v);

    float ww = productoPunto(w, w);
    if (ww < EPS)
        return false;

    float uu = productoPunto(u, u);
    float vv = productoPunto(v, v);

    Punto vxw = productoCruz(v, w);
    Punto wxu = productoCruz(w, u);

    Punto O;
    O.x = A.x + (uu * vxw.x + vv * wxu.x) / (2.0f * ww);
    O.y = A.y + (uu * vxw.y + vv * wxu.y) / (2.0f * ww);
    O.z = A.z + (uu * vxw.z + vv * wxu.z) / (2.0f * ww);

    float Rc = distancia(O, A);
    if (Rc > radio + EPS)
        return false;

    float h2 = radio * radio - Rc * Rc;
    if (h2 < 0.0f)
        h2 = 0.0f;

    float h = sqrt(h2);
    Punto n = normalizar(w);

    S1 = sumar(O, multiplicar(n, h));
    S2 = sumar(O, multiplicar(n, -h));

    return true;
}

// ============================================================
// SECCIÓN CRÍTICA A PARALELIZAR 1: Construcción de Vecindades
// Complejidad actual: O(N^2) en CPU secuencial
// ============================================================
void construirVecindad(float radio)
{
    vecinosBPA.assign(puntos.size(), vector<int>());
    float limite = 2.0f * radio;

    for (int i = 0; i < (int)puntos.size(); i++)
    {
        for (int j = i + 1; j < (int)puntos.size(); j++)
        {
            if (distancia(puntos[i], puntos[j]) <= limite)
            {
                vecinosBPA[i].push_back(j);
                vecinosBPA[j].push_back(i);
            }
        }
    }
}

// ============================================================
// SECCIÓN CRÍTICA A PARALELIZAR 2: Verificación de Esfera Vacía
// Evaluada repetitivamente para cada candidato y cada triángulo
// ============================================================
bool esferaVacia(const Punto& centro, float radio, int ia, int ib, int ic)
{
    float limite = radio - 1e-6f;
    const vector<int>& candidatos = vecinosBPA[ia];

    for (int i : candidatos)
    {
        if (i == ia || i == ib || i == ic)
            continue;

        if (distancia(centro, puntos[i]) < limite)
            return false;
    }
    return true;
}

float anguloPivote(const Punto& A, const Punto& B, const Punto& centroActual, const Punto& centroNuevo)
{
    Punto eje = normalizar(restar(B, A));
    Punto M = multiplicar(sumar(A, B), 0.5f);

    Punto u = restar(centroActual, M);
    Punto v = restar(centroNuevo, M);

    u = restar(u, multiplicar(eje, productoPunto(u, eje)));
    v = restar(v, multiplicar(eje, productoPunto(v, eje)));

    float nu = norma(u);
    float nv = norma(v);

    if (nu < EPS || nv < EPS)
        return 1e30f;

    u = multiplicar(u, 1.0f / nu);
    v = multiplicar(v, 1.0f / nv);

    float seno = productoPunto(eje, productoCruz(u, v));
    float coseno = productoPunto(u, v);

    float angulo = atan2(seno, coseno);
    if (angulo <= EPS)
        angulo += 2.0f * PI;

    return angulo;
}

inline pair<int, int> claveArista(int a, int b)
{
    if (a > b)
        swap(a, b);
    return {a, b};
}

inline tuple<int, int, int> claveTriangulo(int a, int b, int c)
{
    int v[3] = {a, b, c};
    sort(v, v + 3);
    return make_tuple(v[0], v[1], v[2]);
}

bool buscarSemillaNueva(float radio,
                        const set<tuple<int, int, int>>& triangulosUsados,
                        const vector<bool>& puntoCubierto,
                        Triangulo& semilla,
                        Punto& centro)
{
    const int MAX_VECINOS = 40;

    for (int i = 0; i < (int)puntos.size(); i++)
    {
        if (puntoCubierto[i])
            continue;

        vector<pair<float, int>> vecinos;
        for (int j : vecinosBPA[i])
        {
            vecinos.push_back({distancia(puntos[i], puntos[j]), j});
        }
        sort(vecinos.begin(), vecinos.end());

        if ((int)vecinos.size() > MAX_VECINOS)
            vecinos.resize(MAX_VECINOS);

        for (int a = 0; a < (int)vecinos.size(); a++)
        {
            for (int b = a + 1; b < (int)vecinos.size(); b++)
            {
                int j = vecinos[a].second;
                int k = vecinos[b].second;

                tuple<int, int, int> clave = claveTriangulo(i, j, k);
                if (triangulosUsados.count(clave))
                    continue;

                Punto S1, S2;
                if (!centrosEsfera(puntos[i], puntos[j], puntos[k], radio, S1, S2))
                    continue;

                if (esferaVacia(S1, radio, i, j, k))
                {
                    semilla = {i, j, k};
                    centro = S1;
                    return true;
                }

                if (esferaVacia(S2, radio, i, j, k))
                {
                    semilla = {i, j, k};
                    centro = S2;
                    return true;
                }
            }
        }
    }
    return false;
}

ResultadoPivot buscarPuntoBPA(int a, int b, const Punto& centroActual, float radio,
                              const set<tuple<int, int, int>>& triangulosUsados)
{
    ResultadoPivot mejor;
    mejor.punto = -1;
    mejor.angulo = 1e30f;
    mejor.encontrado = false;

    const Punto& A = puntos[a];
    const Punto& B = puntos[b];

    for (int i : vecinosBPA[a])
    {
        if (i == a || i == b)
            continue;

        if (distancia(B, puntos[i]) > 2.0f * radio)
            continue;

        if (triangulosUsados.count(claveTriangulo(a, b, i)))
            continue;

        Punto S1, S2;
        if (!centrosEsfera(A, B, puntos[i], radio, S1, S2))
            continue;

        Punto centros[2] = {S1, S2};
        for (int k = 0; k < 2; k++)
        {
            Punto S = centros[k];
            if (!esferaVacia(S, radio, a, b, i))
                continue;

            float angulo = anguloPivote(A, B, centroActual, S);
            if (angulo < mejor.angulo)
            {
                mejor.punto = i;
                mejor.centro = S;
                mejor.angulo = angulo;
                mejor.encontrado = true;
            }
        }
    }
    return mejor;
}

// Medición completa de generación BPA
MetricasBPA medirGeneracionBPA(float radio, bool verbose = true)
{
    MetricasBPA metricas;
    metricas.numPuntos = (int)puntos.size();
    metricas.radio = radio;

    auto tInicioTotal = chrono::high_resolution_clock::now();

    // 1. Fase de construcción de vecindad
    auto tInicioVecindad = chrono::high_resolution_clock::now();
    construirVecindad(radio);
    auto tFinVecindad = chrono::high_resolution_clock::now();
    metricas.tiempoVecindadMs = chrono::duration<double, milli>(tFinVecindad - tInicioVecindad).count();

    if (verbose)
    {
        cout << "  [+] Vecindades construidas en: " << fixed << setprecision(2)
             << metricas.tiempoVecindadMs << " ms ("
             << metricas.tiempoVecindadMs / 1000.0 << " s)" << endl;
    }

    // 2. Fase de triangulación BPA (multi-semilla + pivoteo de aristas)
    auto tInicioPivoteo = chrono::high_resolution_clock::now();
    malla.clear();
    set<tuple<int, int, int>> triangulosUsados;
    vector<bool> puntoCubierto(puntos.size(), false);
    int regiones = 0;

    const int MAX_TRIANGULOS = max(15000, (int)puntos.size() * 3);

    while ((int)malla.size() < MAX_TRIANGULOS)
    {
        Triangulo semilla;
        Punto centroSemilla;

        if (!buscarSemillaNueva(radio, triangulosUsados, puntoCubierto, semilla, centroSemilla))
            break;

        regiones++;
        queue<AristaActiva> frontera;
        set<pair<int, int>> aristasCerradas;

        tuple<int, int, int> ks = claveTriangulo(semilla.a, semilla.b, semilla.c);
        if (triangulosUsados.count(ks))
            continue;

        malla.push_back(semilla);
        triangulosUsados.insert(ks);

        puntoCubierto[semilla.a] = true;
        puntoCubierto[semilla.b] = true;
        puntoCubierto[semilla.c] = true;

        frontera.push({semilla.a, semilla.b, centroSemilla});
        frontera.push({semilla.b, semilla.c, centroSemilla});
        frontera.push({semilla.c, semilla.a, centroSemilla});

        while (!frontera.empty() && (int)malla.size() < MAX_TRIANGULOS)
        {
            AristaActiva e = frontera.front();
            frontera.pop();

            pair<int, int> clave = claveArista(e.a, e.b);
            if (aristasCerradas.count(clave))
                continue;

            ResultadoPivot r = buscarPuntoBPA(e.a, e.b, e.centro, radio, triangulosUsados);
            if (!r.encontrado)
            {
                aristasCerradas.insert(clave);
                continue;
            }

            Triangulo t = {e.a, e.b, r.punto};
            tuple<int, int, int> kt = claveTriangulo(t.a, t.b, t.c);
            if (triangulosUsados.count(kt))
            {
                aristasCerradas.insert(clave);
                continue;
            }

            malla.push_back(t);
            triangulosUsados.insert(kt);

            puntoCubierto[t.a] = true;
            puntoCubierto[t.b] = true;
            puntoCubierto[t.c] = true;

            aristasCerradas.insert(clave);

            pair<int, int> e1 = claveArista(t.b, t.c);
            pair<int, int> e2 = claveArista(t.c, t.a);

            if (!aristasCerradas.count(e1))
                frontera.push({t.b, t.c, r.centro});

            if (!aristasCerradas.count(e2))
                frontera.push({t.c, t.a, r.centro});
        }
    }

    auto tFinPivoteo = chrono::high_resolution_clock::now();
    metricas.tiempoPivoteoMs = chrono::duration<double, milli>(tFinPivoteo - tInicioPivoteo).count();

    auto tFinTotal = chrono::high_resolution_clock::now();
    metricas.tiempoTotalMs = chrono::duration<double, milli>(tFinTotal - tInicioTotal).count();

    int cubiertos = 0;
    for (bool b : puntoCubierto)
        if (b) cubiertos++;

    metricas.numTriangulos = (int)malla.size();
    metricas.numRegiones = regiones;
    metricas.puntosCubiertos = cubiertos;

    return metricas;
}

bool guardarOBJ(const string& nombre)
{
    ofstream archivo(nombre.c_str());
    if (!archivo)
        return false;

    archivo << "# Stanford Bunny reconstruido con BPA (Benchmark Linux)\n";
    archivo << "# Vertices: " << puntos.size() << "\n";
    archivo << "# Triangulos: " << malla.size() << "\n\n";

    for (const Punto& p : puntos)
        archivo << "v " << p.x << " " << p.y << " " << p.z << "\n";
    archivo << "\n";

    for (const Triangulo& t : malla)
        archivo << "f " << t.a + 1 << " " << t.b + 1 << " " << t.c + 1 << "\n";

    return true;
}

int main(int argc, char** argv)
{
    cout << "================================================================" << endl;
    cout << "  LABORATORIO BALL PIVOTING - MEDICION DE TIEMPOS CPU (LINUX)   " << endl;
    cout << "  Punto 5: Medición para 2503 y 20 000 puntos                   " << endl;
    cout << "================================================================" << endl;

    vector<string> archivos;
    float radio = 0.007f;

    if (argc > 1)
    {
        string arg1 = argv[1];
        if (arg1 == "--all" || arg1 == "-a")
        {
            archivos.push_back("Stanford_Bunny_2503.xyz");
            archivos.push_back("Stanford_Bunny_20000.xyz");
            archivos.push_back("Stanford_Bunny_50000.xyz");
        }
        else
        {
            archivos.push_back(arg1);
            if (argc > 2)
                radio = stof(argv[2]);
        }
    }
    else
    {
        // Por defecto ejecuta los dos casos pedidos en el Punto 5
        archivos.push_back("Stanford_Bunny_2503.xyz");
        archivos.push_back("Stanford_Bunny_20000.xyz");
    }

    vector<MetricasBPA> resultados;

    for (const string& ruta : archivos)
    {
        cout << "\n------------------------------------------------------------" << endl;
        cout << "Procesando archivo: " << ruta << endl;

        if (!cargarXYZ(ruta))
        {
            cerr << "ERROR: No se pudo abrir el archivo: " << ruta << endl;
            cerr << "Asegúrese de estar en la carpeta donde residen los archivos .xyz" << endl;
            continue;
        }

        cout << "Puntos cargados con éxito: " << puntos.size() << endl;
        cout << "Radio BPA seleccionado: " << radio << endl;
        cout << "Iniciando medición con std::chrono (high_resolution_clock)..." << endl;

        MetricasBPA m = medirGeneracionBPA(radio, true);
        resultados.push_back(m);

        cout << "\nResultados para " << m.numPuntos << " puntos:" << endl;
        cout << "  - Triángulos generados  : " << m.numTriangulos << endl;
        cout << "  - Regiones multi-semilla: " << m.numRegiones << endl;
        cout << "  - Puntos cubiertos      : " << m.puntosCubiertos << " / " << m.numPuntos << endl;
        cout << "  - Tiempo Vecindad (CPU) : " << fixed << setprecision(2) << m.tiempoVecindadMs << " ms" << endl;
        cout << "  - Tiempo Pivoteo (CPU)  : " << fixed << setprecision(2) << m.tiempoPivoteoMs << " ms" << endl;
        cout << "  - TIEMPO TOTAL CPU (ms) : " << fixed << setprecision(2) << m.tiempoTotalMs << " ms" << endl;
        cout << "  - TIEMPO TOTAL CPU (s)  : " << fixed << setprecision(4) << (m.tiempoTotalMs / 1000.0) << " s" << endl;

        string salidaObj = "Bunny_" + to_string(m.numPuntos) + "_reconstruido.obj";
        if (guardarOBJ(salidaObj))
        {
            cout << "  [OBJ] Malla guardada en: " << salidaObj << endl;
        }
    }

    if (resultados.size() >= 2)
    {
        cout << "\n================================================================" << endl;
        cout << "             TABLA RESUMEN - COMPARATIVA DE TIEMPOS            " << endl;
        cout << "================================================================" << endl;
        cout << left << setw(12) << "Puntos"
             << setw(10) << "Radio"
             << setw(14) << "Triangulos"
             << setw(18) << "T. Vecindad(s)"
             << setw(18) << "T. Pivoteo(s)"
             << setw(16) << "T. TOTAL (s)" << endl;
        cout << "----------------------------------------------------------------" << endl;

        for (const auto& r : resultados)
        {
            cout << left << setw(12) << r.numPuntos
                 << setw(10) << r.radio
                 << setw(14) << r.numTriangulos
                 << setw(18) << fixed << setprecision(3) << (r.tiempoVecindadMs / 1000.0)
                 << setw(18) << fixed << setprecision(3) << (r.tiempoPivoteoMs / 1000.0)
                 << setw(16) << fixed << setprecision(3) << (r.tiempoTotalMs / 1000.0)
                 << endl;
        }
        cout << "================================================================" << endl;

        double factorPuntos = (double)resultados[1].numPuntos / resultados[0].numPuntos;
        double factorTiempo = resultados[1].tiempoTotalMs / resultados[0].tiempoTotalMs;
        cout << "\nANÁLISIS DE CRECIMIENTO:" << endl;
        cout << "  - Factor de aumento en puntos : " << fixed << setprecision(2) << factorPuntos << "x" << endl;
        cout << "  - Factor de aumento en tiempo : " << fixed << setprecision(2) << factorTiempo << "x" << endl;
        cout << "  (Nótese el crecimiento no lineal debido a las búsquedas cuadráticas O(N^2))" << endl;
    }

    return 0;
}
