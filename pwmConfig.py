import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(18, GPIO.OUT)
pwm = GPIO.PWM(18, 1000)  # 1 kHz
pwm.start(50)             # 50% duty cycle

try:
    time.sleep(60)        # Mantén la señal 60 segundos
finally:
    pwm.stop()
    GPIO.cleanup()