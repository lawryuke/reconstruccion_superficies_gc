#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

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

vector<Punto> puntos;
vector<Triangulo> malla;
vector<vector<int> > vecinosBPA;

Punto centroModelo = {0,0,0};

float radioBPA = 0.007f;

float rotX = 15.0f;
float rotY = -25.0f;
float escala = 12.0f;

bool arrastrando = false;
int mouseX = 0;
int mouseY = 0;

int modo = 1;
// 0 = puntos
// 1 = malla alambrada
// 2 = malla solida


// ============================================================
// Operaciones vectoriales
// ============================================================

Punto sumar(const Punto& A, const Punto& B)
{
    return {A.x+B.x, A.y+B.y, A.z+B.z};
}

Punto restar(const Punto& A, const Punto& B)
{
    return {A.x-B.x, A.y-B.y, A.z-B.z};
}

Punto multiplicar(const Punto& A, float k)
{
    return {A.x*k, A.y*k, A.z*k};
}

float productoPunto(const Punto& A, const Punto& B)
{
    return A.x*B.x + A.y*B.y + A.z*B.z;
}

Punto productoCruz(const Punto& A, const Punto& B)
{
    return {
        A.y*B.z - A.z*B.y,
        A.z*B.x - A.x*B.z,
        A.x*B.y - A.y*B.x
    };
}

float norma(const Punto& A)
{
    return sqrt(productoPunto(A,A));
}

Punto normalizar(const Punto& A)
{
    float n = norma(A);

    if (n < EPS)
        return {0,0,0};

    return {A.x/n, A.y/n, A.z/n};
}

float distancia(const Punto& A, const Punto& B)
{
    return norma(restar(A,B));
}


// ============================================================
// Cargar nube XYZ
// ============================================================

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



// ============================================================
// Calcular centro del modelo para visualizarlo centrado
// ============================================================

void calcularCentroModelo()
{
    if (puntos.empty())
        return;

    float minX = puntos[0].x;
    float maxX = puntos[0].x;

    float minY = puntos[0].y;
    float maxY = puntos[0].y;

    float minZ = puntos[0].z;
    float maxZ = puntos[0].z;

    for (const Punto& p : puntos)
    {
        minX = min(minX, p.x);
        maxX = max(maxX, p.x);

        minY = min(minY, p.y);
        maxY = max(maxY, p.y);

        minZ = min(minZ, p.z);
        maxZ = max(maxZ, p.z);
    }

    centroModelo.x = (minX + maxX) / 2.0f;
    centroModelo.y = (minY + maxY) / 2.0f;
    centroModelo.z = (minZ + maxZ) / 2.0f;
}

// ============================================================
// Geometria de esfera por 3 puntos
// ============================================================

bool centrosEsfera(const Punto& A,
                   const Punto& B,
                   const Punto& C,
                   float radio,
                   Punto& S1,
                   Punto& S2)
{
    Punto u = restar(B,A);
    Punto v = restar(C,A);
    Punto w = productoCruz(u,v);

    float ww = productoPunto(w,w);

    if (ww < EPS)
        return false;

    float uu = productoPunto(u,u);
    float vv = productoPunto(v,v);

    Punto vxw = productoCruz(v,w);
    Punto wxu = productoCruz(w,u);

    Punto O;

    O.x = A.x + (uu*vxw.x + vv*wxu.x)/(2.0f*ww);
    O.y = A.y + (uu*vxw.y + vv*wxu.y)/(2.0f*ww);
    O.z = A.z + (uu*vxw.z + vv*wxu.z)/(2.0f*ww);

    float Rc = distancia(O,A);

    if (Rc > radio + EPS)
        return false;

    float h2 = radio*radio - Rc*Rc;

    if (h2 < 0.0f)
        h2 = 0.0f;

    float h = sqrt(h2);

    Punto n = normalizar(w);

    S1 = sumar(O, multiplicar(n, h));
    S2 = sumar(O, multiplicar(n,-h));

    return true;
}



// ============================================================
// Construir vecindad local para acelerar BPA
// Un punto interior a una esfera que pasa por A se encuentra
// necesariamente a distancia <= 2r de A.
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

    cout << "Vecindad construida para radio "
         << radio << endl;
}

// ============================================================
// Esfera vacia
// ============================================================

bool esferaVacia(const Punto& centro,
                 float radio,
                 int ia,
                 int ib,
                 int ic)
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


// ============================================================
// Angulo de pivotamiento alrededor de una arista AB
// ============================================================

float anguloPivote(const Punto& A,
                   const Punto& B,
                   const Punto& centroActual,
                   const Punto& centroNuevo)
{
    Punto eje = normalizar(restar(B,A));

    Punto M = multiplicar(sumar(A,B), 0.5f);

    Punto u = restar(centroActual,M);
    Punto v = restar(centroNuevo,M);

    u = restar(u, multiplicar(eje, productoPunto(u,eje)));
    v = restar(v, multiplicar(eje, productoPunto(v,eje)));

    float nu = norma(u);
    float nv = norma(v);

    if (nu < EPS || nv < EPS)
        return 1e30f;

    u = multiplicar(u,1.0f/nu);
    v = multiplicar(v,1.0f/nv);

    float seno = productoPunto(eje, productoCruz(u,v));
    float coseno = productoPunto(u,v);

    float angulo = atan2(seno,coseno);

    if (angulo <= EPS)
        angulo += 2.0f*PI;

    return angulo;
}


// ============================================================
// Claves canonicas para evitar duplicados
// ============================================================

pair<int,int> claveArista(int a, int b)
{
    if (a > b)
        swap(a,b);

    return {a,b};
}

tuple<int,int,int> claveTriangulo(int a, int b, int c)
{
    int v[3] = {a,b,c};

    sort(v,v+3);

    return make_tuple(v[0],v[1],v[2]);
}


// ============================================================
// Buscar triangulo semilla
// ============================================================

bool buscarSemilla(float radio,
                   Triangulo& semilla,
                   Punto& centro)
{
    const int MAX_VECINOS = 40;

    for (int i = 0; i < (int)puntos.size(); i++)
    {
        vector<pair<float,int> > vecinos;

        for (int j : vecinosBPA[i])
        {
            vecinos.push_back({
                distancia(puntos[i], puntos[j]),
                j
            });
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

                Punto S1, S2;

                if (!centrosEsfera(
                    puntos[i],
                    puntos[j],
                    puntos[k],
                    radio,
                    S1,
                    S2))
                {
                    continue;
                }

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


// ============================================================
// Buscar siguiente punto al pivotar alrededor de AB
// ============================================================

ResultadoPivot buscarPuntoBPA(int a,
                              int b,
                              const Punto& centroActual,
                              float radio,
                              const set<tuple<int,int,int> >& triangulosUsados)
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

        if (distancia(B,puntos[i]) > 2.0f*radio)
            continue;

        if (triangulosUsados.count(claveTriangulo(a,b,i)))
            continue;

        Punto S1,S2;

        if (!centrosEsfera(
            A,
            B,
            puntos[i],
            radio,
            S1,S2))
        {
            continue;
        }

        Punto centros[2] = {S1,S2};

        for (int k = 0; k < 2; k++)
        {
            Punto S = centros[k];

            if (!esferaVacia(S,radio,a,b,i))
                continue;

            float angulo =
                anguloPivote(A,B,centroActual,S);

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


// ============================================================
// Ball Pivoting pedagogico
// ============================================================

void generarMallaBPA(float radio)
{
    malla.clear();

    cout << endl;
    cout << "Generando malla BPA..." << endl;
    cout << "Radio = " << radio << endl;

    construirVecindad(radio);

    Triangulo semilla;
    Punto centroSemilla;

    if (!buscarSemilla(
        radio,
        semilla,
        centroSemilla))
    {
        cout << "No se encontro triangulo semilla." << endl;
        cout << "Pruebe con otro radio." << endl;
        return;
    }

    cout << "Semilla encontrada: (P"
         << semilla.a << ", P"
         << semilla.b << ", P"
         << semilla.c << ")" << endl;

    queue<AristaActiva> frontera;

    set<pair<int,int> > aristasCerradas;
    set<tuple<int,int,int> > triangulosUsados;

    malla.push_back(semilla);

    triangulosUsados.insert(
        claveTriangulo(
            semilla.a,
            semilla.b,
            semilla.c
        )
    );

    frontera.push({
        semilla.a,
        semilla.b,
        centroSemilla
    });

    frontera.push({
        semilla.b,
        semilla.c,
        centroSemilla
    });

    frontera.push({
        semilla.c,
        semilla.a,
        centroSemilla
    });

    const int MAX_TRIANGULOS = 15000;

    while (!frontera.empty() &&
           (int)malla.size() < MAX_TRIANGULOS)
    {
        AristaActiva e = frontera.front();
        frontera.pop();

        pair<int,int> clave = claveArista(e.a,e.b);

        if (aristasCerradas.count(clave))
            continue;

        ResultadoPivot r =
            buscarPuntoBPA(
                e.a,
                e.b,
                e.centro,
                radio,
                triangulosUsados
            );

        if (!r.encontrado)
        {
            aristasCerradas.insert(clave);
            continue;
        }

        Triangulo t = {
            e.a,
            e.b,
            r.punto
        };

        tuple<int,int,int> kt =
            claveTriangulo(t.a,t.b,t.c);

        if (triangulosUsados.count(kt))
        {
            aristasCerradas.insert(clave);
            continue;
        }

        malla.push_back(t);
        triangulosUsados.insert(kt);

        // La arista por la que se pivoto ya tiene dos triangulos
        aristasCerradas.insert(clave);

        pair<int,int> e1 = claveArista(t.b,t.c);
        pair<int,int> e2 = claveArista(t.c,t.a);

        if (!aristasCerradas.count(e1))
        {
            frontera.push({
                t.b,
                t.c,
                r.centro
            });
        }

        if (!aristasCerradas.count(e2))
        {
            frontera.push({
                t.c,
                t.a,
                r.centro
            });
        }
    }

    cout << "Triangulos generados: "
         << malla.size() << endl;

    modo = 1;

    glutPostRedisplay();
}



// ============================================================
// Guardar malla generada en OBJ
// ============================================================

bool guardarOBJ(const string& nombre)
{
    if (malla.empty())
    {
        cout << "No hay malla para guardar." << endl;
        return false;
    }

    ofstream archivo(nombre.c_str());

    if (!archivo)
    {
        cout << "No se pudo crear: " << nombre << endl;
        return false;
    }

    archivo << "# Stanford Bunny reconstruido con BPA\n";
    archivo << "# Vertices: " << puntos.size() << "\n";
    archivo << "# Triangulos: " << malla.size() << "\n\n";

    for (const Punto& p : puntos)
        archivo << "v " << p.x << " " << p.y << " " << p.z << "\n";

    archivo << "\n";

    for (const Triangulo& t : malla)
        archivo << "f "
                << t.a + 1 << " "
                << t.b + 1 << " "
                << t.c + 1 << "\n";

    archivo.close();

    cout << "Malla guardada en: " << nombre << endl;

    return true;
}

// ============================================================
// Dibujo
// ============================================================

void dibujarPuntos()
{
    glDisable(GL_LIGHTING);

    glPointSize(2.5f);
    glColor3f(0.05f,0.05f,0.05f);

    glBegin(GL_POINTS);

    for (const Punto& p : puntos)
        glVertex3f(p.x,p.y,p.z);

    glEnd();
}

void dibujarMallaAlambrada()
{
    glDisable(GL_LIGHTING);

    glColor3f(0.1f,0.25f,0.8f);
    glLineWidth(1.0f);

    for (const Triangulo& t : malla)
    {
        glBegin(GL_LINE_LOOP);

        glVertex3f(
            puntos[t.a].x,
            puntos[t.a].y,
            puntos[t.a].z
        );

        glVertex3f(
            puntos[t.b].x,
            puntos[t.b].y,
            puntos[t.b].z
        );

        glVertex3f(
            puntos[t.c].x,
            puntos[t.c].y,
            puntos[t.c].z
        );

        glEnd();
    }
}

void dibujarMallaSolida()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glColor3f(0.75f,0.75f,0.78f);

    glBegin(GL_TRIANGLES);

    for (const Triangulo& t : malla)
    {
        const Punto& A = puntos[t.a];
        const Punto& B = puntos[t.b];
        const Punto& C = puntos[t.c];

        Punto n =
            normalizar(
                productoCruz(
                    restar(B,A),
                    restar(C,A)
                )
            );

        glNormal3f(n.x,n.y,n.z);

        glVertex3f(A.x,A.y,A.z);
        glVertex3f(B.x,B.y,B.z);
        glVertex3f(C.x,C.y,C.z);
    }

    glEnd();

    glDisable(GL_LIGHTING);
}


// ============================================================
// Interfaz OpenGL
// ============================================================

void display()
{
    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0.0f, 0.0f, -4.0f);

    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    glScalef(escala, escala, escala);

    glTranslatef(
        -centroModelo.x,
        -centroModelo.y,
        -centroModelo.z
    );

    if (modo == 0)
        dibujarPuntos();

    if (modo == 1)
        dibujarMallaAlambrada();

    if (modo == 2)
        dibujarMallaSolida();

    glutSwapBuffers();
}


// ============================================================
// Teclado
// ============================================================

void teclado(unsigned char tecla, int, int)
{
    if (tecla == 'p' || tecla == 'P')
        modo = 0;

    if (tecla == 'w' || tecla == 'W')
        modo = 1;

    if (tecla == 'f' || tecla == 'F')
        modo = 2;

    if (tecla == '1')
    {
        radioBPA = 0.005f;
        generarMallaBPA(radioBPA);
    }

    if (tecla == '2')
    {
        radioBPA = 0.007f;
        generarMallaBPA(radioBPA);
    }

    if (tecla == '3')
    {
        radioBPA = 0.010f;
        generarMallaBPA(radioBPA);
    }

    if (tecla == 'b' || tecla == 'B')
        generarMallaBPA(radioBPA);

    if (tecla == 'c' || tecla == 'C')
    {
        malla.clear();
        modo = 0;
    }

    if (tecla == 's' || tecla == 'S')
        guardarOBJ("Bunny_BPA_generado.obj");

    if (tecla == '+')
        escala *= 1.1f;

    if (tecla == '-')
        escala /= 1.1f;

    if (tecla == 'r' || tecla == 'R')
    {
        rotX = 15.0f;
        rotY = -25.0f;
        escala = 12.0f;
    }

    if (tecla == 27)
        exit(0);

    glutPostRedisplay();
}

void teclasEspeciales(int tecla, int, int)
{
    if (tecla == GLUT_KEY_LEFT)
        rotY -= 5.0f;

    if (tecla == GLUT_KEY_RIGHT)
        rotY += 5.0f;

    if (tecla == GLUT_KEY_UP)
        rotX -= 5.0f;

    if (tecla == GLUT_KEY_DOWN)
        rotX += 5.0f;

    glutPostRedisplay();
}


// ============================================================
// Mouse
// ============================================================

void mouse(int boton, int estado, int x, int y)
{
    if (boton == GLUT_LEFT_BUTTON)
    {
        if (estado == GLUT_DOWN)
        {
            arrastrando = true;
            mouseX = x;
            mouseY = y;
        }
        else
        {
            arrastrando = false;
        }
    }
}

void movimientoMouse(int x, int y)
{
    if (!arrastrando)
        return;

    rotY += (x-mouseX)*0.5f;
    rotX += (y-mouseY)*0.5f;

    mouseX = x;
    mouseY = y;

    glutPostRedisplay();
}


// ============================================================
// Proyeccion
// ============================================================

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0,0,w,h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45.0,
        (double)w/(double)h,
        0.1,
        100.0
    );

    glMatrixMode(GL_MODELVIEW);
}


// ============================================================
// Luz
// ============================================================

void configurarLuz()
{
    GLfloat posicion[] = {
        2.0f,4.0f,3.0f,1.0f
    };

    GLfloat ambiente[] = {
        0.25f,0.25f,0.25f,1.0f
    };

    GLfloat difusa[] = {
        0.85f,0.85f,0.85f,1.0f
    };

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        posicion
    );

    glLightfv(
        GL_LIGHT0,
        GL_AMBIENT,
        ambiente
    );

    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        difusa
    );
}


// ============================================================
// Main
// ============================================================

int main(int argc, char** argv)
{
    string archivo =
        "Stanford_Bunny_2503.xyz";

    if (argc > 1)
        archivo = argv[1];

    if (!cargarXYZ(archivo))
    {
        cerr << "No se pudo abrir: "
             << archivo << endl;

        return 1;
    }

    calcularCentroModelo();

    cout << "Puntos cargados: "
         << puntos.size() << endl;

    cout << endl;
    cout << "CONTROLES" << endl;
    cout << "P = puntos" << endl;
    cout << "W = malla alambrada" << endl;
    cout << "F = malla solida" << endl;
    cout << "1 = BPA radio 0.005" << endl;
    cout << "2 = BPA radio 0.007" << endl;
    cout << "3 = BPA radio 0.010" << endl;
    cout << "B = regenerar" << endl;
    cout << "C = borrar malla" << endl;
    cout << "S = guardar Bunny_BPA_generado.obj" << endl;
    cout << "Mouse/Flechas = rotar" << endl;
    cout << "+/- = zoom" << endl;
    cout << "ESC = salir" << endl;

    glutInit(&argc,argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(
        900,
        700
    );

    glutCreateWindow(
        "Stanford Bunny - Ball Pivoting"
    );

    glClearColor(
        1.0f,
        1.0f,
        1.0f,
        1.0f
    );

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    configurarLuz();

    glutDisplayFunc(display);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(teclasEspeciales);
    glutMouseFunc(mouse);
    glutMotionFunc(movimientoMouse);
    glutReshapeFunc(reshape);

    // Inicialmente solo mostramos la nube
    modo = 0;

    glutMainLoop();

    return 0;
}
