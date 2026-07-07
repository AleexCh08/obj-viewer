Para ejecutar tu programa directamente desde la terminal de VS Code, asegúrate de estar en la raíz de tu proyecto y utiliza este comando:

```bash
.\build\MiApp.exe

```

*(Nota: Si usas PowerShell, el comando anterior es el correcto. Si usas Git Bash o una terminal de Linux/WSL, el comando sería `./build/MiApp.exe`).*

---

### Código para tu `README.md`

Crea un archivo llamado `README.md` en la raíz de tu proyecto y pega el siguiente contenido. Está redactado de forma profesional y refleja la estructura moderna que acabamos de configurar:

```markdown
# OBJ Viewer

Visor de modelos 3D interactivo desarrollado en C++ moderno. Este proyecto permite cargar, visualizar y manipular archivos de objetos 3D (.obj) utilizando una interfaz gráfica en tiempo real.

## 🚀 Características

* **Renderizado 3D en tiempo real:** Construido sobre OpenGL.
* **Carga de modelos:** Integración con `tinyobjloader` para importar archivos `.obj`.
* **Interfaz de Usuario (UI):** Controles interactivos utilizando ImGui.
* **Diálogos nativos:** Explorador de archivos del sistema a través de `tinyfiledialogs`.
* **Cálculos matemáticos:** Operaciones de álgebra lineal y transformaciones gestionadas por GLM.

## 🛠️ Tecnologías y Librerías Utilizadas

* **C++17**
* **OpenGL** (API gráfica)
* **GLFW** (Gestión de ventanas y eventos)
* **GLAD** (Cargador de extensiones de OpenGL)
* **ImGui** (Interfaz gráfica de usuario inmediata)
* **GLM** (Librería matemática para gráficos)
* **CMake** (Sistema de construcción y configuración)
* **MinGW / GCC** (Compilador)

## 📋 Requisitos Previos

Para compilar y ejecutar este proyecto en tu entorno local, necesitas tener instalado:

1. [Visual Studio Code](https://code.visualstudio.com/)
2. [CMake](https://cmake.org/download/) (Añadido al PATH del sistema)
3. Compilador GCC (Recomendado: MSYS2 / MinGW-w64)
4. Extensiones de VS Code: `C/C++` y `CMake Tools`

## ⚙️ Compilación y Ejecución

El proyecto está estructurado para ser construido fácilmente con CMake. Sigue estos pasos desde tu terminal:

1. **Clonar el repositorio:**
   ```bash
   git clone [https://github.com/TU_USUARIO/obj-viewer.git](https://github.com/TU_USUARIO/obj-viewer.git)
   cd obj-viewer

```

2. **Configurar el proyecto con CMake:**
```bash
cmake -S . -B build -G "MinGW Makefiles"

```


3. **Compilar el código fuente:**
```bash
cmake --build build --config Debug

```


4. **Ejecutar la aplicación:**
```bash
.\build\MiApp.exe

```


## 👨‍💻 Desarrollado por

**AleexCh**

```

```