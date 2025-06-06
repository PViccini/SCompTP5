import socket
import time
import matplotlib.pyplot as plt

SOCKET_PATH = "/tmp/tmp-gpio.sock"  # Debe coincidir con el usado en QEMU

def read_gpio(pin):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(SOCKET_PATH)
        # El protocolo depende de cómo QEMU expone los GPIO
        # Ejemplo ficticio:
        s.sendall(f"read {pin}\n".encode())
        data = s.recv(1024)
        return int(data.decode().strip())

# Ejemplo de lectura y graficado simple
pins = [17, 18]  # GPIOs a leer

selected_pin = pins[0]  # Selecciona el pin a mostrar

times = []
values = []

plt.ion()
fig, ax = plt.subplots()

for t in range(100):
    value = read_gpio(selected_pin)
    times.append(time.time())
    values.append(value)
    ax.clear()
    ax.plot(times, values)
    ax.set_xlabel("Tiempo")
    ax.set_ylabel(f"GPIO {selected_pin}")
    plt.pause(0.1)

plt.ioff()
plt.show()