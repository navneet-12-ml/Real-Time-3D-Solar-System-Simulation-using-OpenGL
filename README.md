# 🌌 3D Solar System Simulation (OpenGL + C++)

A real-time interactive **3D Solar System simulation** built using **C++ and OpenGL (FreeGLUT)**.  
The project visualizes the Sun and seven planets with orbital motion, axial rotation, lighting effects, and camera controls while demonstrating fundamental **computer graphics concepts** such as transformations, lighting, materials, and animation.

---

## 🚀 Features

- 🌞 Emissive Sun with glow effect
- 🪐 7 orbiting planets with individual speeds
- 🔄 Planetary self-rotation (spin)
- 💡 Realistic OpenGL lighting and material reflections
- 🛰 Visible orbital paths
- 💫 Saturn ring rendering
- 🎥 Interactive 3D camera controls
- ⚡ Adjustable simulation speed
- 🖥 On-screen control instructions (HUD)

---

## 🧠 Graphics Concepts Demonstrated

- 3D transformations (translation & rotation)
- Orbital motion simulation
- OpenGL lighting model
- Material properties (specular highlights & shininess)
- Perspective projection
- Real-time animation using elapsed time
- Blending and transparency effects

---

## 🛠 Tech Stack

- **Language:** C++
- **Graphics Library:** OpenGL
- **Window Toolkit:** FreeGLUT
- **Math Library:** C++ `<cmath>`

---

## 🎮 Controls

| Key | Action |
|----|----|
| Arrow Keys | Rotate camera |
| Z | Zoom in |
| X | Zoom out |
| + | Increase simulation speed |
| - | Decrease simulation speed |
| P | Pause / Resume simulation |
| ESC | Exit program |

---

## ⚙️ Compilation (Windows – MinGW)

```bash
g++ solar_shiny.cpp -I"path\to\freeglut\include" -L"path\to\freeglut\lib" -lfreeglut -lopengl32 -lglu32 -o solar.exe
