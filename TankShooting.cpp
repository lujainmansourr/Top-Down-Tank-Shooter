#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <ctime>
#include <cstring>
#include <algorithm>

struct Explosion {
    float x, z;
    int startTime;
};
struct Obstacle {
    float x, z;
    float size;
    int health;
    int type;
};
struct Bullet {
    float x, z;
    float angle;
    int owner;
    int creationTime;
};
struct Tank {
    float x, z;
    float angle;
    int health;
};
struct Firework {
    float x, y, z;
    float r, g, b;
    int startTime;
};

char playerName[50] = "Player";
char enemyName[50] = "Enemy";
bool showPlayerNameEntry = false;
bool enteringPlayer = true;
bool showHowToPlay = false;

int nameCharIndex = 0;
bool showMainMenu = true;

std::vector<Explosion> explosions;
std::vector<Firework> fireworks;

bool showLevelSelection = false;
int selectedLevel = 1;
int lastAIShoot = 0;


const char* menuOptions[] = {
    "2 Players",
    "1 Player (Bot)",
    "Player Names Setup",
    "Select Level",
    "How to Play",
    "Exit"
};


const int menuCount = sizeof(menuOptions) / sizeof(menuOptions[0]);
bool playWithBot = false;

Tank menuTank = { 0, 0, 0 };
int selectedMenuIndex = 0;

void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void update(int value);
void drawText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18);


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tank player = { -10, 0, 0, 3 };
Tank enemy = { 10, 0, 180, 3 };
std::vector<Bullet> bullets;
std::vector<Obstacle> obstacles;

int level = 1, maxLevels = 5, playerWins = 0, enemyWins = 0;
bool gameOver = false, levelTransition = false;
int transitionTimer = 0;

const float tankHalfWidth = 3.0f, tankHalfDepth = 2.0f;
const int shotDelay = 300;
int lastPlayerShot = 0, lastEnemyShot = 0;


void drawHowToPlayScreen() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.1f, 0.1f, 0.1f);
    drawText("HOW TO PLAY", 100, 700);
    drawText("- Player 1: W/A/S/D to move, SPACE to shoot", 100, 640);
    drawText("- Player 2: Arrow keys to move, ENTER to shoot", 100, 580);
    drawText("- Destroy blue blocks to clear path.", 100, 520);
    drawText("- Collect yellow power-ups to gain health.", 100, 460);
    drawText("- Avoid grey blocks (indestructible).", 100, 400);
    drawText("- First tank to reduce the opponent to 0 health wins.", 100, 340);
    drawText("- Press 'M' to return to the main menu.", 100, 280);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

bool checkAABB(float x1, float z1, float hw1, float hd1, float x2, float z2, float hw2, float hd2) {
    return (fabs(x1 - x2) < (hw1 + hw2)) && (fabs(z1 - z2) < (hd1 + hd2));
}

bool isTankFullyInsideGrid(float x, float z) {
    const float arenaLimitX = 19.5f;
    const float arenaLimitZ = 19.5f;

    return (x - tankHalfWidth >= -arenaLimitX && x + tankHalfWidth <= arenaLimitX &&
        z - tankHalfDepth >= -arenaLimitZ && z + tankHalfDepth <= arenaLimitZ);
}

bool tankHitsObstacle(float x, float z) {
    for (auto& o : obstacles) {
        float ho = o.size / 2.0f;
        if (checkAABB(x, z, tankHalfWidth, tankHalfDepth, o.x, o.z, ho, ho))
            return true;
    }
    return false;
}

bool tanksCollide(float x1, float z1, float x2, float z2) {
    return checkAABB(x1, z1, tankHalfWidth, tankHalfDepth, x2, z2, tankHalfWidth, tankHalfDepth);
}

bool bulletHitsObstacle(const Bullet& b) {
    for (auto& o : obstacles) {
        float ho = o.size / 2.0f;
        if (checkAABB(b.x, b.z, 0.2f, 0.2f, o.x, o.z, ho, ho))
            return true;
    }
    return false;
}
void drawText(const char* text, float x, float y, void* font) {
    if (!font) font = GLUT_BITMAP_HELVETICA_18;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glRasterPos2f(x, y);

    for (int i = 0; text[i] != '\0'; ++i) {
        glutBitmapCharacter(font, text[i]);
    }

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}



void drawStylizedBackground() {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.05f, 0.08f, 0.15f);
    glVertex2f(400, 400);
    for (int angle = 0; angle <= 360; angle += 10) {
        float rad = angle * 3.14159f / 180.0f;
        float r = 500;
        glColor3f(0.02f, 0.04f, 0.08f);
        glVertex2f(400 + cos(rad) * r, 400 + sin(rad) * r);
    }
    glEnd();
}

void drawTankModel(float x, float y, float angle) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    glColor3f(0.2f, 0.4f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-20, -10); glVertex2f(20, -10);
    glVertex2f(20, 10);  glVertex2f(-20, 10);
    glEnd();
    glColor3f(0.8f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, -3); glVertex2f(30, -3);
    glVertex2f(30, 3); glVertex2f(0, 3);
    glEnd();
    glPopMatrix();
}

void drawScoreIcons(float x, float y, int count, float r, float g, float b) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    for (int i = 0; i < count; ++i) {
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex2f(x + i * 15, y);
        glVertex2f(x + i * 15 + 10, y);
        glVertex2f(x + i * 15 + 10, y + 10);
        glVertex2f(x + i * 15, y + 10);
        glEnd();
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawArenaBorders() {
    glColor3f(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 2; ++i) {
        glPushMatrix();
        glTranslatef(0.0f, 1.0f, (i == 0 ? -20.5f : 20.5f));
        glScalef(41.0f, 2.0f, 1.0f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    for (int i = 0; i < 2; ++i) {
        glPushMatrix();
        glTranslatef((i == 0 ? -20.5f : 20.5f), 1.0f, 0.0f);
        glScalef(1.0f, 2.0f, 41.0f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }
}

void drawTank(const Tank& t, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(t.x, 1.0f, t.z);
    glRotatef(t.angle, 0, 1, 0);

    glPushMatrix();
    glScalef(3.0f, 1.0f, 2.0f);
    glColor3f(r, g, b);
    glutSolidCube(2.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glColor3f(r + 0.2f, g + 0.2f, b + 0.2f);
    glutSolidSphere(0.8, 16, 16);
    glPopMatrix();

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex3f(1.0f, 1.0f, -0.15f);
    glVertex3f(3.5f, 1.0f, -0.15f);
    glVertex3f(3.5f, 1.0f, 0.15f);
    glVertex3f(1.0f, 1.0f, 0.15f);
    glEnd();

    glPopMatrix();
}

void drawMap() {
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(-20.1f, 0.01f, -20.1f);
    glVertex3f(20.1f, 0.01f, -20.1f);
    glVertex3f(20.1f, 0.01f, 20.1f);
    glVertex3f(-20.1f, 0.01f, 20.1f);

    glEnd();
}

void drawBullets() {
    for (auto& b : bullets) {
        glPushMatrix();
        glTranslatef(b.x, 1.0f, b.z);
        glPushMatrix();
        glTranslatef(0.0f, -1.0f, 0.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.01f, 0.0f);
        for (int angle = 0; angle <= 360; angle += 10) {
            float rad = angle * M_PI / 180.0f;
            glVertex3f(cos(rad) * 0.25f, 0.01f, sin(rad) * 0.25f);
        }
        glEnd();
        glPopMatrix();
        if (b.owner == 1)
            glColor4f(1.0f, 0.0f, 0.0f, 0.2f);
        else
            glColor4f(1.0f, 0.5f, 0.1f, 0.2f);
        glutSolidSphere(0.35, 12, 12);
        float trailLength = 0.5f;
        float rad = b.angle * M_PI / 180.0f;
        float dx = -trailLength * cos(rad);
        float dz = trailLength * sin(rad);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        if (b.owner == 1)
            glColor3f(1.0f, 0.3f, 0.3f);
        else
            glColor3f(1.0f, 0.6f, 0.2f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(dx, 0.0f, dz);
        glEnd();
        if (b.owner == 1)
            glColor3f(1.0f, 0.0f, 0.0f);
        else
            glColor3f(1.0f, 0.5f, 0.1f);
        glutSolidSphere(0.2, 16, 16);
        glPopMatrix();
    }
}

void drawObstacles() {
    for (auto& o : obstacles) {
        glPushMatrix();
        glTranslatef(o.x, 1, o.z);
        if (o.type == 1) {
            glColor3f(0.3f, 0.3f, 0.3f);
            glutSolidCube(o.size);
        }
        else if (o.type == 2) {
            glColor3f(1.0f, 0.85f, 0.0f);
            glutSolidSphere(o.size * 0.6f, 16, 16);
        }
        else {
            glColor3f(0.2f, 0.4f, 0.9f);
            glutSolidCube(o.size);
        }
        glPopMatrix();
    }
}

void drawExplosions() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    for (auto& e : explosions) {
        float age = (currentTime - e.startTime) / 1000.0f;
        float scale = age * 5.0f;
        float alpha = 1.0f - age;
        if (alpha < 0) alpha = 0;
        glPushMatrix();
        glTranslatef(e.x, 1.5f, e.z);
        glColor4f(1.0f, 0.4f, 0.0f, alpha);
        glutSolidSphere(scale, 20, 20);
        glPopMatrix();
    }
}

void drawMenuTanksUnderOptions3D() {
    glViewport(0, 0, 800, 800);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0, 40, 0,
        0, 0, 0,
        0, 0, -1);
    glTranslatef(0, 5, 2);
    Tank leftTank = { -10, -2, 0, 3 };
    Tank rightTank = { 10, -2, 180, 3 };
    drawTank(leftTank, 0.1f, 0.1f, 0.6f);
    drawTank(rightTank, 0.6f, 0.1f, 0.1f);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
}

void drawPillButton(float x, float y, float width, float height, float r, float g, float b) {
    const int segments = 50;
    float radius = height / 2.0f;
    float cxLeft = x + radius;
    float cxRight = x + width - radius;
    float cy = y - radius;
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(cxLeft, y);
    glVertex2f(cxRight, y);
    glVertex2f(cxRight, y - height);
    glVertex2f(cxLeft, y - height);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cxLeft, cy);
    for (int i = 0; i <= segments; ++i) {
        float angle = M_PI / 2 + (i * M_PI / segments);
        glVertex2f(cxLeft + radius * cos(angle), cy + radius * sin(angle));
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cxRight, cy);
    for (int i = 0; i <= segments; ++i) {
        float angle = -M_PI / 2 + (i * M_PI / segments);
        glVertex2f(cxRight + radius * cos(angle), cy + radius * sin(angle));
    }
    glEnd();
}

void drawFireworks() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    for (auto& fw : fireworks) {
        float time = (currentTime - fw.startTime) / 1000.0f;
        if (time > 2.0f) continue;
        float alpha = 1.0f - (time / 2.0f);
        glPushMatrix();
        glTranslatef(fw.x, fw.y + time * 2.0f, fw.z);
        glColor4f(fw.r, fw.g, fw.b, alpha);
        for (int i = 0; i < 10; ++i) {
            glPushMatrix();
            glRotatef(i * 36, 1, 1, 0);
            glutSolidSphere(0.2f, 8, 8);
            glPopMatrix();
        }
        glPopMatrix();
    }
}

void drawMainMenu() {
    glColor3f(0.27f, 0.27f, 0.27f);
    drawText("TOP DOWN TANK BATTLE", 250, 560, GLUT_BITMAP_TIMES_ROMAN_24);
    float x = 300;
    float y = 500;
    float width = 200;
    float height = 50;
    float spacing = 60;
    for (int i = 0; i < menuCount; ++i) {
        float top = y - i * spacing;
        bool isSelected = (i == selectedMenuIndex);
        float r = isSelected ? 0.6f : 0.2f;
        float g = r, b = r;
        drawPillButton(x, top, width, height, r, g, b);
        int textWidth = 0;
        const char* label = menuOptions[i];
        for (int j = 0; label[j] != '\0'; ++j) {
            textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, label[j]);
        }
        float textX = x + (width - textWidth) / 2.0f;
        float textY = top - 35;
        glColor3f(1, 1, 1);
        drawText(label, textX, textY);
        if (isSelected) {
            drawText(">", textX - 20, textY);
        }
    }
    drawMenuTanksUnderOptions3D();
}

void drawNameInputScreen() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    drawText("ENTER PLAYER NAME:", 280, 450, GLUT_BITMAP_HELVETICA_18);
    drawText("Press ENTER to confirm", 260, 400, GLUT_BITMAP_HELVETICA_18);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void displayNameInput() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawNameInputScreen();
    glutSwapBuffers();
}

void displayMainMenu() {
    glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawMainMenu();
    glutSwapBuffers();
}

void drawRoundedRect(float x, float y, float width, float height, float radius, bool isSelected) {
    const int segments = 16;
    float angle;
    if (isSelected)
        glColor3f(0.6f, 0.6f, 0.6f);
    else
        glColor3f(0.3f, 0.3f, 0.3f);

    glBegin(GL_POLYGON);
    for (int i = 0; i <= segments; ++i) {
        angle = M_PI + (M_PI / 2) * (i / (float)segments);
        glVertex2f(x + radius + cos(angle) * radius, y - radius + sin(angle) * radius);
    }
    for (int i = 0; i <= segments; ++i) {
        angle = 1.5 * M_PI + (M_PI / 2) * (i / (float)segments);
        glVertex2f(x + width - radius + cos(angle) * radius, y - radius + sin(angle) * radius);
    }
    for (int i = 0; i <= segments; ++i) {
        angle = 0 + (M_PI / 2) * (i / (float)segments);
        glVertex2f(x + width - radius + cos(angle) * radius, y - height + radius + sin(angle) * radius);
    }
    for (int i = 0; i <= segments; ++i) {
        angle = M_PI / 2 + (M_PI / 2) * (i / (float)segments);
        glVertex2f(x + radius + cos(angle) * radius, y - height + radius + sin(angle) * radius);
    }
    glEnd();
    glColor3f(1, 1, 1);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x + radius, y);
    glVertex2f(x + width - radius, y);
    glVertex2f(x + width, y - radius);
    glVertex2f(x + width, y - height + radius);
    glVertex2f(x + width - radius, y - height);
    glVertex2f(x + radius, y - height);
    glVertex2f(x, y - height + radius);
    glVertex2f(x, y - radius);
    glEnd();
}

void drawPlayerNameEntry() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    if (enteringPlayer) {
        drawText("ENTER PLAYER NAME:", 260, 520);
        drawText(playerName, 260, 480);
    }
    else {
        drawText("ENTER ENEMY NAME:", 260, 520);
        drawText(enemyName, 260, 480);
    }
    drawText("Press ENTER to confirm", 260, 440);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawLevelSelection() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(220, 430);
    glVertex2f(580, 430);
    glVertex2f(580, 600);
    glVertex2f(220, 600);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(220, 430);
    glVertex2f(580, 430);
    glVertex2f(580, 600);
    glVertex2f(220, 600);
    glEnd();
    drawText("SELECT LEVEL (1-5):", 270, 550);
    drawText("Press number key to choose level", 270, 510);
    drawText("ESC to return to menu", 270, 470);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void shootBullet(const Tank& t, int owner) {
    float rad = t.angle * M_PI / 180.0f;
    float bx = t.x + cos(rad) * 3.5f;
    float bz = t.z - sin(rad) * 3.5f;
    bullets.push_back({ bx, bz, t.angle, owner, glutGet(GLUT_ELAPSED_TIME) });
}

void resetLevel() {
    bullets.clear();
    player = { -10, 0, 0, 3 };
    enemy = { 10, 0, 180, 3 };
    obstacles.clear();
    obstacles.push_back({ player.x + 4.0f, player.z + 5.0f, 3.0f, 999, 1 });
    obstacles.push_back({ enemy.x - 4.0f, enemy.z - 5.0f, 3.0f, 999, 1 });
    if (level >= 2) {
        obstacles.push_back({ 0.0f, 0.0f, 3.0f, 999, 1 });
    }
    int greyCount = 0, blueCount = 0, powerupCount = 0;
    switch (level) {
    case 1:
        greyCount = 1 + rand() % 2;
        blueCount = 0;
        powerupCount = 0;
        break;
    case 2:
        greyCount = 2 + rand() % 2;
        blueCount = 0;
        powerupCount = 1 + rand() % 2;
        break;
    case 3:
        greyCount = 3 + rand() % 2;
        blueCount = 3;
        powerupCount = 0;
        break;
    case 4:
        greyCount = 4 + rand() % 2;
        blueCount = 4;
        powerupCount = 3;
        break;
    case 5:
    default:
        greyCount = 6 + rand() % 3;
        blueCount = 3;
        powerupCount = 3;
        break;
    }
    for (int i = 0; i < greyCount; ++i) {
        float x, z;
        bool valid;
        do {
            x = -15 + rand() % 30;
            z = -15 + rand() % 30;
            valid = true;
            if (checkAABB(x, z, 3, 3, player.x, player.z, tankHalfWidth, tankHalfDepth) ||
                checkAABB(x, z, 3, 3, enemy.x, enemy.z, tankHalfWidth, tankHalfDepth) ||
                checkAABB(x, z, 3, 3, 0, 0, 1.5f, 1.5f)) {
                valid = false;
            }
            for (auto& o : obstacles) {
                if (o.type == 1) {
                    float ho = o.size / 2.0f;
                    if (checkAABB(x, z, 1.5f, 1.5f, o.x, o.z, ho, ho)) {
                        valid = false;
                        break;
                    }
                }
            }
        } while (!valid);

        obstacles.push_back({ x, z, 3.0f, 999, 1 });
    }
    for (int i = 0; i < blueCount; ++i) {
        float x, z;
        bool valid;
        do {
            x = -15 + rand() % 30;
            z = -15 + rand() % 30;
            valid = true;
            if (checkAABB(x, z, 3, 3, player.x, player.z, tankHalfWidth, tankHalfDepth) ||
                checkAABB(x, z, 3, 3, enemy.x, enemy.z, tankHalfWidth, tankHalfDepth) ||
                checkAABB(x, z, 3, 3, 0, 0, 1.5f, 1.5f)) {
                valid = false;
            }
            for (auto& o : obstacles) {
                if (o.type == 1) {
                    float ho = o.size / 2.0f;
                    if (checkAABB(x, z, 1.5f, 1.5f, o.x, o.z, ho, ho)) {
                        valid = false;
                        break;
                    }
                }
            }
        } while (!valid);
        obstacles.push_back({ x, z, 3.0f, 3, 0 });
    }
    for (int i = 0; i < powerupCount; ++i) {
        float x, z;
        bool valid;
        do {
            x = -15 + rand() % 30;
            z = -15 + rand() % 30;
            valid = true;
            if (checkAABB(x, z, 3, 3, player.x, player.z, tankHalfWidth, tankHalfDepth) ||
                checkAABB(x, z, 3, 3, enemy.x, enemy.z, tankHalfWidth, tankHalfDepth) ||
                checkAABB(x, z, 3, 3, 0, 0, 1.5f, 1.5f)) {
                valid = false;
            }
            for (auto& o : obstacles) {
                if (o.type == 0 || o.type == 1) {
                    float ho = o.size / 2.0f;
                    if (checkAABB(x, z, 1.5f, 1.5f, o.x, o.z, ho, ho)) {
                        valid = false;
                        break;
                    }
                }
            }
        } while (!valid);
        obstacles.push_back({ x, z, 3.0f, 3, 2 });
    }
}

void resetGame() {
    level = 1;
    playerWins = 0;
    enemyWins = 0;
    gameOver = false;
    levelTransition = false;
    bullets.clear();
    lastPlayerShot = 0;
    lastEnemyShot = 0;
    resetLevel();
    glutPostRedisplay();
    glutTimerFunc(0, update, 0);
}

void increaseDifficulty() {
    for (auto& o : obstacles) {
        o.size *= 1.2f;
    }
}

void drawHeart(float x, float y, float size, float r, float g, float b) {
    float radius = size / 2.0f;
    float triangleHeight = size * 1.2f;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x - radius, y);
    for (int i = 0; i <= 360; ++i) {
        float angle = i * M_PI / 180.0f;
        float dx = radius * cos(angle);
        float dy = radius * sin(angle);
        glVertex2f(x - radius + dx, y + dy);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x + radius, y);
    for (int i = 0; i <= 360; ++i) {
        float angle = i * M_PI / 180.0f;
        float dx = radius * cos(angle);
        float dy = radius * sin(angle);
        glVertex2f(x + radius + dx, y + dy);
    }
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(x - size, y);
    glVertex2f(x + size, y);
    glVertex2f(x, y - triangleHeight);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawLives(int xStart, int y, int lives, float r, float g, float b) {
    for (int i = 0; i < lives; i++) {
        drawHeart(xStart + i * 25, y, 5, r, g, b);
    }
}

void display() {
    if (showMainMenu || showPlayerNameEntry || showLevelSelection) {
        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (showHowToPlay) {
        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawHowToPlayScreen();
        glutSwapBuffers();
        return;
    }
    if (showMainMenu || showPlayerNameEntry || showLevelSelection) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 800, 0, 800);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        if (showPlayerNameEntry) {
            drawPlayerNameEntry();
            glutSwapBuffers();
            return;
        }
        if (showMainMenu) {
            drawMainMenu();
            glutSwapBuffers();
            return;
        }
        if (showLevelSelection) {
            drawLevelSelection();
            glutSwapBuffers();
            return;
        }
    }
    else {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, 1.0, 1.0, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 40, 40, 0, 0, 0, 0, 1, 0);
        drawMap();
        drawArenaBorders();
        drawTank(player, 0.1f, 0.1f, 0.6f);
        drawTank(enemy, 0.4f, 0.1f, 0.1f);
        drawObstacles();
        drawBullets();
        drawExplosions();
        if (gameOver)
            drawFireworks();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, 800, 0, 800);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glColor3f(0.0f, 0.0f, 0.0f);
        drawText(playerName, 300, 760);
        drawText("VS", 390, 760);
        drawText(enemyName, 460, 760);
        drawScoreIcons(301, 730, playerWins, 0.1f, 0.1f, 0.6f);
        drawScoreIcons(461, 730, enemyWins, 0.9f, 0.2f, 0.2f);
        drawLives(304, 710, player.health, 0.1f, 0.1f, 0.6f);
        drawLives(464, 710, enemy.health, 0.8f, 0.1f, 0.1f);
        if (gameOver) {
            drawText("GAME OVER!", 320, 400);
            drawText("PRESS R TO RESTART", 300, 370);
            drawText("SPACE: Player Shoot | ENTER: Enemy Shoot", 200, 340);
        }
        else if (levelTransition) {
            drawText("LEVEL COMPLETE!", 300, 400);
        }
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
    glutSwapBuffers();
}

void update(int value) {
    if (gameOver)
        return;
    if (levelTransition) {
        transitionTimer--;
        if (transitionTimer <= 0) {
            if (player.health > 0) playerWins++;
            else enemyWins++;
            level++;
            if (level > maxLevels) {
                gameOver = true;
                fireworks.clear();
                for (int i = 0; i < 20; ++i) {
                    fireworks.push_back({
                        -18.0f + static_cast<float>(rand()) / RAND_MAX * 36.0f,
                        5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f,
                        -18.0f + static_cast<float>(rand()) / RAND_MAX * 36.0f,
                        (float)(rand() % 100) / 100.0f,  // r
                        (float)(rand() % 100) / 100.0f,  // g
                        (float)(rand() % 100) / 100.0f,  // b
                        glutGet(GLUT_ELAPSED_TIME)
                        });
                }
                glutPostRedisplay();
                return;
            }
            levelTransition = false;
            resetLevel();
            increaseDifficulty();
        }
        else {
            glutTimerFunc(16, update, 0);
            return;
        }
    }
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [currentTime](const Bullet& b) {
        return currentTime - b.creationTime > 5000;
        }), bullets.end());
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [currentTime](const Explosion& e) {
        return currentTime - e.startTime > 1000;
        }), explosions.end());
    float speed = 0.4f;
    std::vector<Bullet> updated;
    for (auto& b : bullets) {
        b.x += speed * cos(b.angle * M_PI / 180.0);
        b.z -= speed * sin(b.angle * M_PI / 180.0);
        if (!isTankFullyInsideGrid(b.x, b.z)) continue;
        bool hitObstacle = false;
        for (auto& o : obstacles) {
            float ho = o.size / 2.0f;
            if (checkAABB(b.x, b.z, 0.2f, 0.2f, o.x, o.z, ho, ho)) {
                if (o.type == 1) {
                    hitObstacle = true;
                }
                else if (o.type == 2) {
                    o.health--;
                    if (o.health <= 0) {
                        explosions.push_back({ o.x, o.z, currentTime });
                        if (b.owner == 1 && player.health < 3) player.health++;
                        if (b.owner == 2 && enemy.health < 3) enemy.health++;
                    }
                    hitObstacle = true;
                }
                else {
                    o.health--;
                    if (o.health <= 0) {
                        explosions.push_back({ o.x, o.z, currentTime });
                    }
                    hitObstacle = true;
                }
                break;
            }
        }
        if (hitObstacle) continue;
        if (b.owner == 1 && checkAABB(b.x, b.z, 0.2f, 0.2f, enemy.x, enemy.z, tankHalfWidth, tankHalfDepth)) {
            explosions.push_back({ enemy.x, enemy.z, currentTime });
            enemy.health--;
            if (enemy.health <= 0) {
                levelTransition = true;
                transitionTimer = 90;
            }
            continue;
        }
        if (b.owner == 2 && checkAABB(b.x, b.z, 0.2f, 0.2f, player.x, player.z, tankHalfWidth, tankHalfDepth)) {
            explosions.push_back({ player.x, player.z, currentTime });
            player.health--;
            if (player.health <= 0) {
                levelTransition = true;
                transitionTimer = 90;
            }
            continue;
        }
        updated.push_back(b);
    }
    bullets = updated;
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
        [](const Obstacle& o) { return o.health <= 0 && o.type != 1; }),
        obstacles.end());
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
    if (playWithBot && !gameOver && !levelTransition) {
        float dx = player.x - enemy.x;
        float dz = player.z - enemy.z;
        float desiredAngle = atan2(-dz, dx) * 180.0f / M_PI;
        if (fabs(enemy.angle - desiredAngle) > 5.0f) {
            if (fmod(enemy.angle - desiredAngle + 360.0f, 360.0f) > 180.0f)
                enemy.angle += 2.0f;
            else
                enemy.angle -= 2.0f;
        }
        float rad = enemy.angle * M_PI / 180.0;
        float nextX = enemy.x + 0.4f * cos(rad);
        float nextZ = enemy.z - 0.4f * sin(rad);
        if (isTankFullyInsideGrid(nextX, nextZ) &&
            !tankHitsObstacle(nextX, nextZ) &&
            !tanksCollide(nextX, nextZ, player.x, player.z)) {
            enemy.x = nextX;
            enemy.z = nextZ;
        }
        static int lastAIShoot = 0;
        if (fabs(enemy.angle - desiredAngle) < 10.0f &&
            currentTime - lastAIShoot > 1200) {
            shootBullet(enemy, 2);
            lastAIShoot = currentTime;
        }
    }
}

void keyboard(unsigned char key, int x, int y) {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    if (key == 'm' || key == 'M') {
        if (showHowToPlay) {
            showHowToPlay = false;
            showMainMenu = true;
            glutPostRedisplay();
            return;
        }
        if (!showMainMenu) {
            showMainMenu = true;
            showLevelSelection = false;
            showPlayerNameEntry = false;
            bullets.clear();
            obstacles.clear();
            glutPostRedisplay();
            return;
        }
    }
    if (showPlayerNameEntry) {
        char* target = enteringPlayer ? playerName : enemyName;
        if (key == 13) {
            if (strlen(target) == 0)
                strcpy(target, enteringPlayer ? "Player" : "Enemy");
            if (enteringPlayer) {
                enteringPlayer = false;
                nameCharIndex = 0;
            }
            else {
                showPlayerNameEntry = false;
                showMainMenu = true;
            }
        }
        else if (key == 8 && nameCharIndex > 0) {
            target[--nameCharIndex] = '\0';
        }
        else if (isalpha(key) || key == ' ') {
            if (nameCharIndex < 49) {
                target[nameCharIndex++] = key;
                target[nameCharIndex] = '\0';
            }
        }
        glutPostRedisplay();
        return;
    }
    if (showMainMenu && key == 13) {
        switch (selectedMenuIndex) {
        case 0:
            playWithBot = false;
            showMainMenu = false;
            level = 1;
            resetLevel();
            glutTimerFunc(0, update, 0);
            break;
        case 1:
            playWithBot = true;
            showMainMenu = false;
            level = 1;
            resetLevel();
            glutTimerFunc(0, update, 0);
            break;
        case 2:
            showPlayerNameEntry = true;
            showMainMenu = false;
            enteringPlayer = true;
            memset(playerName, 0, sizeof(playerName));
            memset(enemyName, 0, sizeof(enemyName));
            nameCharIndex = 0;
            break;
        case 3:
            showMainMenu = false;
            showLevelSelection = true;
            break;

        case 4:
            showMainMenu = false;
            showHowToPlay = true;
            break;
        case 5:
            exit(0);
        }
        return;

    }
    if (showLevelSelection) {
        if (key >= '1' && key <= '5') {
            level = key - '0';
            showLevelSelection = false;
            resetLevel();
            glutTimerFunc(0, update, 0);
        }
        else if (key == 27) {
            showLevelSelection = false;
            showMainMenu = true;
        }
        glutPostRedisplay();
        return;
    }
    if (gameOver && (key == 'r' || key == 'R')) {
        resetGame();
        return;
    }
    if (!gameOver) {
        float move = 0.5f, rot = 5.0f;
        float rad = player.angle * M_PI / 180.0;
        float newX = player.x;
        float newZ = player.z;
        switch (key) {
        case ' ':
            if (currentTime - lastPlayerShot > shotDelay) {
                shootBullet(player, 1);
                lastPlayerShot = currentTime;
            }
            break;
        case 13:
            if (!showMainMenu && !showLevelSelection && !showPlayerNameEntry && !showHowToPlay) {
                if (currentTime - lastEnemyShot > shotDelay) {
                    shootBullet(enemy, 2);
                    lastEnemyShot = currentTime;
                }
            }
            break;

        case 'w': case 'W':
            newX += move * cos(rad);
            newZ -= move * sin(rad);
            break;
        case 's': case 'S':
            newX -= move * cos(rad);
            newZ += move * sin(rad);
            break;
        case 'a': case 'A':
            player.angle += rot;
            break;
        case 'd': case 'D':
            player.angle -= rot;
            break;
        }
        if ((key == 'w' || key == 'W' || key == 's' || key == 'S') &&
            isTankFullyInsideGrid(newX, newZ) &&
            !tankHitsObstacle(newX, newZ) &&
            !tanksCollide(newX, newZ, enemy.x, enemy.z)) {
            player.x = newX;
            player.z = newZ;
        }
        glutPostRedisplay();
    }
}

void specialKeys(int key, int x, int y) {
    if (showMainMenu) {
        if (key == GLUT_KEY_UP) {
            selectedMenuIndex = (selectedMenuIndex - 1 + menuCount) % menuCount;
            glutPostRedisplay();
            return;
        }
        else if (key == GLUT_KEY_DOWN) {
            selectedMenuIndex = (selectedMenuIndex + 1) % menuCount;
            glutPostRedisplay();
            return;
        }
    }
    if (!gameOver && !showMainMenu && !showLevelSelection && !showPlayerNameEntry) {
        float move = 0.5f, rot = 5.0f;
        float rad = enemy.angle * M_PI / 180.0;
        float newX = enemy.x;
        float newZ = enemy.z;
        switch (key) {
        case GLUT_KEY_UP:
            newX += move * cos(rad);
            newZ -= move * sin(rad);
            break;
        case GLUT_KEY_DOWN:
            newX -= move * cos(rad);
            newZ += move * sin(rad);
            break;
        case GLUT_KEY_LEFT:
            enemy.angle += rot;
            break;
        case GLUT_KEY_RIGHT:
            enemy.angle -= rot;
            break;
        }
        if ((key == GLUT_KEY_UP || key == GLUT_KEY_DOWN) &&
            isTankFullyInsideGrid(newX, newZ) &&
            !tankHitsObstacle(newX, newZ) &&
            !tanksCollide(newX, newZ, player.x, player.z)) {
            enemy.x = newX;
            enemy.z = newZ;
        }
        glutPostRedisplay();
    }
}

void enableKeyRepeat() {
    glutIgnoreKeyRepeat(0);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.76f, 0.70f, 0.50f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1.0, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

int main(int argc, char** argv) {
    srand(time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Top Down Tank Battle");
    init();
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMainLoop();
}
