import matplotlib.pyplot as plt
import numpy as np
import sys

SAMPLE_RATE = 20000  # 20 kHz
ACQ_TIME_SEC = 1
N_SAMPLES = SAMPLE_RATE * ACQ_TIME_SEC

#
# Primero cargo el driver del kernel con el comando:
#   sudo insmod *ubicacionDelDrive*/cdd_TP5.ko
#
# Verifico que el driver se haya cargado correctamente con:
#   lsmod | grep cdd_TP5
#
# Este driver permite adquirir señales digitales de dos pines GPIO (Pin18 y Pin22)
# de la Raspberry Pi a una tasa de muestreo de 20 kHz durante 1 segundo.
#
#
# Ejecutar el script pwmConfig.py para generar una senal de 20Hz en la salida del pin 12 
# (PWM0)
# 
# 
# Ahora ya estaria listo para usar el driver desde el espacio de usuario.
# Cómo se usa este script desde el espacio de usuario?
# 1. Seleccionar la señal y disparar la adquisición
# Para seleccionar la señal 0 (Pin18 - GPIO 24) y disparar la adquisición:
#       echo 0 | sudo tee /dev/cdd_TP5      (o, si se cuenta con los permisos: echo 0 > /dev/cdd_TP5)
#
# Para seleccionar la señal 1 (Pin22 - GPIO 25) y disparar la adquisición:
#       echo 1 | sudo tee /dev/cdd_TP5
#
# Cada vez que escribes "0" o "1" en el device, el driver realiza una 
# adquisición de 1 segundo a 20 kHz de la señal seleccionada.
#
# 2. Leer los datos adquiridos
# Para leer los datos adquiridos, puedes usar el comando:
#       sudo cat /dev/cdd_TP5 | tee samples.txt > /dev/null
# 
# Esto guardará los datos en un archivo llamado "samples.txt".
#
# 3. Para verificar cual fue la señal seleccionada, puedes leer el mensaje de log del driver:
#       dmesg | tail
#
# Esto mostrará el último mensaje del driver, que indica la señal seleccionada.
# 
# Asegúrate de que el driver esté cargado y funcionando correctamente antes de ejecutar este script.
# 4. Activa el venv con la librería matplotlib instalada:
#       source .venv/bin/activate
#
# 5. Ejecuta este script pasando la señal seleccionada como argumento, por ejemplo:
#      python3 plot_signal.py 0
# 
# Usa las teclas:
# z: Zoom in (menos tiempo, más detalle)
# x: Zoom out (más tiempo, menos detalle)
# Flechas izquierda/derecha: Mover la ventana de tiempo


# Mapeo de señal a pin físico y GPIO (kernel 6.2+)
SIGNAL_MAP = {
    0: {"pin": 18, "gpio": 24},
    1: {"pin": 22, "gpio": 25},
}

# Selección de señal (por argumento o por defecto)
if len(sys.argv) > 1 and sys.argv[1] in ("0", "1"):
    selected_signal = int(sys.argv[1])
else:
    selected_signal = 0  # Por defecto

info = SIGNAL_MAP[selected_signal]
pin = info["pin"]
gpio = info["gpio"]

# Lee los datos del archivo generado por el driver
with open("samples.txt", "r") as f:
    data = f.read().strip()
    values = np.array([int(c) for c in data[:N_SAMPLES]])

# Eje de tiempo
t = np.arange(N_SAMPLES) / SAMPLE_RATE

# Parámetros de zoom (octavas)
octave = 0  # 0: muestra todo, 1: mitad, 2: cuarto, etc.
center = N_SAMPLES // 2  # Centro de la ventana

def plot_window():
    global octave, center
    window = N_SAMPLES // (2 ** octave)
    if window < 100: window = 100  # No menos de 100 muestras
    start = max(0, center - window // 2)
    end = min(N_SAMPLES, start + window)
    start = max(0, end - window)  # Corrige si se pasa del final

    plt.clf()
    plt.plot(t[start:end], values[start:end], drawstyle='steps-post')
    plt.xlabel("Tiempo (s)")
    plt.ylabel("Nivel digital")
    plt.title(f"Señal digital muestreada (20 kHz, 1s)\n"
              f"Entrada seleccionada: Pin físico {pin} (GPIO {gpio})\n"
              f"Ventana: {window} muestras ({window/SAMPLE_RATE:.4f} s)")
    plt.ylim(-0.2, 1.2)
    plt.grid(True)
    plt.tight_layout()
    plt.draw()

def on_key(event):
    global octave, center
    if event.key == 'z':  # Zoom in (menos tiempo, más detalle)
        if octave < int(np.log2(N_SAMPLES)) - 1:
            octave += 1
            plot_window()
    elif event.key == 'x':  # Zoom out (más tiempo, menos detalle)
        if octave > 0:
            octave -= 1
            plot_window()
    elif event.key == 'left':
        window = N_SAMPLES // (2 ** octave)
        center = max(window // 2, center - window // 4)
        plot_window()
    elif event.key == 'right':
        window = N_SAMPLES // (2 ** octave)
        center = min(N_SAMPLES - window // 2, center + window // 4)
        plot_window()

plt.figure(figsize=(10, 4))
plot_window()
plt.connect('key_press_event', on_key)
plt.show()