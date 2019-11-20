#include "glut.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "Texture.hpp"

Texture t;

void Reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

void Draw(void)
{
    const char pikachu[] = "Pikachu.png";
    const char earth[] = "Earth.png";
    static bool a = true;
    printf("code = %d\n", t.loadTextureFromPng((a) ? pikachu : earth, 720, 720));
    a = !a;
    static float angle = 0.0;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t.getTextureName() ); //bind the texture

    glPushMatrix();
    glRotatef(angle, 0.0f, 0.0f, 1.0f );
    glBegin( GL_QUADS );
    glTexCoord2d(0.0,0.0); glVertex2d(-1.0,-1.0);
    glTexCoord2d(1.0,0.0); glVertex2d(+1.0,-1.0);
    glTexCoord2d(1.0,1.0); glVertex2d(+1.0,+1.0);
    glTexCoord2d(0.0,1.0); glVertex2d(-1.0,+1.0);
    glEnd();
    glPopMatrix();
    glutSwapBuffers();
    angle += 0.01;
    glFlush();
    t.freeTexture();

    // glutSolidCube(2);
    // glClear(GL_COLOR_BUFFER_BIT);

    // glColor3f(0.0f, 0.0f, 1.0f);
    // glLineWidth(1);

    // glBegin(GL_LINES);
    // glVertex2f(0, 0.5f);
    // glVertex2f(0, -0.5f);
    // glEnd();
    // glFlush();  
}

int main(int argc, char** argv)
{
    //init
    {
        glutInit(&argc, argv);
        glutInitWindowSize(730, 730);
        glutInitWindowPosition(0, 0);
        // glutInitDisplayMode(GLUT_RGB);
        glutInitDisplayMode(GLUT_DOUBLE);
        glutCreateWindow("Player for chinese camera");
    }

    // printf("code = %d\n", t.loadTextureFromPng("Earth.png", 720, 720));

    int g = 0;
    while (g++ != 1)
    {
        clock_t begin = clock();
        Draw();
        clock_t end = clock();
        printf("%f\n", ((double)end - begin) / CLOCKS_PER_SEC);
        // glutReshapeFunc(Reshape);
        // glutDisplayFunc(Draw);
        glClearColor(0, 0, 0, 0);
    }

    // glutMainLoop();
    return 0;
}