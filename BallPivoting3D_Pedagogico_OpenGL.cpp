#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const float PI = 3.14159265358979323846f;
const float EPS = 0.000001f;
const float RADIO_ESFERA = 1.60f;

struct Punto
{
    float x, y, z;
};

struct Triangulo
{
    int a, b, c;
};

vector<Punto> puntos = {
    { 0.00f,  0.00f,  1.00f},   // P0: punto elevado
    {-1.50f,  0.00f,  0.00f},   // P1
    {-1.06f, -1.06f,  0.00f},   // P2
    { 0.00f, -1.50f,  0.00f},   // P3
    { 1.06f, -1.06f,  0.00f},   // P4
    { 1.50f,  0.00f,  0.00f},   // P5
    { 1.06f,  1.06f,  0.00f},   // P6
    { 0.00f,  1.50f,  0.00f},   // P7
    {-1.06f,  1.06f,  0.00f}    // P8
};

vector<Triangulo> malla;

float rotX = 25.0f;
float rotY = -30.0f;

bool arrastrando = false;
int mouseX = 0;
int mouseY = 0;

Punto centroEsferaActual = {0,0,0};
bool mostrarEsfera = false;


// ------------------------------------------------------------
// Operaciones vectoriales
// ------------------------------------------------------------

Punto restar(const Punto& A, const Punto& B)
{
    return {A.x - B.x, A.y - B.y, A.z - B.z};
}

Punto sumar(const Punto& A, const Punto& B)
{
    return {A.x + B.x, A.y + B.y, A.z + B.z};
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
    return sqrt(productoPunto(A, A));
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
    return norma(restar(A, B));
}


// ------------------------------------------------------------
// Calcula los dos centros de una esfera de radio R que pasa
// por tres puntos A, B y C.
// ------------------------------------------------------------

bool centrosEsfera(const Punto& A, const Punto& B, const Punto& C,
                   float radio, Punto& S1, Punto& S2)
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

    O.x = A.x + (uu*vxw.x + vv*wxu.x)/(2.0f*ww);
    O.y = A.y + (uu*vxw.y + vv*wxu.y)/(2.0f*ww);
    O.z = A.z + (uu*vxw.z + vv*wxu.z)/(2.0f*ww);

    float Rc = distancia(O, A);

    if (Rc > radio)
        return false;

    float h = sqrt(radio*radio - Rc*Rc);
    Punto n = normalizar(w);

    S1 = sumar(O, multiplicar(n, h));
    S2 = sumar(O, multiplicar(n, -h));

    return true;
}


// ------------------------------------------------------------
// Escoge uno de los dos centros.
// Para visualizar la esfera exterior escogemos el de mayor z.
// ------------------------------------------------------------

bool calcularCentroEsfera(const Triangulo& t, Punto& centro)
{
    Punto S1, S2;

    if (!centrosEsfera(
        puntos[t.a],
        puntos[t.b],
        puntos[t.c],
        RADIO_ESFERA,
        S1,
        S2))
    {
        return false;
    }

    centro = (S1.z > S2.z) ? S1 : S2;

    return true;
}


// ------------------------------------------------------------
// Agrega un triangulo por cada pulsacion de ESPACIO.
// P0 es el punto central elevado.
// ------------------------------------------------------------

void avanzarPaso()
{
    if ((int)malla.size() >= 8)
    {
        cout << "Malla completa." << endl;
        return;
    }

    int i = (int)malla.size() + 1;
    int siguiente = (i == 8) ? 1 : i + 1;

    Triangulo t = {0, i, siguiente};

    malla.push_back(t);

    mostrarEsfera = calcularCentroEsfera(t, centroEsferaActual);

    cout << "Triangulo T" << malla.size()-1
         << " = (P" << t.a
         << ", P" << t.b
         << ", P" << t.c << ")" << endl;

    glutPostRedisplay();
}


// ------------------------------------------------------------
// Reiniciar
// ------------------------------------------------------------

void reiniciar()
{
    malla.clear();
    mostrarEsfera = false;

    cout << endl;
    cout << "Ejemplo 3D reiniciado." << endl;
    cout << "ESPACIO: agregar triangulo" << endl;
    cout << "Mouse o flechas: rotar escena" << endl;

    glutPostRedisplay();
}


// ------------------------------------------------------------
// Dibujar texto 2D sobre la ventana
// ------------------------------------------------------------

void dibujarTexto(float x, float y, const string& texto)
{
    glRasterPos2f(x, y);

    for (char c : texto)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
}


// ------------------------------------------------------------
// Dibujar ejes
// ------------------------------------------------------------

void dibujarEjes()
{
    glLineWidth(2.0f);

    glBegin(GL_LINES);

    glColor3f(0.8f, 0.1f, 0.1f);
    glVertex3f(0,0,0);
    glVertex3f(2.2f,0,0);

    glColor3f(0.1f, 0.6f, 0.1f);
    glVertex3f(0,0,0);
    glVertex3f(0,2.2f,0);

    glColor3f(0.1f, 0.2f, 0.8f);
    glVertex3f(0,0,0);
    glVertex3f(0,0,2.2f);

    glEnd();
}


// ------------------------------------------------------------
// Dibujar nube de puntos
// ------------------------------------------------------------

void dibujarPuntos()
{
    glPointSize(9.0f);
    glColor3f(0.05f, 0.05f, 0.05f);

    glBegin(GL_POINTS);

    for (const Punto& p : puntos)
        glVertex3f(p.x, p.y, p.z);

    glEnd();
}


// ------------------------------------------------------------
// Dibujar malla como alambre
// ------------------------------------------------------------

void dibujarMalla()
{
    glLineWidth(2.5f);

    for (int i = 0; i < (int)malla.size(); i++)
    {
        const Triangulo& t = malla[i];

        if (i == (int)malla.size()-1)
            glColor3f(0.9f, 0.15f, 0.15f);
        else
            glColor3f(0.1f, 0.3f, 0.9f);

        glBegin(GL_LINE_LOOP);

        glVertex3f(puntos[t.a].x, puntos[t.a].y, puntos[t.a].z);
        glVertex3f(puntos[t.b].x, puntos[t.b].y, puntos[t.b].z);
        glVertex3f(puntos[t.c].x, puntos[t.c].y, puntos[t.c].z);

        glEnd();
    }
}


// ------------------------------------------------------------
// Dibujar esfera alambrada del ultimo triangulo
// ------------------------------------------------------------

void dibujarEsfera()
{
    if (!mostrarEsfera)
        return;

    glPushMatrix();

    glTranslatef(
        centroEsferaActual.x,
        centroEsferaActual.y,
        centroEsferaActual.z
    );

    glColor3f(0.1f, 0.65f, 0.2f);
    glLineWidth(1.0f);

    glutWireSphere(
        RADIO_ESFERA,
        18,
        14
    );

    glPopMatrix();

    glPointSize(6.0f);
    glBegin(GL_POINTS);
    glVertex3f(
        centroEsferaActual.x,
        centroEsferaActual.y,
        centroEsferaActual.z
    );
    glEnd();
}


// ------------------------------------------------------------
// Dibujar escena 3D
// ------------------------------------------------------------

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0.0f, 0.0f, -7.0f);

    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    dibujarEjes();
    dibujarMalla();
    dibujarEsfera();
    dibujarPuntos();

    // Texto en coordenadas de pantalla
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3f(0,0,0);

    dibujarTexto(
        15,
        670,
        "ESPACIO: nuevo triangulo   R: reiniciar   Flechas/Mouse: rotar   ESC: salir"
    );

    stringstream ss;
    ss << "Triangulos generados: " << malla.size() << " / 8";

    dibujarTexto(15, 645, ss.str());

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}


// ------------------------------------------------------------
// Teclado normal
// ------------------------------------------------------------

void teclado(unsigned char tecla, int, int)
{
    if (tecla == ' ')
        avanzarPaso();

    if (tecla == 'r' || tecla == 'R')
        reiniciar();

    if (tecla == 27)
        exit(0);
}


// ------------------------------------------------------------
// Flechas para rotar
// ------------------------------------------------------------

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


// ------------------------------------------------------------
// Rotacion con mouse
// ------------------------------------------------------------

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

    rotY += (x - mouseX) * 0.5f;
    rotX += (y - mouseY) * 0.5f;

    mouseX = x;
    mouseY = y;

    glutPostRedisplay();
}


// ------------------------------------------------------------
// Configuracion de proyeccion
// ------------------------------------------------------------

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0, 0, w, h);

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


// ------------------------------------------------------------
// Programa principal
// ------------------------------------------------------------

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(800, 700);

    glutCreateWindow(
        "Ball Pivoting 3D - Ejemplo pedagogico"
    );

    glClearColor(
        1.0f,
        1.0f,
        1.0f,
        1.0f
    );

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(teclasEspeciales);
    glutMouseFunc(mouse);
    glutMotionFunc(movimientoMouse);
    glutReshapeFunc(reshape);

    reiniciar();

    glutMainLoop();

    return 0;
}
