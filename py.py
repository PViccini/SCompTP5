import matplotlib.pyplot as plt
import numpy as np

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
# Este driver permite adquirir señales digitales de dos pines GPIO (Pin15 y Pin16)
# de la Raspberry Pi a una tasa de muestreo de 20 kHz durante 1 segundo.
#
# Ahora ya estaria listo para usar el driver desde el espacio de usuario.
# Cómo se usa este script desde el espacio de usuario?
# 1. Seleccionar la señal y disparar la adquisición
# Para seleccionar la señal 0 (Pin15 - GPIO 22) y disparar la adquisición:
#       echo 0 | sudo tee /dev/cdd_TP5
#
# Para seleccionar la señal 1 (Pin16 - GPIO 23) y disparar la adquisición:
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
#

# Lee los datos del archivo generado por el driver
with open("samples.txt", "r") as f:
    data = f.read().strip()
    # Si los datos están sin separadores:
    values = np.array([int(c) for c in data[:N_SAMPLES]])

# Eje de tiempo
t = np.arange(N_SAMPLES) / SAMPLE_RATE

# Graficar
plt.figure(figsize=(10, 4))
plt.plot(t, values, drawstyle='steps-post')
plt.xlabel("Tiempo (s)")
plt.ylabel("Nivel digital")
plt.title("Señal digital muestreada (20 kHz, 1 segundo)")
plt.ylim(-0.2, 1.2)
plt.grid(True)
plt.show()