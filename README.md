Este repositorio contiene el código perteneciente al proyecto de la asignatura ELCO de la ETSIT UPM (2021).

# Toy2Joy

## Distribución del código

En primer lugar la carpeta *Toy2Joy* contiene el SW embebido en el ESP32. Las librerías usadas son:

* Hardware Serial
* DfPlayer
* Neopixel-Adafruit
* SD
* CSV parser

Nota: Aunque si se examina el código se puede llegar a la conclusión de que el código es portable a una arquitectura ArduinoUNO, no está testada y por lo tanto no puede confirmase su validez.

En segundo lugar, se encuentra la carpeta *generateRGBmatrixCSV* dentro de la cual se encuentra el script de python que traduce los archivos ".png" de la carpeta *Caracteres8x8* a diferentes fichero ".h" en la carpeta *data*.

## Miembros del equipo

* Imanol Torres Inchaurza
* Carlos José Llorente Cortijo
* Noelia Blázquez García
* María Gómez Heredero
* Rodrigo Moratilla Alarcón
