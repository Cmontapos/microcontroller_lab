#!/usr/bin/env python3

import threading
import tkinter as tk
from queue import Empty, Queue

import serial


PUERTO = "/dev/ttyACM1"
VELOCIDAD = 115200
PASSWORD = b"Password"
mensajes_para_enviar = Queue()


def escuchar_serial():
    while True:
        try:
            with serial.Serial(PUERTO, VELOCIDAD, timeout=1) as puerto:
                ventana.after(0, estado.set, f"Conectado a {PUERTO}")

                while True:
                    try:
                        texto = mensajes_para_enviar.get_nowait()
                        puerto.write((texto + "\n").encode())
                        ventana.after(0, estado.set, f"Enviado: {texto}")
                    except Empty:
                        pass

                    mensaje = puerto.readline().strip()

                    if mensaje == PASSWORD:
                        ventana.after(0, ventana.destroy)
                        return

        except serial.SerialException:
            ventana.after(
                0, estado.set, f"Esperando conexión en {PUERTO}..."
            )
            if detener.wait(2):
                return


def enviar():
    texto = entrada.get().strip()
    if texto:
        mensajes_para_enviar.put(texto+"\n")
        entrada.delete(0, tk.END)


ventana = tk.Tk()
ventana.title("Pantalla bloqueada")
ventana.configure(bg="black")
ventana.attributes("-fullscreen", True)
ventana.attributes("-topmost", True)
ventana.protocol("WM_DELETE_WINDOW", lambda: None)

estado = tk.StringVar(value=f"Esperando conexión en {PUERTO}...")

tk.Label(
    ventana,
    text="PANTALLA BLOQUEADA",
    font=("Arial", 36, "bold"),
    fg="white",
    bg="black",
).pack(expand=True)

entrada = tk.Entry(
    ventana,
    font=("Arial", 18),
    width=30,
)
entrada.pack(pady=10)
entrada.bind("<Return>", lambda _evento: enviar())

tk.Button(
    ventana,
    text="Enviar por serial",
    font=("Arial", 14),
    command=enviar,
).pack()

tk.Label(
    ventana,
    textvariable=estado,
    font=("Arial", 16),
    fg="lightblue",
    bg="black",
).pack(pady=40)

detener = threading.Event()
hilo = threading.Thread(target=escuchar_serial, daemon=True)
hilo.start()

ventana.mainloop()
detener.set()
