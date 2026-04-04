# Sistema de Chat Local Simulado en C

Universidad Nacional Autónoma de Honduras  
Facultad de Ingeniería — Ingeniería en Sistemas Computacionales  
Programación Estructurada — PE-1000

## Descripción

Sistema de chat local que permite a varios usuarios conectarse, ver quiénes están en línea, enviarse mensajes privados y mensajes grupales. Toda la comunicación se realiza mediante archivos en disco (sin sockets ni red), usando procesos (`fork`), hilos (`pthread`), mutex y señales UNIX.

## Requisitos

- Sistema operativo Linux (o WSL en Windows)
- GCC con soporte para pthreads
- `make`

## Compilación

Desde la raíz del proyecto:

```bash
make
```

Esto genera dos ejecutables: `chat_server` y `chat_client`.

Para limpiar los ejecutables y la carpeta `data/`:

```bash
make clean
```

## Ejecución

### 1. Iniciar el servidor

Abrir una terminal y ejecutar:

```bash
./chat_server
```

El servidor crea automáticamente la carpeta `data/` con todos los archivos necesarios. No hay que crear nada manualmente. Debe estar corriendo antes de conectar cualquier cliente.

### 2. Conectar un cliente

Abrir una terminal separada por cada usuario y ejecutar:

```bash
./chat_client <nombre_de_usuario>
```

Ejemplo:

```bash
./chat_client ana
```

El nombre de usuario no puede contener el carácter `|`.

## Comandos disponibles

Una vez conectado, el cliente muestra un prompt `>` donde se pueden escribir los siguientes comandos:

| Comando | Descripción | Ejemplo |
|---|---|---|
| `LIST` | Ver los usuarios conectados en este momento | `LIST` |
| `PRIVATE <usuario> <mensaje>` | Enviar un mensaje privado a otro usuario | `PRIVATE carlos Hola, ¿cómo estás?` |
| `GLOBAL <mensaje>` | Enviar un mensaje a todos los usuarios conectados | `GLOBAL Buenas tardes a todos` |
| `LOGOUT` | Cerrar sesión y salir del chat | `LOGOUT` |

## Apagar el servidor

Presionar `Ctrl+C` en la terminal donde corre el servidor. El servidor notifica a todos los clientes conectados, limpia los archivos temporales y se cierra ordenadamente.

## Estructura del proyecto

```
chat_local/
├── src/
│   ├── server.c          ← proceso servidor
│   ├── client.c          ← proceso cliente
│   ├── protocol.h        ← constantes y tipos compartidos
│   ├── user_manager.c    ← gestión de usuarios
│   ├── user_manager.h
│   ├── file_comm.c       ← lectura/escritura de inbox y outbox
│   ├── file_comm.h
│   ├── logger.c          ← registro de eventos
│   └── logger.h
├── data/                 ← creada en runtime, no va en el repo
│   ├── usuarios.txt
│   ├── chat.log
│   ├── inbox/
│   └── outbox/
├── Makefile
└── README.md
```

## Notas

- La carpeta `data/` es creada por el usuario al no existir o luego de cada make clean, dentro de la carpeta se crean nada mas las carpetas inbox y outbox
- Cada cliente crea sus propios archivos `inbox` y `outbox` identificados por su PID, y los elimina al cerrar sesión.
- El archivo `data/chat.log` guarda un registro de todos los eventos con marca de tiempo.
- Si un cliente se cierra abruptamente sin hacer LOGOUT, el servidor lo detecta y lo elimina de la tabla de usuarios automáticamente.