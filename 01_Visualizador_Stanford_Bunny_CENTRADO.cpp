#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Punto
{
    float x, y, z;
};

struct Triangulo
{
    int a, b, c;
};

vector<Punto> puntos;
vector<Triangulo> triangulos;

Punto centroModelo = {0,0,0};

float rotX = 15.0f;
float rotY = -25.0f;
float escala = 12.0f;

bool arrastrando = false;
int mouseX = 0;
int mouseY = 0;

int modo = 0;
// 0 = puntos
// 1 = malla alambrada
// 2 = malla solida

bool cargarOBJ(const string& nombre)
{
    ifstream archivo(nombre.c_str());

    if (!archivo)
        return false;

    puntos.clear();
    triangulos.clear();

    string linea;

    while (getline(archivo, linea))
    {
        if (linea.size() < 2)
            continue;

        if (linea[0] == 'v' && linea[1] == ' ')
        {
            stringstream ss(linea);
            char v;
            Punto p;

            ss >> v >> p.x >> p.y >> p.z;
            puntos.push_back(p);
        }
        else if (linea[0] == 'f' && linea[1] == ' ')
        {
            stringstream ss(linea);
            char f;
            string sa, sb, sc;

            ss >> f >> sa >> sb >> sc;

            int a = atoi(sa.c_str());
            int b = atoi(sb.c_str());
            int c = atoi(sc.c_str());

            triangulos.push_back({a-1, b-1, c-1});
        }
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

void dibujarPuntos()
{
    glPointSize(2.0f);
    glColor3f(0.05f, 0.05f, 0.05f);

    glBegin(GL_POINTS);

    for (const Punto& p : puntos)
        glVertex3f(p.x, p.y, p.z);

    glEnd();
}

void dibujarMallaAlambrada()
{
    glColor3f(0.1f, 0.25f, 0.8f);
    glLineWidth(1.0f);

    for (const Triangulo& t : triangulos)
    {
        glBegin(GL_LINE_LOOP);

        glVertex3f(puntos[t.a].x, puntos[t.a].y, puntos[t.a].z);
        glVertex3f(puntos[t.b].x, puntos[t.b].y, puntos[t.b].z);
        glVertex3f(puntos[t.c].x, puntos[t.c].y, puntos[t.c].z);

        glEnd();
    }
}

void dibujarMallaSolida()
{
    glColor3f(0.75f, 0.75f, 0.78f);

    glBegin(GL_TRIANGLES);

    for (const Triangulo& t : triangulos)
    {
        const Punto& A = puntos[t.a];
        const Punto& B = puntos[t.b];
        const Punto& C = puntos[t.c];

        float ux = B.x - A.x;
        float uy = B.y - A.y;
        float uz = B.z - A.z;

        float vx = C.x - A.x;
        float vy = C.y - A.y;
        float vz = C.z - A.z;

        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;

        float n = sqrt(nx*nx + ny*ny + nz*nz);

        if (n > 0.000001f)
        {
            nx /= n;
            ny /= n;
            nz /= n;
        }

        glNormal3f(nx, ny, nz);

        glVertex3f(A.x, A.y, A.z);
        glVertex3f(B.x, B.y, B.z);
        glVertex3f(C.x, C.y, C.z);
    }

    glEnd();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    else if (modo == 1)
        dibujarMallaAlambrada();
    else
        dibujarMallaSolida();

    glutSwapBuffers();
}

void teclado(unsigned char tecla, int, int)
{
    if (tecla == 'p' || tecla == 'P')
        modo = 0;

    if (tecla == 'w' || tecla == 'W')
        modo = 1;

    if (tecla == 'f' || tecla == 'F')
        modo = 2;

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

void configurarIluminacion()
{
    GLfloat posicion[] = {2.0f, 4.0f, 3.0f, 1.0f};
    GLfloat ambiente[] = {0.25f, 0.25f, 0.25f, 1.0f};
    GLfloat difusa[] = {0.85f, 0.85f, 0.85f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, posicion);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, difusa);

    glEnable(GL_LIGHT0);
}

int main(int argc, char** argv)
{
    string archivo = "Stanford_Bunny_2503.obj";

    if (argc > 1)
        archivo = argv[1];

    if (!cargarOBJ(archivo))
    {
        cerr << "No se pudo cargar: " << archivo << endl;
        return 1;
    }

    calcularCentroModelo();

    cout << "Stanford Bunny cargado." << endl;
    cout << "Vertices: " << puntos.size() << endl;
    cout << "Triangulos: " << triangulos.size() << endl;
    cout << endl;
    cout << "P = nube de puntos" << endl;
    cout << "W = malla alambrada" << endl;
    cout << "F = malla solida" << endl;
    cout << "Mouse/Flechas = rotar" << endl;
    cout << "+/- = zoom" << endl;
    cout << "R = reiniciar vista" << endl;

    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(900, 700);
    glutCreateWindow("Stanford Bunny - OpenGL");

    glClearColor(1, 1, 1, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    configurarIluminacion();

    glutDisplayFunc(display);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(teclasEspeciales);
    glutMouseFunc(mouse);
    glutMotionFunc(movimientoMouse);
    glutReshapeFunc(reshape);

    glutMainLoop();

    return 0;
}
