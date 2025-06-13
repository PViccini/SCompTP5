# Trabajo Práctico N°5: Device Drivers

## Cátedra de Sistemas de Computación, FCEFyN, UNC – 2025

**Grupo:** Los puntos flotantes  
**Integrantes:** Julieta Bernaus, Franco Di Pasquo, Carlos Patricio Viccini  
**Docente:** Javier Jorge

---

## Introducción

En este trabajo se aborda el desarrollo de un driver de dispositivo de carácter (CDD) para la plataforma Raspberry Pi, orientado a la adquisición de señales digitales a alta velocidad. El objetivo es comprender la interacción entre el espacio de usuario y el kernel de Linux, así como la manipulación eficiente de periféricos mediante drivers propios.

---

## Objetivos

- Implementar un driver de carácter capaz de sensar dos señales digitales externas durante un intervalo de un segundo.
- Permitir la selección dinámica de la señal a sensar desde el espacio de usuario.
- Desarrollar una aplicación en Python que lea y grafique la señal adquirida, mostrando información relevante sobre la adquisición.

---

## Descripción del Hardware y Entorno

Se utilizó una Raspberry Pi 3 Model B, con sistema operativo Linux actualizado (kernel 6.2+). La adquisición de señales se realiza sobre los pines físicos 18 (GPIO24) y 22 (GPIO25), configurados como entradas digitales. La comunicación con la placa se realizó mediante SSH sobre red local.

---

## Implementación del Driver

El driver, implementado en C (`cdd_TP5.c`), utiliza la API GPIO moderna del kernel para acceder a los pines. Se registró un dispositivo de carácter en `/dev/cdd_TP5`, permitiendo operaciones de lectura y escritura desde el espacio de usuario.

### Principales características:

- **Selección de señal:** Mediante escritura de '0' o '1' en el dispositivo, el driver selecciona el GPIO a sensar.
- **Adquisición de datos:** Al seleccionar la señal, el driver realiza una adquisición de 20.000 muestras en 1 segundo (20 kHz), almacenando los datos en un buffer interno.
- **Lectura de datos:** El usuario puede leer el buffer completo como una secuencia de caracteres ('0' y '1'), representando el estado digital de la señal muestreada.
- **Sincronización:** Se emplea un mutex para evitar condiciones de carrera durante la adquisición y lectura.

## Pruebas y Resultados

Se realizaron pruebas de adquisición utilizando la señal PWM generada por pwmConfig.py. El driver demostró adquirir correctamente las muestras a 20 kHz durante 1 segundo, sin pérdidas ni errores de sincronización. La visualización en py.py permitió analizar la forma de onda y validar la fidelidad de la adquisición.

---

## Conclusiones

El desarrollo del driver de dispositivo permitió comprender en profundidad la interacción entre el hardware y el software en sistemas Linux. La Raspberry Pi demostró ser una plataforma versátil y potente para el desarrollo de proyectos de adquisición de datos y control.

---

## Anexos

- Código fuente del driver: [`cdd_TP5.c`](./cdd_TP5.c).
- Script de Python para adquisición y graficación: [`py.py`](./py.py).
- Generador de señal PWM para pruebas: [pwmConfig.py](./pwmConfig.py).
- Videos de las pruebas realizadas:[`configuracion_rpi_carge_del_cdd.mp4`](./video/configuracion_rpi_carga_del_cdd.mp4), [`generacion_de_muestras.mp4`](./video/generacion_de_muestras.mp4), [`muestreo_variando_frequencia.mp4`](./video/muestreo_variando_frecuencia.mp4).

---

## Referencias

1. Documentación oficial de Raspberry Pi.
2. Documentación del kernel de Linux.
3. Tutoriales y guías sobre desarrollo de drivers en Linux.
4. Material de cátedra y ejemplos provistos.

