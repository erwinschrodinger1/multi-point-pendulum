#include <GLFW/glfw3.h>
#include <cmath>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <vector>

// Constants
const float PI = 3.14159265358979323846;
const float gravity = 9.81;     // Acceleration due to gravity
const float deltaTime = 0.016;  // Time step
const float damping = 0.9999;   // Damping factor
const float trailDuration = 20; // Duration for each trail point to remain visible (in seconds)

// Pendulum class to manage each pendulum's state
class Pendulum
{
public:
    float angle;
    float angleSpeed;
    float length;
    float mass;

    Pendulum(float angle, float length, float mass)
        : angle(angle), angleSpeed(0.0), length(length), mass(mass) {}
};

// Structure to hold position and color data for the trail
struct TrailPoint
{
    float x, y;
    float alpha;
    float timeStamp; // Time when the point was added
};

// Mouse state
bool mousePressed = false;
double mouseX, mouseY;
bool draggingLastPendulum = false;

// Function to convert window coordinates to OpenGL coordinates
void windowToOpenGLCoordinates(double x, double y, double &outX, double &outY, int windowWidth, int windowHeight)
{
    outX = (x / windowWidth) * 6.0 - 3.0;
    outY = ((windowHeight - y) / windowHeight) * 6.0 - 3.0;
}

// Mouse button callback function
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            draggingLastPendulum = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
            draggingLastPendulum = false;
        }
    }
}

// Mouse motion callback function
void cursorPositionCallback(GLFWwindow *window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

void display(const std::vector<Pendulum> &pendulums, const std::deque<TrailPoint> &trail)
{
    glClear(GL_COLOR_BUFFER_BIT);

    float x = 0.0;
    float y = 0.0;

    // Draw the pendulums
    for (const auto &pendulum : pendulums)
    {
        float x_new = x + pendulum.length * sin(pendulum.angle);
        float y_new = y - pendulum.length * cos(pendulum.angle);

        // Draw the pendulum rod
        glBegin(GL_LINES);
        glVertex2f(x, y);
        glVertex2f(x_new, y_new);
        glEnd();

        // Draw the pendulum bob
        glBegin(GL_QUADS);
        glVertex2f(x_new - 0.05, y_new - 0.05);
        glVertex2f(x_new + 0.05, y_new - 0.05);
        glVertex2f(x_new + 0.05, y_new + 0.05);
        glVertex2f(x_new - 0.05, y_new + 0.05);
        glEnd();

        x = x_new;
        y = y_new;
    }

    // Draw the fading trail
    glBegin(GL_LINE_STRIP);
    for (const auto &point : trail)
    {
        glColor4f(1.0, 1.0, 1.0, point.alpha);
        glVertex2f(point.x, point.y);
    }
    glEnd();

    // Reset color to white
    glColor4f(1.0, 1.0, 1.0, 1.0);
}

void update(std::vector<Pendulum> &pendulums, std::deque<TrailPoint> &trail, int windowWidth, int windowHeight, float currentTime)
{
    if (mousePressed && !draggingLastPendulum)
    {
        double mouseXOpenGL, mouseYOpenGL;
        windowToOpenGLCoordinates(mouseX, mouseY, mouseXOpenGL, mouseYOpenGL, windowWidth, windowHeight);

        // Check if the mouse is near the last pendulum bob
        float lastX = 0.0;
        float lastY = 0.0;
        for (const auto &pendulum : pendulums)
        {
            lastX += pendulum.length * sin(pendulum.angle);
            lastY -= pendulum.length * cos(pendulum.angle);
        }

        if (std::hypot(lastX - mouseXOpenGL, lastY - mouseYOpenGL) < 0.1)
        {
            draggingLastPendulum = true;
        }
    }

    if (draggingLastPendulum)
    {
        double mouseXOpenGL, mouseYOpenGL;
        windowToOpenGLCoordinates(mouseX, mouseY, mouseXOpenGL, mouseYOpenGL, windowWidth, windowHeight);
        float dx = mouseXOpenGL;
        float dy = mouseYOpenGL;
        pendulums.back().angle = atan2(dx, -dy);
        pendulums.back().angleSpeed = 0.0;
    }
    else
    {
        // Calculate angles acceleration and update angles and speeds
        Pendulum &p1 = pendulums[0];
        Pendulum &p2 = pendulums[1];

        float m1 = p1.mass;
        float m2 = p2.mass;
        float l1 = p1.length;
        float l2 = p2.length;
        float a1 = p1.angle;
        float a2 = p2.angle;
        float a1_v = p1.angleSpeed;
        float a2_v = p2.angleSpeed;

        float num1 = -gravity * (2 * m1 + m2) * sin(a1);
        float num2 = -m2 * gravity * sin(a1 - 2 * a2);
        float num3 = -2 * sin(a1 - a2) * m2;
        float num4 = a2_v * a2_v * l2 + a1_v * a1_v * l1 * cos(a1 - a2);
        float denom = l1 * (2 * m1 + m2 - m2 * cos(2 * a1 - 2 * a2));
        float a1_a = (num1 + num2 + num3 * num4) / denom;

        num1 = 2 * sin(a1 - a2);
        num2 = (a1_v * a1_v * l1 * (m1 + m2));
        num3 = gravity * (m1 + m2) * cos(a1);
        num4 = a2_v * a2_v * l2 * m2 * cos(a1 - a2);
        denom = l2 * (2 * m1 + m2 - m2 * cos(2 * a1 - 2 * a2));
        float a2_a = num1 * (num2 + num3 + num4) / denom;

        p1.angleSpeed += a1_a * deltaTime;
        p1.angleSpeed *= damping;
        p1.angle += p1.angleSpeed * deltaTime;

        p2.angleSpeed += a2_a * deltaTime;
        p2.angleSpeed *= damping;
        p2.angle += p2.angleSpeed * deltaTime;
    }

    // Update the trail
    float x = 0.0;
    float y = 0.0;
    for (const auto &pendulum : pendulums)
    {
        x += pendulum.length * sin(pendulum.angle);
        y -= pendulum.length * cos(pendulum.angle);
    }
    trail.push_back({x, y, 1.0, currentTime});

    // Remove old trail points
    while (!trail.empty() && (currentTime - trail.front().timeStamp) > trailDuration)
    {
        trail.pop_front();
    }

    // Update the alpha of trail points
    for (auto &point : trail)
    {
        point.alpha = 1.0f - (currentTime - point.timeStamp) / trailDuration;
    }
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        return -1;
    }

    // Create a GLFW window
    GLFWwindow *window = glfwCreateWindow(800, 800, "Double Pendulum Simulation with Trail and Damping", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set the viewport
    glViewport(0, 0, 800, 800);

    // Set up the projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-3.0, 3.0, -3.0, 3.0, -1.0, 1.0);

    // Register mouse callbacks
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);

    // Initialize pendulums
    std::srand(std::time(0));
    std::vector<Pendulum> pendulums = {
        Pendulum(PI / 4, 1.0, 1.0), // First pendulum
        Pendulum(PI / 4, 1.0, 1.0)  // Second pendulum
    };

    // Trail of the last pendulum bob
    std::deque<TrailPoint> trail;

    float startTime = glfwGetTime();
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime() - startTime;

        // Update simulation
        update(pendulums, trail, 800, 800, currentTime);

        // Render
        display(pendulums, trail);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}