// solar_shiny.cpp
// Fast & shiny 3D Solar System (7 planets) - fixed-function OpenGL (freeglut)
// Compile (MinGW / Windows): g++ solar_shiny.cpp -I"path\to\freeglut\include" -L"path\to\freeglut\lib" -lfreeglut -lopengl32 -lglu32 -o solar.exe

#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>

// Window
int winW = 1100, winH = 700;

// Camera & view control
float camDistance = 60.0f;
float camAngleX = 20.0f; // pitch
float camAngleY = -25.0f; // yaw

// Animation control
bool paused = false;
float globalSpeed = 1.5f; // larger default multiplier for faster motion

// Planet struct
struct Planet {
    std::string name;
    float radius;        // sphere radius
    float orbitRadius;   // distance from Sun
    float baseOrbitSpeed;// base degrees per second (scaled)
    float orbitAngle;    // current angle in degrees
    float spinSpeed;     // deg/s for self-rotation
    float spinAngle;     // current spin
    float r,g,b;         // color
    float specular[4];   // specular color
    float shininess;     // material shininess
    float tilt;          // axis tilt
};

// Planets list
std::vector<Planet> planets;

// Time tracking
int lastTime = 0;

// Orbital ring geometry
void drawOrbit(float radius) {
    const int segments = 200;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        glVertex3f(x, 0.0f, z);
    }
    glEnd();
}

void initPlanets() {
    planets.clear();
    // Radii/orbits chosen for pleasing layout; speeds increased for lively motion.
    planets.push_back({"Mercury", 0.6f, 6.0f, 120.0f, 0.0f, 240.0f, 0.0f, 0.7f,0.7f,0.7f, {0.8f,0.8f,0.8f,1.0f}, 60.0f, 1.0f});
    planets.push_back({"Venus",   0.95f, 8.5f, 80.0f,  0.0f, -40.0f, 0.0f, 0.9f,0.6f,0.3f, {0.9f,0.7f,0.5f,1.0f}, 50.0f, 2.0f});
    planets.push_back({"Earth",   1.1f, 11.0f, 60.0f,  0.0f, 360.0f, 0.0f, 0.2f,0.45f,0.9f, {0.9f,0.9f,1.0f,1.0f}, 80.0f, 0.0f});
    planets.push_back({"Mars",    0.85f, 14.0f, 48.0f,  0.0f, 220.0f, 0.0f, 0.9f,0.35f,0.25f, {1.0f,0.7f,0.6f,1.0f}, 40.0f, 1.5f});
    planets.push_back({"Jupiter", 2.6f, 18.5f, 26.0f,  0.0f, 100.0f, 0.0f, 0.95f,0.75f,0.45f, {1.0f,0.9f,0.8f,1.0f}, 30.0f, 1.0f});
    planets.push_back({"Saturn",  2.2f, 24.0f, 16.0f,  0.0f, 60.0f, 0.0f, 0.95f,0.85f,0.55f, {1.0f,1.0f,0.9f,1.0f}, 25.0f, 2.5f});
    planets.push_back({"Uranus",  1.7f, 30.0f, 10.0f,  0.0f, -80.0f, 0.0f, 0.6f,0.9f,0.95f, {0.8f,0.95f,1.0f,1.0f}, 60.0f, 0.0f});
}

// {"Mercury",        // 1: name
//  0.6f,             // 2: radius of the planet (size)
//  6.0f,             // 3: orbit radius (distance from Sun)
//  120.0f,           // 4: orbit speed (degrees per second)
//  0.0f,             // 5: starting orbit angle
//  240.0f,           // 6: spin speed (rotation around itself)
//  0.0f,             // 7: starting spin angle
//  0.7f, 0.7f, 0.7f, // 8–10: color (R,G,B)
//  {0.8f,0.8f,0.8f,1.0f},  // 11: specular highlight color (RGBA)
//  60.0f,            // 12: shininess value (glossiness)
//  1.0f              // 13: tilt (planet axis tilt)
// }


// Lighting & materials
void setupLights() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Sun is the main positional light
    GLfloat lightPos[] = {0.0f, 0.0f, 0.0f, 1.0f}; // at Sun
    GLfloat lightAmbient[]  = {0.03f, 0.03f, 0.03f, 1.0f};
    GLfloat lightDiffuse[]  = {1.0f, 0.98f, 0.9f, 1.0f};
    GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // Slight attenuation so distant planets are dimmer (cosmetic)
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.3f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.01f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.001f);

    // Material defaults & use glColor for ambient+diffuse
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Strong specular reflection allowed on materials
    GLfloat defaultSpec[] = {0.7f, 0.7f, 0.7f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, defaultSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);
}

// Draw the Sun with emissive color and halo
void drawSun() {
    // Emissive material for Sun
    GLfloat sunEmission[] = {1.0f, 0.9f, 0.25f, 1.0f};
    GLfloat noEmission[]  = {0.0f, 0.0f, 0.0f, 1.0f};

    // Save & set emission
    GLfloat matAmb[4] = {1.0f,0.85f,0.2f,1.0f};
    GLfloat matSpec[4] = {1.0f,1.0f,0.8f,1.0f};
    glPushMatrix();
      // make Sun slightly rotate for visual interest
      glRotatef((float)glutGet(GLUT_ELAPSED_TIME) * 0.01f, 0.0f, 1.0f, 0.0f);

      // Emission + diffuse color
      glMaterialfv(GL_FRONT, GL_EMISSION, sunEmission);
      glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
      glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matAmb);

      glDisable(GL_LIGHTING); // draw a bright sphere and overlay, but still keep the GL light updated
      glColor3f(1.0f, 0.9f, 0.25f);
      glutSolidSphere(4.0f, 48, 48);
      glEnable(GL_LIGHTING);

      // Reset emission for subsequent objects
      glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
    glPopMatrix();

    // Add a blended halo/glow (screen-space-ish)
    glPushMatrix();
      glDisable(GL_LIGHTING);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
      glColor4f(1.0f, 0.85f, 0.25f, 0.25f);
      const int haloSegments = 40;
      glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0,0,0);
        for (int i = 0; i <= haloSegments; ++i) {
          float a = (2.0f * M_PI * i) / haloSegments;
          float r = 7.0f;
          glVertex3f(r * cosf(a), 0.0f, r * sinf(a));
        }
      glEnd();
      // larger faint ring
      glColor4f(1.0f, 0.85f, 0.25f, 0.08f);
      glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0,0,0);
        for (int i = 0; i <= haloSegments; ++i) {
          float a = (2.0f * M_PI * i) / haloSegments;
          float r = 14.0f;
          glVertex3f(r * cosf(a), 0.0f, r * sinf(a));
        }
      glEnd();
      glDisable(GL_BLEND);
      glEnable(GL_LIGHTING);
    glPopMatrix();

    // Reposition light (in case transforms changed)
    GLfloat lightPos[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
}

// Draw a planet with shiny material and spin
void drawPlanet(const Planet &p) {
    glPushMatrix();

    // Orbit translation
    float angRad = p.orbitAngle * (M_PI / 180.0f);
    float x = p.orbitRadius * cosf(angRad);
    float z = p.orbitRadius * sinf(angRad);
    glTranslatef(x, 0.0f, z);

    // Apply axis tilt
    glRotatef(p.tilt, 0.0f, 0.0f, 1.0f);

    // Self spin
    glRotatef(p.spinAngle, 0.0f, 1.0f, 0.0f);

    // Material: set color, specular and shininess
    GLfloat spec[4] = {p.specular[0], p.specular[1], p.specular[2], p.specular[3]};
    glColor3f(p.r, p.g, p.b);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, p.shininess);

    // Draw sphere (normalize normals to keep lighting correct after scaling)
    glutSolidSphere(p.radius, 36, 36);

    // Optional: tiny atmosphere highlight (subtle)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(p.r + 0.12f, p.g + 0.12f, p.b + 0.18f, 0.06f);
    glutSolidSphere(p.radius * 1.03f, 24, 24);
    glDisable(GL_BLEND);

    glPopMatrix();
}

void displayHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
      glLoadIdentity();
      gluOrtho2D(0, winW, 0, winH);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glColor3f(1,1,1);
        std::string info = "Controls: Arrows rotate | z/x zoom | +/- speed | p pause | Esc quit";
        glRasterPos2i(10, winH - 18);
        for (char c : info) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

        std::string state = " Speed: " + std::to_string((int)(globalSpeed*100)) + "% ";
        state += paused ? " PAUSED " : "";
        glRasterPos2i(10, winH - 34);
        for (char c : state) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        glEnable(GL_LIGHTING);
      glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera transform
    glTranslatef(0.0f, -2.0f, -camDistance);
    glRotatef(camAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(camAngleY, 0.0f, 1.0f, 0.0f);

    // Draw coordinate helper (optional)
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
      glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(6,0,0);
      glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,6,0);
      glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,6);
    glEnd();
    glEnable(GL_LIGHTING);

    // Sun
    drawSun();

    // Draw orbits (thin dashed feel)
    glDisable(GL_LIGHTING);
    glColor3f(0.65f, 0.65f, 0.65f);
    for (const auto &p : planets) drawOrbit(p.orbitRadius);
    glEnable(GL_LIGHTING);

    // Draw planets
    for (const auto &p : planets) drawPlanet(p);

    // If Saturn-like, draw rings around Saturn (second-last planet in our list)
    if (planets.size() >= 6) {
        const Planet &sat = planets[5];
        glPushMatrix();
          float angRad = sat.orbitAngle * (M_PI / 180.0f);
          float sx = sat.orbitRadius * cosf(angRad);
          float sz = sat.orbitRadius * sinf(angRad);
          glTranslatef(sx, 0.0f, sz);
          glRotatef(sat.tilt, 0.0f, 0.0f, 1.0f);
          glRotatef(70.0f, 1.0f, 0.0f, 0.0f);
          glDisable(GL_LIGHTING);
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          // ring geometry
          int seg = 80;
          float inner = sat.radius * 1.4f;
          float outer = sat.radius * 2.8f;
          glColor4f(0.95f, 0.85f, 0.6f, 0.6f);
          glBegin(GL_TRIANGLE_STRIP);
            for (int i=0;i<=seg;i++){
              float a = (2.0f * M_PI * i)/seg;
              float cx = cosf(a);
              float cy = sinf(a);
              glVertex3f(inner * cx, 0.0f, inner * cy);
              glVertex3f(outer * cx, 0.0f, outer * cy);
            }
          glEnd();
          glDisable(GL_BLEND);
          glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    // HUD & swap
    displayHUD();
    glutSwapBuffers();
}

// Update animation
void idleFunc() {
    int now = glutGet(GLUT_ELAPSED_TIME); // ms
    if (lastTime == 0) lastTime = now;
    int dt = now - lastTime; // milliseconds
    lastTime = now;

    if (!paused) {
        for (auto &p : planets) {
            // orbit: baseOrbitSpeed is degrees per second -> deg per ms
            float degPerMs = p.baseOrbitSpeed / 1000.0f;
            p.orbitAngle += degPerMs * dt * globalSpeed;
            if (p.orbitAngle >= 360.0f) p.orbitAngle = fmod(p.orbitAngle, 360.0f);

            // spin (self-rotation)
            float spinDegPerMs = p.spinSpeed / 1000.0f;
            p.spinAngle += spinDegPerMs * dt * globalSpeed;
            if (p.spinAngle >= 360.0f) p.spinAngle = fmod(p.spinAngle, 360.0f);
        }
    }

    glutPostRedisplay();
}

// Resize callback
void reshape(int w, int h) {
    winW = w; winH = h;
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;
    glViewport(0,0,w,h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.1f, 500.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Keyboard
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: exit(0); break;
        case 'p': case 'P': paused = !paused; break;
        case '+': globalSpeed *= 1.2f; if (globalSpeed > 40.0f) globalSpeed = 40.0f; break;
        case '-': globalSpeed /= 1.2f; if (globalSpeed < 0.01f) globalSpeed = 0.01f; break;
        case 'z': case 'Z': camDistance -= 4.0f; if (camDistance < 10.0f) camDistance = 10.0f; break;
        case 'x': case 'X': camDistance += 4.0f; if (camDistance > 400.0f) camDistance = 400.0f; break;
    }
    glutPostRedisplay();
}

// Special keys
void specialKeys(int key, int x, int y) {
    const float step = 4.0f;
    switch (key) {
        case GLUT_KEY_LEFT: camAngleY -= step; break;
        case GLUT_KEY_RIGHT: camAngleY += step; break;
        case GLUT_KEY_UP: camAngleX -= step; if (camAngleX < -90) camAngleX = -90; break;
        case GLUT_KEY_DOWN: camAngleX += step; if (camAngleX > 90) camAngleX = 90; break;
    }
    glutPostRedisplay();
}

// OpenGL initialization
void initGL() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE); // keep normals correct after scaling transforms
    setupLights();

    glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(winW, winH);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("Navneet_yadav");

    initPlanets();
    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idleFunc);

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutMainLoop();
    return 0;
}
