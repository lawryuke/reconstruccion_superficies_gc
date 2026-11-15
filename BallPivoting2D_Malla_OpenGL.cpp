// ==============================================================================
// UNIVERSIDAD NACIONAL DE SAN AGUSTIN
// ESCUELA PROFESIONAL DE CIENCIA DE LA COMPUTACION
// CURSO: Computacion Grafica - Grupo A
// DOCENTE: Gutierrez Caceres Juan Carlos
// 
// INTEGRANTES:
//   - Miramira Bellido Rimsky Augusto
//   - Machaca Muniz Jose Alejandro
// 
// PRACTICA: Reconstruccion de superficies con Ball Pivoting
// ARCHIVO: BallPivoting2D_Malla_OpenGL.cpp
// ==============================================================================

#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const float PI = 3.14159265358979323846f;
const float EPS = 0.0001f;

struct Punto {
    float x, y;
};

struct Triangulo {
    int a, b, c;
};

vector<Punto> puntos = {
    {210, 190}, // P0: punto central
    {100, 100}, // P1
    {200, 80},  // P2
    {310, 110}, // P3
    {340, 210}, // P4
    {280, 310}, // P5
    {170, 320}, // P6
    {80, 250},  // P7
    {70, 160}   // P8
};

vector<Triangulo> malla;
vector<bool> usado;

const int CENTRO = 0;

int actual = 1;
int primero = 1;

Punto centroCirculoActual = {0, 0};
float radioCirculoActual = 0.0f;

bool terminado = false;

// ------------------------------------------------------------
// Operaciones geometricas
// ------------------------------------------------------------

float distancia(const Punto &A, const Punto &B) {
    float dx = A.x - B.x;
    float dy = A.y - B.y;
    return sqrt(dx * dx + dy * dy);
}

float anguloDesdeCentro(const Punto &P) {
    const Punto &O = puntos[CENTRO];
    return atan2(P.y - O.y, P.x - O.x);
}

float anguloPositivo(float a) {
    while (a <= EPS)
        a += 2.0f * PI;

    while (a > 2.0f * PI)
        a -= 2.0f * PI;

    return a;
}

// ------------------------------------------------------------
// Circuncirculo de tres puntos
// ------------------------------------------------------------

bool calcularCircuncirculo(const Punto &A, const Punto &B, const Punto &C,
                           Punto &centro, float &radio) {
    float d =
        2.0f * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));

    if (fabs(d) < EPS)
        return false;

    float a2 = A.x * A.x + A.y * A.y;
    float b2 = B.x * B.x + B.y * B.y;
    float c2 = C.x * C.x + C.y * C.y;

    centro.x = (a2 * (B.y - C.y) + b2 * (C.y - A.y) + c2 * (A.y - B.y)) / d;

    centro.y = (a2 * (C.x - B.x) + b2 * (A.x - C.x) + c2 * (B.x - A.x)) / d;

    radio = distancia(centro, A);

    return true;
}

// ------------------------------------------------------------
// Busca el siguiente punto del contorno.
// Version pedagogica:
// elige el punto no usado que produce el menor giro positivo
// alrededor del punto central.
// ------------------------------------------------------------

int buscarSiguiente() {
    float anguloActual = anguloDesdeCentro(puntos[actual]);

    float mejorGiro = 1e30f;
    int mejor = -1;

    for (int i = 1; i < (int)puntos.size(); i++) {
        if (i == actual)
            continue;

        if (usado[i])
            continue;

        float anguloCandidato = anguloDesdeCentro(puntos[i]);

        float giro = anguloPositivo(anguloCandidato - anguloActual);

        if (giro < mejorGiro) {
            mejorGiro = giro;
            mejor = i;
        }
    }

    // Si ya usamos todos los puntos, cerramos con el primero.
    if (mejor == -1 && actual != primero)
        mejor = primero;

    return mejor;
}

// ------------------------------------------------------------
// Agrega un triangulo a la malla.
// ------------------------------------------------------------

void avanzarPaso() {
    if (terminado)
        return;

    int siguiente = buscarSiguiente();

    if (siguiente == -1) {
        terminado = true;
        return;
    }

    Triangulo t = {CENTRO, actual, siguiente};

    malla.push_back(t);

    calcularCircuncirculo(puntos[t.a], puntos[t.b], puntos[t.c],
                          centroCirculoActual, radioCirculoActual);

    cout << "Triangulo generado: (" << t.a << ", " << t.b << ", " << t.c << ")"
         << endl;

    if (siguiente == primero) {
        terminado = true;
        cout << "Malla cerrada." << endl;
    } else {
        usado[siguiente] = true;
        actual = siguiente;
    }

    glutPostRedisplay();
}

// ------------------------------------------------------------
// Reinicia la practica.
// ------------------------------------------------------------

void reiniciar() {
    malla.clear();

    usado.assign(puntos.size(), false);

    actual = 1;
    primero = 1;

    usado[actual] = true;

    centroCirculoActual = {0, 0};
    radioCirculoActual = 0.0f;

    terminado = false;

    cout << endl;
    cout << "Practica reiniciada." << endl;
    cout << "ESPACIO = agregar un triangulo" << endl;

    glutPostRedisplay();
}

// ------------------------------------------------------------
// Dibujo OpenGL
// ------------------------------------------------------------

void dibujarTexto(float x, float y, const string &texto) {
    glRasterPos2f(x, y);

    for (char c : texto)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
}

void dibujarCirculo(const Punto &C, float r) {
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i < 100; i++) {
        float angulo = 2.0f * PI * i / 100.0f;

        glVertex2f(C.x + r * cos(angulo), C.y + r * sin(angulo));
    }

    glEnd();
}

void dibujarTriangulos() {
    glLineWidth(2.0f);

    for (int i = 0; i < (int)malla.size(); i++) {
        const Triangulo &t = malla[i];

        if (i == (int)malla.size() - 1)
            glColor3f(0.9f, 0.2f, 0.2f);
        else
            glColor3f(0.1f, 0.3f, 0.9f);

        glBegin(GL_LINE_LOOP);

        glVertex2f(puntos[t.a].x, puntos[t.a].y);
        glVertex2f(puntos[t.b].x, puntos[t.b].y);
        glVertex2f(puntos[t.c].x, puntos[t.c].y);

        glEnd();
    }
}

void dibujarPuntos() {
    glPointSize(10.0f);
    glColor3f(0.0f, 0.0f, 0.0f);

    glBegin(GL_POINTS);

    for (const Punto &p : puntos)
        glVertex2f(p.x, p.y);

    glEnd();

    for (int i = 0; i < (int)puntos.size(); i++) {
        stringstream ss;
        ss << "P" << i;

        dibujarTexto(puntos[i].x + 7.0f, puntos[i].y + 7.0f, ss.str());
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    dibujarTriangulos();

    // Circuncirculo del ultimo triangulo
    if (!malla.empty()) {
        glColor3f(0.2f, 0.65f, 0.2f);
        glLineWidth(1.5f);

        dibujarCirculo(centroCirculoActual, radioCirculoActual);

        glPointSize(5.0f);

        glBegin(GL_POINTS);
        glVertex2f(centroCirculoActual.x, centroCirculoActual.y);
        glEnd();
    }

    dibujarPuntos();

    glColor3f(0.0f, 0.0f, 0.0f);

    dibujarTexto(10, 375,
                 "ESPACIO: nuevo triangulo   R: reiniciar   ESC: salir");

    stringstream estado;

    estado << "Triangulos generados: " << malla.size();

    dibujarTexto(10, 355, estado.str());

    if (!terminado) {
        stringstream ss;

        ss << "Punto actual: P" << actual;

        dibujarTexto(10, 335, ss.str());
    } else {
        dibujarTexto(10, 335, "Malla terminada");
    }

    glutSwapBuffers();
}

// ------------------------------------------------------------
// Teclado
// ------------------------------------------------------------

void teclado(unsigned char tecla, int, int) {
    if (tecla == ' ')
        avanzarPaso();

    if (tecla == 'r' || tecla == 'R')
        reiniciar();

    if (tecla == 27)
        exit(0);
}

// ------------------------------------------------------------
// Ventana
// ------------------------------------------------------------

void reshape(int w, int h) {
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, 420, 0, 400);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// ------------------------------------------------------------
// Programa principal
// ------------------------------------------------------------

int main(int argc, char **argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    glutInitWindowSize(800, 700);

    glutCreateWindow("Ball Pivoting 2D - Malla pedagogica");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutKeyboardFunc(teclado);
    glutReshapeFunc(reshape);

    reiniciar();

    glutMainLoop();

    return 0;
}
