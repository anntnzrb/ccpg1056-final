#import "@preview/sourcerer:0.2.1": code
#import "template.typ": *

#show: project.with(
  title: "PA4: Diseño de un Pipeline con IPC", authors: (
    (
      name: "Juan Antonio González", email: "juangonz@espol.edu.ec", affiliation: "ESPOL (FIEC)",
    ), (
      name: "Oscar García Tinoco", email: "omgarcia@espol.edu.ec", affiliation: "ESPOL (FIEC)",
    ),
  ),
)

#show link: it => [#set text(blue); #underline(it)]

= Problemática a Resolver

El objetivo principal de este proyecto es aplicar técnicas de sincronización y
comunicación entre procesos que han sido revisadas en clase. Esta solución debe
implementar un pipeline de procesamiento de imágenes utilizando comunicación
entre procesos (*IPC*) y técnicas de sincronización en *C*.

La problemática específica consiste en diseñar e implementar un sistema
compuesto por *tres programas principales* que trabajen de forma concurrente:

1. *Publicador*: lee una imagen de entrada en formato _BMP_ y la almacena en un
  recurso de memoria compartida

2. *Desenfocador*: aplica un filtro de desenfoque (*blur*) a la primera mitad de la
  imagen

3. *Realzador*: aplica un kernel de realce de bordes (*edge detection*) a la
  segunda mitad de la imagen

Estos programas deben sincronizarse adecuadamente para procesar la imagen en
etapas, garantizando que:

- El *desenfocador* y el *realzador* se ejecuten como _procesos independientes_ una
  vez que el *publicador* haya cargado la imagen en memoria compartida
- Al finalizar, se combinen las dos porciones procesadas de la imagen en una sola
  y se guarde en disco

A continuación se presenta un diagrama de actividades que describe el
comportamiento esperado del pipeline @mermaid-chart:

// https://www.mermaidchart.com/app/projects/bed189c5-7739-4bfb-a9ad-8aadd6fc612d/diagrams/fc9137a0-e48e-4a9b-a93b-2b27f80860a7/version/v0.1/edit
#figure(
  image("assets/uml.png", width: 80%), caption: "Diagrama de Actividades del Pipeline",
)

#pagebreak()

= Limitaciones y Resoluciónes
Durante el desarrollo del proyecto se presentaron algunas limitaciones, cada una
de las cuales requirió soluciones específicas para garantizar que el programa
funcione como fue originalmente intencionado.

== Sincronización Entre Procesos

El principal problema surgió debido a que el publicador continuaba su ejecución
sin haber confirmado que los filtros que se ejecutaron como procesos
independientes, terminaran de ejecutarse. Estos ocasionaba que el publicador
intentara continuar con su ejecución y guardar una imagen incompleta o corrupta,
lo que resultaba en que el programa finalizara abruptamente o crasheara.

La solución a este problema fue implementar un publicador que ejecutara ambos
filtros de manera simultánea una vez que la memoria compartida ya se haya
mapeado, y que espere la finalización de estos procesos mediante la función `waitpid` para
garantizar que no se continue con la ejecución utilizando una imagen incompleta
y posiblemente dañada.

== División de Responsabilidades Entre Filtros

Otro problema que se presentó fue garantizar que cada filtro procesara
exactamente la mitad de la imagen asignada. Esto implicaba evitar la
sobreescritura de píxeles o la omisión de los que ya habían sido tratados por el
otro filtro, lo cual resultaba en la generación de una imagen con un
procesamiento de baja calidad o impreciso.

Para resolver este problema se implementó un publicador que asignaba los rangos
de filas y parámetros que definían el área de operación para cada filtro. Esta
asignación detallada permitía que cada filtro procesara exactamente la mitad de
la imagen que le correspondía. Una vez establecidos estos parámetros se logró la
división del trabajo mediante el uso de múltiples hilos, mejorando el
rendimiento del programa y acotando las instrucciones solicitadas.

== Gestión de Memoria

La gestión de memoria se presentó como un tópico bastante tedioso. Este problema
surgió debido a la necesidad de compartir datos entre procesos utilizando
memoria compartida, lo que aumentaba el riesgo de fugas de memoria si los
recursos no se liberaban adecuadamente.

Para lidiar con este problema, se utilizó memoria compartida mediante la
libreria *POSIX*. Se empleó la función `mmap` para mapear la memoria compartida.
Ahora, la asignación de memoria se realiza de manera dinámica según el tamaño de
la imagen la cual es liberada por la función `munmap` y `shm_unlink`.

#pagebreak()

= Salidas de Pantalla y Ejecución

#lorem(10)

#pagebreak()

= Anexos

== Repositorio Online

La totalidad del código desarrollado se encuentra disponible en el siguiente
#link("https://github.com/anntnzrb/ccpg1056-final")[repositorio de GitHub].

#pagebreak()

#bibliography("bib.bib", style: "ieee", full: true, title: "Referencias")